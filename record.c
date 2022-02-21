/*  RECORD PACKAGE
**  Written by S. Jeyakumar 28-10-2006

**  pack and write  data  to a file
**  also stream the data for getonline clients

**  gets overloaded for sampling rates smaller than 0.5 millisecond 
*/

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include <sys/poll.h>

#include "data.h"
#include "acqcommon.h"
#include "record.h"

#include "server.h"

#include "stream.c"
#include "recordthread.c"




struct timeval getscantime(int scan, struct tm *tmtime)
{
char *st ;
struct timeval  itime ;
struct tm  *tmt ;
int nchan ;
long int usec ;
long int basetime ;

          nchan = ahead->nchan ;
         
//to be locked
         itime = ahead->starttime ;
         basetime = itime.tv_sec - itime.tv_sec%2 ;
         itime.tv_sec -= basetime ;
         usec = itime.tv_sec*1000*1000 + itime.tv_usec ;

          if (scan == (BUFFERPAGES/nchan -1) )
          {
             usec -= (int)(1024*ahead->sampling*1000) ;
          }
          else
          {
            usec += (int)scan*1024*ahead->sampling*1000 ;
          }

          itime.tv_usec = usec%(1000*1000) ;
          itime.tv_sec =  (usec - itime.tv_usec)/(1000*1000) ;
          itime.tv_sec += basetime ;
            if (itime.tv_usec < 0 ) 
            { 
              itime.tv_sec -= 1 ;
              itime.tv_usec = (1000*1000)+itime.tv_usec ;
            } 

          tmt =  localtime(&itime.tv_sec);
         st = ctime(&itime.tv_sec) ;
         //fprintf(stderr,"%s", st);

         if( tmtime != 0 )
         { *tmtime = *tmt ; }

return  itime ;
}

int getfilename(char *fname, int ichan, int scan, char *source )
{
char tfname[581];
int rchan ;
struct tm  tmtime ;
int ret ;

           memset(fname,'\0', 581);
           memset(tfname,'\0', 581);
           getscantime(scan, &tmtime);
           rchan = ahead->channels[ichan] ;
           sprintf(tfname,"%4d-%02d-%02d-%02d%02d%02d.chan-%d.%s.mxd", 
                                    tmtime.tm_year+1900,
                                    tmtime.tm_mon+1,
                                    tmtime.tm_mday,
                                    tmtime.tm_hour,
                                    tmtime.tm_min,
                                    tmtime.tm_sec,
                                    rchan,
                                    source
                                    );

          if ( checkdatadir() <  0 ) recordstatus = STOPRECORD ;
          strcpy(fname,recorddir);
          strcpy(fname+strlen(fname),"/");
          strcpy(fname+strlen(fname),currentdir);
          strcpy(fname+strlen(fname),"/");
          strcpy(fname+strlen(fname),tfname);
          

}

int initmemory()
{
         shmid1 = shmget(shmkey1, totalshmsize, 0) ;
         shmid2 = shmget(shmkey2, shmparsize, 0) ;

         if( (shmid1 ==-1 ) )
         {
           fprintf(stderr,"RECORD: acq is not running no buffer shared memory 1\n"); 
           fprintf(stderr,"RECORD: Exiting... \n"); 
           exit(0);
         }

         if( (shmid2 ==-1 ) )
         {
           fprintf(stderr,"RECORD: acq is not running no par shared memory 2\n"); 
           fprintf(stderr,"RECORD: Exiting... \n"); 
           exit(0);
         }

        ahead=NULL ;
        ahead = (acqhead *)shmat(shmid2, NULL, SHM_RDONLY);

        if ( (ahead == NULL )  )
        {
          fprintf(stderr,"RECORD:  buffer 2 attachement error %d %d\n",
                            ahead, shmid2);
          fprintf(stderr,"RECORD: Exiting... \n"); 
          exit(0);
        }

       if (ahead->acqstatus == STOPACQ)
       {
           shmdt(ahead);
           fprintf(stderr,"RECORD: acq memory is deleted.  Exiting....\n"); 
           exit(0);
       }

        abuf = (acqbuf *) shmat(shmid1, NULL, SHM_RDONLY);

        if ( (abuf == NULL )  )
        {
          fprintf(stderr,"RECORD:  buffer 1 attachement error %d %d\n",
                             abuf, shmid1);
          fprintf(stderr,"RECORD: Exiting... \n"); 
          exit(0);
        }

#ifdef STREAM
        nvolts = PLOTTIMESPAN*60/(ahead->sampling/1000);
        if  ( (1.0*nvolts/1024/ahead->nchan/BUFFERPAGES) > 1 ) nvolts = BUFFERPAGES*1024 ;
        volts =  calloc(nvolts, sizeof(float));
        if ( (volts == NULL )  )
        {
          fprintf(stderr,"STREAM: volts memory error \n");
          fprintf(stderr,"RECORD: Exiting... \n"); 
          exit(0);
        }
#endif



}

int logmessage(char *fstr, char *message)
{
struct timeval curt ;
struct timezone tz;
char tstr[25] ;

 gettimeofday(&curt, &tz);
    strncpy(tstr,ctime(&curt.tv_sec),24);
    tstr[24] = '\0' ;

 if (reclogfp)
 {
    fprintf(reclogfp, "%s ",  fstr);
    fprintf(reclogfp, "%s", message);
    fprintf(reclogfp, " :%24s\n",  tstr);

  fflush(reclogfp);
 }
 else fprintf(stderr, "RECORD: logfile closed...... why?\n");
}


// write the header   information            //
int writeheader(int ichan, int fd)
{
char mstr[581] ;
char        *c ;
int         hoffset ;
time_t      curt ;

  

       if ( !fd )
       {
        sprintf(mstr, "Writing header.. Channel %d file closed for unknown reasons", ichan );
        logmessage("RECORD", mstr);
        return 0 ;
       }

      
       // to avoid race condition with serverthread //
       // check for the recording status            // 
     if ( (recordinfo.channelstat[ichan] & RECORDING ) )
     {

         memcpy(&dataheader.magic,"MEXARTDATA",10);

         curt = time(0);
         c = ctime(&curt);
         memcpy(&dataheader.obsdate, c, 24);

         memcpy(&dataheader.source, recordinfo.source[ichan], 8);
         dataheader.chan =  ichan ;
         dataheader.sampling =  ahead->sampling ;


         write(fd, &dataheader, 2048);
     
         sprintf(mstr, "Written header for channel %d", ichan ); 
         logmessage("RECORD", mstr);
     }
}

struct timeval writefile(int ichan)
{
char mstr[581] ;
char tstr[25];
double          volts[1024] ;
float           rvolts[1024] ;
int             i, j ;
int             scanno ; 
struct tm       tmtime ;
struct timeval  scantime ;
sampl_t         *dataptr ;
int             scanmin, scanmax ;
int             *recno ;
int             fd ;


              scanmin = recordinfo.oldscan[ichan] ;
              scanmax = recordinfo.curscan[ichan] ;
              recno = &recordinfo.recno[ichan] ;
              fd = recordinfo.fd[ichan] ;

      if (scanmin != scanmax )
      {
       // do the actual write 

          if (*recno == 0 ) writeheader(ichan, fd);


               if ( scanmin > scanmax ) 
               {
               // circular buffer effect 
               // scanmax is added with the maxscan 
                  scanmax = scanmax+(BUFFERPAGES/ahead->nchan);
               }



                 scanno = scanmin ; 
            for (j=scanmin ; j < scanmax ; j++ )
            {

                scantime =  getscantime(j, &tmtime);
                scanno = j%(BUFFERPAGES/ahead->nchan) ; 


                for (i=0 ; i < 1024 ; i++ )
                {
                  dataptr = abuf->buf + scanno*(ahead->nchan)*1024 + 
                         i*(ahead->nchan)+ichan ;
                  volts[i] = ahead->adclowervalue + ahead->ctovolts*(*dataptr);
                  rvolts[i] = (float)volts[i];
                  //if (i%256==0)
                  // fprintf(stderr,"%d %g\n", i ,  volts[i] );
                }

                // record the data in a file //
                if(fd)
                {
                 write(fd, recno, sizeof(int));
                 write(fd, &scantime, sizeof(struct timeval));
                 write(fd, rvolts, sizeof(float)*1024);

                 strcpy(tstr, asctime(&tmtime) );
                 tstr[24] = '\0' ;
                 sprintf(mstr, "Channel %d recno scanno scan-time %d %d %24s", 
                         ichan, *recno, scanno, tstr);
                 logmessage("RECORD", mstr);
                }
                else
                {
                 sprintf(mstr, "Channel %d recno %d file closed for unknowd reasons", 
                         ichan, *recno );
                 logmessage("RECORD", mstr);
                }

               (*recno)++ ;
            }

      }

 return scantime ;
}


/////////////////////////////////////////////////
//                 record_time_stop            //
/////////////////////////////////////////////////
//                                             //
// This adds extra overhead to check whether   //
// end time has reached for every scan and     //
// may be avoided by an external program       //
// to check for the end time for a stop signal //
//                                             //
// if the end time has reached                 //
//   stop recording                            //
//   close the file                            //
//   reset recno to zero                       //
int record_time_stop(struct timeval scantime, int ichan)
{
char mstr[581] ;

  
  if (recordinfo.cmd[ichan] == 's' )
  {
    if ( scantime.tv_sec >= recordinfo.endtime[ichan].tv_sec )
    {
    // end has reached  
     recordinfo.channelstat[ichan] &= (~RECORDING) ;
     close(recordinfo.fd[ichan]);
     recordinfo.fd[ichan] = 0;
     recordinfo.recno[ichan]  = 0 ;
     recordinfo.begintime[ichan].tv_sec  = 0 ;
     recordinfo.begintime[ichan].tv_usec  = 0 ;
     recordinfo.endtime[ichan].tv_sec  =  0 ;
     recordinfo.endtime[ichan].tv_usec  =  0 ;
     recordinfo.cmd[ichan] == 'e' ;
     sprintf(mstr, "  endtime reached, stopping channel %d", ichan ); 
     logmessage("RECORD", mstr);
    }
  }
}

// close the files that are open         //             
int recordend()
{
int i ;

       for ( i=0 ; i < recordinfo.nchan ; i++ )
       {
         if ( recordinfo.channelstat[i] & RECORDING )
         {
            close( recordinfo.fd[i]);
         }
       }
// release the shared memory
    shmdt(abuf);
    shmdt(ahead);
}


int changefilename(int ichan)
{
// close the file if the time is crossing 00 hour
// close the file
// set recordno to zero
struct timeval curt ;
struct timeval oldt ;
struct timezone tz;
char otstr[25] ;
char ctstr[25] ;
char tstr[11] ;
int oldh ;
int curh ;
int cscan, oscan ;
char mstr[581] ;

        cscan = recordinfo.curscan[ichan] ;
        oscan = recordinfo.oldscan[ichan] ;
        if (oscan == cscan ) { return 0 ; }
 //        sprintf(mstr, "Changing directory and filename %d : %d\n", cscan, oscan);
 //        logmessage("RECORD", mstr);

   if ( strncmp(recordinfo.source[ichan],"TMSERIES",8) == 0 )
   if ( recordinfo.recno[ichan] != 0 )
   {

     oldt=getscantime(recordinfo.oldscan[ichan], 0);
     curt=getscantime(recordinfo.curscan[ichan], 0);
      memset(tstr,0,11);
      strncpy(ctstr,ctime(&curt.tv_sec),24);
      ctstr[24] = '\0' ;
      strncpy(tstr,ctstr+11,2);
      sscanf(tstr,"%d", &curh);

      memset(tstr,0,11);
      strncpy(otstr,ctime(&oldt.tv_sec),24);
      otstr[24] = '\0' ;
      strncpy(tstr,otstr+11,2);
      sscanf(tstr,"%d", &oldh);

      if ( curh < oldh )
      {
      /* change of time here */
         sprintf(mstr, "Changing directory and filename" );
         //sprintf(mstr, "Changing directory and filename");
         logmessage("RECORD", mstr);
         sprintf(mstr, "curscan %d : oldscan %d"
               ,cscan,oscan);
         logmessage("RECORD", mstr);
         sprintf(mstr, "curh %d : oldh %d"
               ,curh, oldh);
         logmessage("RECORD", mstr);
         sprintf(mstr, "curt : oldt %d"
               ,ctstr, otstr);
         logmessage("RECORD", mstr);

         if ( recordinfo.fd[ichan] ) close(recordinfo.fd[ichan]) ;
         recordinfo.recno[ichan] = 0 ;
         recordinfo.fd[ichan] = 0 ;
      }
   }
 return 0 ;
}

// records each channel in the aprropriate file//
int recordchan()
{
char mstr[581];
int            lscan ;
struct timeval scantime ;
int            fd ;
int            i ;
char *cc ;

       dataheader.sampling =  ahead->sampling ;
       while( recordstatus != STOPRECORD ) 
       {
//to be locked
           lscan = abuf->stat ;
          for ( i=0 ; i < ahead->nchan ; i++ )
          {

              if( recordinfo.channelstat[i] & RECORDING ) 
              {

                  recordinfo.oldscan[i] = recordinfo.curscan[i] ;
                  recordinfo.curscan[i] = lscan ;

                  changefilename(i);

// BEWARE of race condition    here               //
// recno may be zero because of the serverthread  //
// overwriting it                                 //
// so check for  recording variable again         //
                 if( recordinfo.recno[i] == 0 )
                 {
                    //open the the file only if that was not open before
                    if ( ( !recordinfo.fd[i] ) &&
                         (recordinfo.channelstat[i] & RECORDING)
                       )
                    {
                     //open file here 
                     recordinfo.oldscan[i] = recordinfo.curscan[i] ;
                     getfilename(recordinfo.fname[i],   i, 
                               recordinfo.curscan[i], 
                               recordinfo.source[i] ) ;

                      fd = open(recordinfo.fname[i], O_WRONLY|O_CREAT, filemode) ;
                        sprintf(mstr,"  data filename %s", 
                                                  recordinfo.fname[i] );
                        logmessage("RECORD", mstr);

                      if ( fd <= 0) 
                      {
                        sprintf(mstr,"  error opening data file for source %s", 
                                     recordinfo.source[i]);
                        logmessage("RECORD", mstr);
                        cc = strerror(errno);
                        sprintf(mstr,"  %s", cc);
                        logmessage("RECORD", mstr);
                       // stop recording 
                        recordinfo.channelstat[i] &= (~RECORDING)  ;
                      }
                      else
                      {
                        sprintf(mstr,"  opening data file for source %s", 
                                     recordinfo.source[i]);
                        logmessage("RECORD", mstr);
                       recordinfo.fd[i] =  fd ;
                      }
                    }

                    // set the beginning and end times here //
                       recordinfo.begintime[i] =  
                        getscantime(recordinfo.curscan[i], 0);

                     if (recordinfo.cmd[i] == 's' )
                     {
                       if (recordinfo.endtime[i].tv_sec < 0 )
                       {
                        recordinfo.endtime[i].tv_sec = 
                             recordinfo.begintime[i].tv_sec - 
                             recordinfo.endtime[i].tv_sec - 
                             (int)1024*ahead->sampling/1000 ;
                       }
                     }
                    
                 }
              
                scantime = writefile(i);
                record_time_stop(scantime,i);

              }
              else
              {
              // do this again to avoid race condition //
              // from the serverthread                 // 
                  recordinfo.recno[i] = 0  ;
                  if ( recordinfo.fd[i] > 0 ) 
                  {
                       close(recordinfo.fd[i] );
                  }
 
              }

             

          }

          if (ahead->acqstatus == STOPACQ ) recordstatus = STOPRECORD ;
       }

// useful only if the record program    //
// receives a STOP signal               //
         recordend();


return 0 ;
}


void recordmain(void *arg)
{
int        ichan;
int i ;
char *c ;
int  ct  ;
struct timeval desttime ;

          

        recordinfo.nchan = ahead->nchan;
        for (i=0; i < recordinfo.nchan; i++ )
        {
          recordinfo.channelstat[i] &= ~RECORDING;
        }


        recordchan();


}


int readsetupfile( char *inpfile )
{
char *c;
char *st ;
char line[181];
char str[181] ;
char tstr[181] ;
char *sptr ;
int ival ;
FILE *fp;
time_t      curt ;
struct tm *lttime ;
int commentbegin ;
int ct ;
char *ap ;


        commentbegin = 0 ;
        memset(&dataheader,0, 2048);

       fp = fopen(inpfile, "r");
       if (!fp) 
       {
        fprintf(stderr, "Error opening file %s \n", inpfile );
        exit(0);
       }

// initial values //
         strcpy(recordlogfilename,"record.log");

       fgets(line, 180, fp);
     while (!feof(fp) )
     {
       if( *line != '#' )
       {
         strcpy(str, line); sptr = str ;
         st = strsep(&sptr,"=");

          if ( strstr(st, "recorddir") != NULL )
          {
             st = strsep(&sptr,"=");
             if (sscanf(st,"%s", tstr) == 1 ) strcpy(recorddir,tstr);
          }

          if ( strstr(st, "recordlogfile") != NULL )
          {
            st = strsep(&sptr,"=");
		if (sscanf(st,"%s", tstr) == 1 ) strcpy(recordlogfilename,tstr);
          }
          if ( strstr(st, "observer") != NULL )
          {
            st = strsep(&sptr,"=");
		if (sscanf(st,"%s", tstr) == 1 ) 
                      strncpy(dataheader.observer,tstr,27);

          }


          if ( commentbegin == 1 )
          {
            //st = strsep(&sptr,"=");
		if (strlen(st) >= 1 )
                {

                  strncpy(ap,sptr, 2019-strlen(dataheader.other) );

                }
                else
                 commentbegin = 2 ;
          }

          if ( strstr(st, "comment") != NULL )
          {
            if ( commentbegin != 1 )
            {
              st = strsep(&sptr,"=");
		if (sscanf(st,"%s", tstr) == 1 ) 
                  strncpy(dataheader.other,st,2019);
              commentbegin = 1 ;
            }
          }
              if ( commentbegin != 0 )
              {
                 // remove the trailing returns
                  ap = dataheader.other+strlen(dataheader.other) ;
                  if ( *(ap-1) == '\n' ) *(ap-1) = ' ' ;
              }

       }
       fgets(line, 180, fp);
     }


    strcpy(tstr, recorddir);
    strcpy(tstr+strlen(tstr), "/");
    strcpy(tstr+strlen(tstr), recordlogfilename);
     strcpy(recordlogfilename,tstr);

    curt = time(0);
    //tmt = gmtime(&curt);
    c = ctime(&curt);
    memset(str, '\0', 181);
    lttime =  localtime(&curt);
    sprintf(str, "%04d-%02d-%02d", 
                       lttime->tm_year+1900,
                       lttime->tm_mon+1,
                       lttime->tm_mday );



//    strncpy(str,c+8,2);
//    if (*str == ' ') *str='0' ;
//    strncpy(str+strlen(str),c+4,3);
//    strncpy(str+strlen(str),c+20,4);

     strcpy(recordlogfilename+strlen(recordlogfilename), ".");
     strcpy(recordlogfilename+strlen(recordlogfilename), str);


     fprintf(stderr,"RECORD: Logfilename %s \n", recordlogfilename );
}

int checkdatadir()
{
char mstr[181] ;
char str[181] ;
char tdir[581] ;
char *cc ;
char *c ;
int ret ;
struct tm  *lttime ;
time_t curt ;
//struct tm *tmt ;



    curt = time(0);
    //tmt = gmtime(&curt);
    c = ctime(&curt);
    lttime =  localtime(&curt);
    memset(str, '\0', 181);
    memset(tdir, '\0', 581);

    sprintf(str, "%04d-%02d-%02d", 
                       lttime->tm_year+1900,
                       lttime->tm_mon+1,
                       lttime->tm_mday );

//    strncpy(str,c+8,2); 
//    if (*str == ' ') *str='0' ;
//    strncpy(str+strlen(str),c+4,3); 
//    strncpy(str+strlen(str),c+20,4); 

    if( strcmp(currentdir, str) != 0 )
    {
      memset(currentdir, '\0', 181);
      strcpy(currentdir,str);
      // check the existence of the current dir
      strcpy(tdir, recorddir);
      strcpy(tdir+strlen(tdir), "/");
      strcpy(tdir+strlen(tdir), currentdir);
      strcpy(tdir+strlen(tdir), "/");
       umask(~dirmode);
       ret = mkdir(tdir, dirmode);
   
        if ( ret < 0 )
        {
          if ( errno != EEXIST)
          {
          sprintf(mstr,"  error opening directory %s", tdir);
          logmessage("RECORD", mstr);
          cc = strerror(errno);
          sprintf(mstr,"  %s", cc);
          logmessage("RECORD", mstr);
          }
          else { ret = 0; } 
        }
        else
        {
          sprintf(mstr,"  changing to directory %s", tdir);
          logmessage("RECORD", mstr);
        }
    }

  return ret ;
}


int main(int argc,  char *argv[])
{
int pret;
int i ;
int ichan;
int  ret ;
pid_t pid ;

           recordstatus = -1 ;
          if (argc == 1  )
          {
          //assume start, if no argument is present
            recordstatus = STARTRECORD ;
          }

          if (argc > 1  )
          {
            if (strstr(argv[1],"start" ) != NULL ) recordstatus = STARTRECORD ;
            if (strstr(argv[1],"stop" ) != NULL ) recordstatus = STOPRECORD ;
            if (strstr(argv[1],"status" ) != NULL ) recordstatus = STATUSRECORD ;
          }          

          if (argc > 2  )
          {
             strncpy(recordinpfilename, argv[2], 580 );
          }          

          if (recordstatus == -1)
          { 
             fprintf(stderr,"Usage: record start|stop|status  [input file name]\n");
             exit(0);
          } 


       //check for running record //   
       initthreadclient(&recordstatus);
       pret = pthread_join( threadclient, NULL);
       if (recordstatus == STOPRECORD) exit(0);

       // read the input parameters //
       readsetupfile(recordinpfilename);

       //init shared memory and check for running acq //

        //fprintf(stderr, "OK 2\n");
        initmemory();


       // go ahead running record //
        initfiles();

       pid = makedaemon1();


       // setup thread to receive commands //
       // child only 
       if ( pid  == 0 ) initthreadserver();

       //wait till the server is up and running 
       if ( pid != 0  ) 
       {
         while ( (ret = checkserver(&recordstatus) ) != 0 );
         exit (EXIT_SUCCESS);
       } 
       //child makes daemon2
       makedaemon2(pid);

#ifdef STREAM
       // setup thread for streaming  //
       initthreadstream();
#endif 
       // setup thread for actual recording //
       initthreadrecord();



#ifdef STREAM
       //pthread_join( threadstream, NULL);  
#endif 
       pthread_join( threadrecord, NULL);  
       pthread_join( threadserver, NULL);  
}
