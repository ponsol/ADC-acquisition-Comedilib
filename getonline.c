/*  GETONLINE PACKAGE
**  Written by S. Jeyakumar 28-10-2006

**  getonline does three functions 
**  1. plot the data using gnuplot when called as mxgplot 
**  2. write the data in /database/actual when called as mxdonline
**  3. write the data to pipe when called as mxponline


**  Usage:  program -c  channel1 channel2 ....   
*/


#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>
//#include <comedilib.h

#include <mcheck.h>


#include <sys/poll.h>
#include <pthread.h>

//#include <sys/ipc.h>
//#include <sys/shm.h>
#include <math.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>


//#define DEBUG

#include "server.h"
#include "cmd.h"

/* the following is expected by cmd.c */
/* to avoid redefinition of logmessage */
/* if defined define the subroutine logmessage */
#ifndef __LOGMESSAGE__
#define __LOGMESSAGE__
#endif

#include "cmd.c"

#include "getonline.h"


 
char   onlinedir[] = "/database/actual" ;
/* plotpar contains: timeinterval sfactor nrunmean nmeandiv readjust plotlock zeroref dmin dmax*/
_plotpar plotpar = {
            3.0,   /* timeinterval    (plot time interval in seconds  : input par   )  */
            1.0,   /* datarate (MBy/s)(plot time interval dat/drate  : input par   )  */
            5,     /* sfactor       (zoom  sfactor*rms is the scale : control par )  */
            1,     /* nrunmean      (number of points  runmean      : input par   )  */
           10,     /* nmeandiv      (number of divisions for runmean: input par   )  */
            0,     /* readjust      (readjust timespan window 
                                     see also PLOTTIMESPAN    : control par )  */
            0,     /* plotlock      (lock the plotting        : control par )  */
          0.0,     /* zeroref       (data offset              : to be read        )  */
          0.0,     /* dmin          (data min                 : to be calculated  )  */
          0.0      /* dmax          (data max                 : to be calculated  )  */
              } ; 

#include "scaledata.c"

#include "writeonline.c"

#include "plotgnu.c"


/* the online dir is prefixed to this file name */
/* if that exists otherwise /tmp is prefixed    */
char   logfilename[] = "mxonline.log" ;

pthread_t   threadPlotReceive; 
pthread_t   threadRecordServer; 
 
pthread_t   threadPlotMain; 
pthread_t   threadPlotInput; 


int logmessage(char *fstr, char *message)
{
struct timeval curt ;
struct timezone tz;
char tstr[25] ;

 gettimeofday(&curt, &tz);
    strncpy(tstr,ctime(&curt.tv_sec),24);
    tstr[24] = '\0' ;

 if (getonlinelogfp)
 {
    fprintf(getonlinelogfp, "%s ",  fstr);
    fprintf(getonlinelogfp, "%s", message);
    fprintf(getonlinelogfp, " :%24s\n",  tstr);

  fflush(getonlinelogfp);
 }
 else fprintf(stderr, "RECORD: logfile closed...... why?\n");

}


int parseoptionsplot(int argc, char *argv[])
{
char   *c ;
int    ct  ;
int    optionfound ;
int    i,j ;
int    ival ;

                 j = 0 ;
                 optionfound = 0 ; 
           for ( i =1 ; i < argc ; i++)
           {

              if (optionfound ) 
              {  
                if ( ct = sscanf(argv[i],"%d", &ival) )
                {
                    Rclient.pchannels[j]  = ival ;
                    j++ ;
                }
              }  
              if ( strstr(argv[i],"-c") != NULL )
              {
                 optionfound = 1 ; 
                 if ( sscanf(argv[i]+2,"%d", &ival) == 1 )
                 {
                  Rclient.pchannels[j] = ival ; j++ ; 
                 }
              }
           }

           Rclient.pnchan = j ;


          if (argc == 1)
          { 
            // assume channel 0 
            Rclient.pchannels[0]  = 0 ;
            Rclient.pnchan = 1 ;
          } 
          else
          {
            if (optionfound == 0 ) 
            { 
              fprintf(stderr,"PLOT: bad option\n"); exit(0); 
            } 
          }
}

// get the input events //
int getinputevent( int *pchannels, char *cmd )
{
char str[181];
char c ;
struct pollfd ufds;
float fval ;
int  ival ;
int  pnchan ;
int  i;
int  ret ;
int ct ;


    // first call send the cmd line options //
    if (Rclient.pnchan != -1 )
    {
         pnchan = Rclient.pnchan ; 
       for ( i= 0; i < pnchan ; i++ )
       {
         pchannels[i] = Rclient.pchannels[i] ;
         cmd[i] = 's' ;
       }
        Rclient.pnchan = -1 ;
      return pnchan ; 
    }

    //read input here //
         ufds.fd = 0 ;
         ufds.events = POLLIN  ;
         ufds.revents =  0;
         ret = poll(&ufds, 1, 2000);
         pnchan = 0 ;
      if (ret)
      {
        //input occurred //
         
          fgets(str, 180, stdin);
          ct =sscanf(str,"%c %d", &c, &ival);
           if(ct == 2)
           {
             pchannels[pnchan] = ival ;
             cmd[pnchan] = c ;
             pnchan++ ;
           }
           else
           {
             if ( c == 'q' ) pnchan = -1 ;
           }

             if( c =='z')
             {
               ct =sscanf(str,"%c %f", &c, &fval);
               if (ct == 2 ) plotpar.sfactor = fval ;
               pnchan = -2 ;
             }
             if( c =='r')
             {
               ct =sscanf(str,"%c %f", &c, &fval);
               if (ct == 2 ) plotpar.nrunmean = (int)fval ;
               pnchan = -3 ;
             }
             if( c =='o')
             {
               ct =sscanf(str,"%c %f", &c, &fval);
               if (ct == 2 ) plotpar.zeroref = fval ;
               pnchan = -4 ;
             }

      }

 return pnchan ;
}

void PlotInput3(char *argv)
{
/* input reader for mxponline */
/* read the data from gtk widget status */

   while ( Rclient.stopplot != STOPPLOT )
   {
     /*watch the struture defined in gtk routine */
     
   }
}
void PlotInput2(char *argv)
{
/* input reader for mxdonline */
/* Assume all channels received by the threadRecordServer are to be plotted */
/* Open the data port for all the channels  */
/* If "openagain" is reset by the threadRecordServer */
/* then redo the openning here   */
/* Send the "stream" command to the record server to start streaming */
/* Leave the memory management to the threadPlotReceive to handle */

char mes[81];
int i ;
int achan ;


   while ( Rclient.stopplot != STOPPLOT )
   {
     if (Rserver.openagain)
     {
        Rserver.openagain = 0 ;
      for ( i =0 ; i < Rserver.RecordNchan ; i++)
      {
             achan = Rserver.RecordChannels[i] ;

           // start the channel here
           if ( !( pdat[achan].status & SOCKETOPEN ) )
           {

               /* make the dataptr to null */
               /* memory allocation is done bye PlotReceive thread */
               if (pdat[achan].dataptr != NULL)
                 { free(pdat[achan].dataptr); }
               if (pdat[achan].timeptr != NULL)
                 { free(pdat[achan].timeptr); }
                pdat[achan].dataptr = NULL ;
                pdat[achan].timeptr = NULL ;


               pdat[achan].streamsock = 
               socketcmdopen("PLOT", recordcmdport+achan+1);
               sprintf(mes,"stream %d\n", achan);

              if ( socketcmdsend(pdat[achan].streamsock, mes,80) < 0 )
              {
                fprintf(stderr,"PLOT: streamport mesg cannot be sent %d\n", achan); 
              }
              else
              {
#ifdef DEBUG
                fprintf(stderr,"PLOT: mesg sent for chan %d\n", achan); 
#endif
              }
               /* streamsock ready, so triger other thread */
                if (pdat[achan].streamsock ) {
                  pdat[achan].status |= STREAMING;
                  pdat[achan].status |= SOCKETOPEN;
                }
          }
          else
          {
            fprintf(stderr,"PLOT: already chan open \n %d", achan);
          }
      } /* Nchan for loop ends here */
     } /*openagain loops ends here */


   }

}

void PlotInput1(char *argv)
{
/* readinput from terminal    */
/* used along with mxgplot option */

//int    i ;
//int    ichan;
//int    cmdsock ;
//int    rec ;
//struct timeval timeout ;

char   mes[181] ;
int    i ;
int    achan ;
int    pnchan ;
int    pchannels[MAXCHAN];
int    tcn ;
int    tcnls[MAXCHAN];
char   cmd[MAXCHAN+1];


         pnchan = -1 ;
   while ( Rclient.stopplot != STOPPLOT )
   {
    
      pnchan = getinputevent(pchannels, cmd);
#ifdef DEBUG
      fprintf(stderr,"PLOT: recevied chan %d %c\n", pnchan, cmd[0]);
#endif

       if(pnchan == -1 ) Rclient.stopplot = STOPPLOT ;

        if (pnchan <  -1 )
        {
          Rclient.dataready = 1;
        }

      for ( i =0 ; i < pnchan ; i++)
      {
             achan = pchannels[i] ;
        if (cmd[i] == 's' )
        {
          // start the channel here
          if ( !( pdat[achan].status & SOCKETOPEN ) )
          {
          
                 /* PlotReceive thread allocates memory */
                  pdat[achan].dataptr = NULL ;
                  pdat[achan].timeptr = NULL ;

               pdat[achan].streamsock = 
               socketcmdopen("PLOT", recordcmdport+achan+1);
               sprintf(mes,"stream %d\n", achan);
                if (pdat[achan].streamsock ) {
                  pdat[achan].status |= STREAMING;
                  pdat[achan].status |= SOCKETOPEN;
                }
              if ( socketcmdsend(pdat[achan].streamsock, mes,80) < 0 )
              {
                fprintf(stderr,"PLOT: streamport mesg cannot be sent %d\n", achan); 
              }
              else
              {
#ifdef DEBUG
                fprintf(stderr,"PLOT: mesg sent for chan %d\n", achan); 
#endif
              }
          }
          else
          {
            fprintf(stderr,"PLOT: already ploting chan\n %d", achan);
          }
        }
        if (cmd[i] == 'e' )
        {
          // end the channel here
          if ( ( pdat[achan].status & STREAMING ) )
          {
            pdat[achan].status &= ~STREAMING ;
            close(pdat[achan].streamsock);
            pdat[achan].status &= ~SOCKETOPEN ;
          }
          else
          {
            fprintf(stderr,"PLOT: not ploting chan\n %d", achan);
          }
        }
      }

   }//while loop

}

void PlotInput(char *argv)
{
    if (prooption == 1 ) PlotInput1(argv); 
    if (prooption == 2 ) PlotInput2(argv); 
    if (prooption == 3 ) PlotInput3(argv); 
}



int SetDataMemory(int achan)
{
char str[181] ;
int i ;
int retval ;

         Rclient.datasize = (int)4*(PLOTTIMESPAN*60)/Rclient.sampling*1000 ;
         retval = 0 ;

         if ( pdat[achan].dataptr == NULL )
         {
            pdat[achan].dataptr = (float *)calloc(Rclient.datasize/4,4);

             if (pdat[achan].dataptr == NULL)
             {
              sprintf(str,"calloc dataptr error %d\n", achan);
              logmessage("MXONLINE", str); exit(0);
              retval = -1 ;
             }
         }
         if (pdat[achan].timeptr == NULL )
         {
            pdat[achan].timeptr = (float *)calloc(Rclient.datasize/4,4);
             if (pdat[achan].timeptr == NULL)
             {
              sprintf(str,"calloc timeptr error %d\n", achan);
              logmessage("MXONLINE", str); exit(0);
              retval = -2 ;
             }
         }


 return retval ;
}


int changebuffer(void * buff)
{
int size ;
void *t ;
       size =  Rclient.datasize ;
      if (Rclient.buffersize < Rclient.datasize )
      {
       t = realloc( buff, (int)(Rclient.datasize*1.5));
      }
       plotpar.readjust = 1 ;
return plotpar.readjust ;
}

int changebuffer1(int pnchan, int *pchannels )
{
int i ;
int achan ;

        Rclient.datasize = (int)4*(20*60)/Rclient.sampling*1000 ;
       for ( i =0 ; i < pnchan ; i++)
       {

             achan = pchannels[i] ;
         if ( pdat[achan].dataptr != NULL )
         {
            free(pdat[achan].dataptr);
            pdat[achan].dataptr = (float *)calloc(Rclient.datasize/4,4);

             if (pdat[achan].dataptr == NULL)
             {
              fprintf(stderr,"PLOT: calloc dataptr error %d\n", achan); exit(0);
             }
         }
         if (pdat[achan].timeptr != NULL )
         {
            free(pdat[achan].timeptr);
            pdat[achan].timeptr = (float *)calloc(Rclient.datasize/4,4);
             if (pdat[achan].timeptr == NULL)
             {
              fprintf(stderr,"PLOT: calloc timeptr error %d\n", achan); exit(0);
             }
         }
       }

       plotpar.readjust = 1 ;

 return plotpar.readjust ;
}


int gettimeaxis(struct timeval btime, float *xtime, int offset)
{
long int usec, busec ;
long int basetime ;
int i ;
struct tm *tmbtime ;
struct timeval stime ;

               if (offset == 0 ) return 0 ;
               basetime = btime.tv_sec - btime.tv_sec%2 ;
               btime.tv_sec -= basetime ;

                 busec = btime.tv_sec*1000*1000 + btime.tv_usec ;


              for (i= 0 ; i < Rclient.datasize/4 ; i++ )
              {

                 usec = busec + (int)Rclient.sampling*1000*(i-offset) ;

                 stime.tv_usec = usec%(1000*1000);
                 stime.tv_sec = (usec - stime.tv_usec)/(1000*1000);
                 stime.tv_sec += basetime ;


                     tmbtime = localtime(&stime.tv_sec);
                  xtime[i] =
                       tmbtime->tm_hour*1.0 +
                       tmbtime->tm_min/60.0 +
                       (tmbtime->tm_sec + stime.tv_usec/1.E6)/60.0/60.0;

              }
               btime.tv_sec += basetime ;

}


/* plots the data if the data is ready */
void PlotMain(char *argv)
{
int pnchan ;
int pchannels[MAXCHAN];
int i ;

     while(1)
     {

              //get pchannels and pnchan  again
                 pnchan = 0 ;
                for ( i =0 ; i < MAXCHAN ; i++)
                {
                  if ( pdat[i].status & STREAMING )
                  {
                     pchannels[pnchan] = i ;
                     pnchan++ ;
                  }
                }


       if( Rclient.dataready == 1 )
       {
#ifdef DEBUG
             fprintf(stderr,"data ready detected\n");
#endif
            Rclient.dataready = 0 ;
            scaledata(pnchan,pchannels);
            if  ( prooption == 1 )
            {
#ifdef DEBUG
              fprintf(stderr,"plotgnu called pnchan %d", pnchan);
#endif
              plotgnu(pnchan, pchannels);
            }
            if  ( prooption == 3 )
            {
#ifdef DEBUG
              fprintf(stderr,"writepipe called pnchan %d", pnchan);
#endif
             writepipe(pnchan, pchannels);
            }

            if  ( prooption == 2 )
            {
#ifdef DEBUG
              fprintf(stderr,"writeonline called pnchan %d", pnchan);
#endif
              writeonline( pnchan, pchannels);
#ifdef DEBUG
              fprintf(stderr,"data written %d\n", pnchan);
#endif
            }
 
       }

     }
}


int getplotchannels( int *pchannels )
{
int pnchan, j ;

               pnchan = 0 ;
             for (j= 0 ; j < MAXCHAN; j++ )
             {
                if ( pdat[j].status & STREAMING )
                {
                   pchannels[pnchan] = j ;
                   pnchan++ ;
                }
             }
return pnchan ;
}


/* This routine sets the variable dataready  */
/* so that the plot thread is activated      */
int triggerplot(int pnchan, int *pchannels, struct timeval *pstime)
{
int j ;
int achan ;
float t1, t2, ti, tw ;
struct timeval curt ;
struct timezone tz ;
float tdiff ;

             for (j= 0 ; j < pnchan; j++ )
             {
              achan = pchannels[j] ;
              gettimeaxis(pstime[j],  pdat[achan].timeptr, pdat[achan].offset) ;
             }
#ifdef DEBUG
              fprintf(stderr, "gettimeaxis over pnchan %d\n", pnchan);
#endif

             /* tell the plotter to plot                                        */
             /* plot if the data can be written and read by datarate speed      */ 
             /* and the data is "timinterval" seconds newer                     */ 
             /* or the (new_data_size / 2.5MBytes/s) is  smaller than above     */
             
             ti = ( Rclient.datasize*pnchan*3.0*2.0)/(plotpar.drate*8*1024*1024);
             t1 = pdat[0].etime ;
             t2 = pdat[0].timeptr[Rclient.datasize/4 - 1];
             if(t2 < t1 ) t2 += 24.0 ;
              tw = (t2 - t1)*60.0*60.0 ;

              gettimeofday(&curt, &tz);
              tdiff  = (curt.tv_sec - Rclient.writetime.tv_sec);
             
#ifdef DEBUG
              fprintf(stderr, "TW : TI : TDIFF : TIN %f %f %f %f\n", tw, ti, tdiff, plotpar.timeinterval);
#endif 
             if ( ( tw > ti ) && ( tdiff > plotpar.timeinterval ) )
             {
                Rclient.dataready = 1 ;
                Rclient.writetime =  curt;
             }

             /* set the end time of all the chans to the new value */
             for (j= 0 ; j < pnchan; j++ )
             {
              achan = pchannels[j] ;
              pdat[achan].etime = pdat[achan].timeptr[Rclient.datasize/4 - 1];
             }

}



void PlotReceive(void *argv)
{
char str[181];
int    asock ;
extern int errno ;
int    ret  ;
struct pollfd ufds[MAXCHAN];
int i, j, rec ;
int  pnchan ;
int  foundtime ;
int  pchannels[MAXCHAN] ;
int  pnrec[MAXCHAN] ;
int  rem ;
int  maxbyte;
int  maxbytechan;
int  offset;
int  achan ;
int  jk;
float *dp ;
int fillsize ;

struct timeval  pstime[MAXCHAN] ;

struct timeval  pretime[MAXCHAN] ;
int readjust ;
int gotchannel;


      sprintf(str,"buffer size (MB) %f \n", Rclient.buffersize/1024.0/1024*4.0);
      logmessage("MXONLINE", str);

      if (Rclient.buffer == NULL)
      Rclient.buffer = (float *)malloc(Rclient.buffersize); 

      if (!Rclient.buffer)
      {
       fprintf(stderr,"PLOT: bufffer error \n"); exit(0); 
      }

      Rclient.stopplot = 0 ;
     while( ! Rclient.stopplot )
     {
          //get pchannels and pnchan 
               pnchan = 0 ;
              for ( i =0 ; i < MAXCHAN ; i++)
              {
                if ( ( pdat[i].status & STREAMING ) &&
                       ( pdat[i].status & SOCKETOPEN )
                   )
                {
                   pchannels[pnchan] = i ;
                   pnchan++ ;
                }
              }

           for ( i =0 ; i < pnchan ; i++)
           {
             achan = pchannels[i] ; 
            ufds[i].fd = pdat[achan].streamsock ;
            ufds[i].events = POLLIN | POLLOUT | POLLERR ;
            ufds[i].revents =  0;
           }

#ifdef DEBUG
//         fprintf(stderr, "trying to read total chans %d and %d \n", pnchan, Rserver.RecordNchan );
#endif
         ret = poll(ufds, pnchan, 1000);
         if (ret>0)
         {
               maxbyte = 0 ;
               maxbytechan = 0 ;
               gotchannel = 0 ;
             for ( i =0 ; i < pnchan ; i++)
             {
               pnrec[i] = 0 ;
                 achan = pchannels[i] ;

//                          fprintf(stderr,"Receive: checking channel %d \n", achan); 
              if( ufds[achan].revents & POLLERR) 
              {
                asock = pdat[achan].streamsock ;
                close(asock);
                pdat[achan].status &= ~SOCKETOPEN ;
                pdat[achan].status &= ~STREAMING ;
                Rserver.RecordCchan++ ;
                fprintf(stderr,"POLLERR on channel %d\n", achan);
              }

              if( (ufds[i].revents & POLLOUT)
                  && ( !(ufds[achan].revents & POLLERR) )
                )
              {
                achan = pchannels[i] ;
                asock = pdat[achan].streamsock ;
                fillsize = pdat[achan].fillsize ;
//                rec = socket_read_data(asock, (char *)Rclient.buffer, Rclient.buffersize);
//                          fprintf(stderr,"Receive: waiting to receive  %d \n", achan); 
                 rec = recv(asock, (char *)Rclient.buffer, Rclient.buffersize,  MSG_DONTWAIT) ;

#ifdef DEBUG
//                fprintf(stderr,"data received for chan %d %d\n", achan,rec );
#endif

                 if(rec>0)
                 {
                   if( !(ufds[achan].revents & POLLERR) )
                   {
                        
                          foundtime = 0;
                      jk=rec-1;
                      while( jk >= 0)
                      {

                        if( (strncmp((char*)Rclient.buffer+jk,"TIME",4) == 0)) 
                        {
                           //read the time first 
                          memcpy(&pstime[i], ((char *)Rclient.buffer)+jk+4,8);
                          memcpy(&Rclient.sampling, ((char *)Rclient.buffer)+jk+12,4);

                          readjust = plotpar.readjust ; 
                          if (readjust == 0 )
                          {
                             changebuffer((void *) Rclient.buffer);
                             readjust = 1 ;
                          }
                          pretime[achan] = pstime[i] ;
#ifdef DEBUG
                          fprintf(stderr,"TIME received %d %d %d %f\n", 
                                achan, pstime[i].tv_sec, pstime[i].tv_usec, Rclient.sampling);
#endif
                           rec = rec-16 ;
                           memmove( ((char *)Rclient.buffer)+jk, 
                                    ((char *)Rclient.buffer)+jk+16,rec-jk);
                             
                           foundtime = 1;
                        }
                       jk--;
                      }

                       SetDataMemory(achan);
                      if ( ( foundtime == 0 ) && rec != 0 ) pstime[i] = pretime[achan] ;
 
                      pnrec[i] = rec ;
                      rem = (Rclient.datasize - rec );
                      pdat[achan].offset =  rem/ 4 ;
       
                        dp = pdat[achan].dataptr ;
                        if( dp)
                        {
                         memmove(dp,dp+rec/4, rem);
                         memmove(dp+rem/4,Rclient.buffer,rec);
                         fillsize += rec/4 ;
                        }
                      if (rec > maxbyte)
                      {
                        maxbyte = rec ;
                        maxbytechan = achan ;
                      }

                   pdat[achan].fillsize =
                       fillsize < Rclient.datasize/4 ? fillsize : Rclient.datasize/4 ;
                   }
 
                  gotchannel++ ;
                }

                if (rec < 0 )
                {
                  /* this following loop finally works */
                  /* close the sock and reset the server */
                  /* do not free the pointers since the other threads might be using */
                  /* they are freed when the SetDataMemory is called again */
                  /* Note: But the memory is not freed when the program quits    */
                  if (errno != EAGAIN)
                  {
                  // close the sock //
                   close(asock);
                   pdat[achan].status &= ~SOCKETOPEN ;
                   pdat[achan].status &= ~STREAMING ;
                  //if (pdat[achan].dataptr != NULL ) free(pdat[achan].dataptr);
                  //if (pdat[achan].timeptr != NULL ) free(pdat[achan].timeptr);
                   Rserver.RecordCchan++ ;
                   pdat[achan].fillsize = 0 ;
                  }
                }
                if (rec == 0 )
                {
#ifdef DEBUG                  
//                 fprintf(stderr, "rec value is zero");
//                 fprintf(stderr, "errorno %d\n", errno);
#endif 
                }

              }/*channel pollout ready  end*/

             }/* for chan loops ends here */

                 //get pchannels and pnchan  again
                 pnchan = 0 ;
                for ( i =0 ; i < MAXCHAN ; i++)
                {
                  if ( pdat[i].status & STREAMING )
                  {
                     pchannels[pnchan] = i ;
                     pnchan++ ;
                  }
                }
 
               /* all the live channels are read */
               /* send dataready to plot thread */
               if (gotchannel == pnchan )
               {
                 triggerplot(pnchan, pchannels, pstime);
               }


         } //poll ret
         if (ret<=0)
         { 
#ifdef DEBUG                  
                 fprintf(stderr, "poll ret %d errorno %d\n", ret, errno);
#endif 
         } //poll ret

     }//while Rclient.stopplot loop

}


void RecordServer(char *argv)
{
char str[81];
char message[281];
int ret ;
int  rec ;
int redo ;
int checksock ;
struct pollfd ufds;
int ct, ival ;
int schan ;
int i ;
int go, trec , tbytes;

struct _ttmes {
      int nchan ; 
   }  *pttmes ;

char *c ;


    Rserver.RecordServerSock = -1 ;
  while(1)
  {
    if (Rserver.RecordServerSock == -1)
    {
      
      while(Rserver.RecordServerSock == -1)
      {
       Rserver.RecordServerSock = socketcmdopenloop("getonline", recordcmdport);
       if (Rserver.RecordServerSock == -1) sleep(10);
      }
       ret = send(Rserver.RecordServerSock, "nchan", 5, 0);
#ifdef DEBUG
       fprintf(stderr,"sent nchan message\n");
#endif
       rec = -1 ;
       memset(message,0,80);
       
       
            trec = 0 ;
            go = 1 ;
            tbytes = -1 ;
            c = (char *) calloc(1, sizeof( struct _ttmes)) ;
         while (go)
         {
              /* maximum expected is _ttmes structure */
            rec = recv(Rserver.RecordServerSock, c+trec, sizeof(struct _ttmes), 0) ;
            if (rec > 0 )
            {
                trec += rec ;
                if (trec == sizeof(struct _ttmes) ) go = 0 ;
            }

         sprintf(str,"received bytes %d", trec);
         logmessage("MXONLINE", str);
         } /* read all the bytes */
         logmessage("MXONLINE", "server ok ");
             pttmes = (struct _ttmes *) c ;
             schan =  pttmes->nchan ;

              for( i = 0 ; i < schan ; i++ )
              {
                   Rserver.RecordChannels[i] =  i ;
              }
          /* set the nchan to the saved value */
          /* this makes it thread safe */
          Rserver.RecordNchan = schan ;
          Rserver.RecordCchan = 0 ;
          /* set the following to start opening data port */
          if (schan) { Rserver.openagain = 1 ; Rserver.filereset = 1; }
          free(c);
         sprintf(str,"server nchan %d", schan);
         logmessage("MXONLINE", str);
    }

    if (Rserver.RecordServerSock != -1)
    {
    /* socket is open so wait for it to close */
    /* if closed redo opening the sock */
    /* and set RecordNchan to zero so that the threadPlotReceive resets */
          checksock = 1 ;
          while(checksock )
          {
//            ufds.fd = Rserver.RecordServerSock;
//            ufds.events = POLLIN | POLLOUT | POLLERR ;
//            ufds.revents =  0;
//            ret = poll(&ufds, 1, 1000);
//              ret = send(Rserver.RecordServerSock, "", 0, 0);
//              if ( ret < 0 ) 
              if (Rserver.RecordCchan == Rserver.RecordNchan ) 
              {
                checksock   = 0 ;
                Rserver.RecordNchan = 0;
                Rserver.filereset = 1;
                close(Rserver.RecordServerSock);
                /* this resets the server */
                Rserver.RecordServerSock = -1;
                logmessage("MXONLINE", "server shutdown received; restart");
              }
          }
    }

  }/* while sock loops ends here */

}

int  initthreadPlotInput()
{
int ret ;

         ret = pthread_create( &threadPlotInput, NULL, (void *)&PlotInput, NULL);
         if (ret ) {
          fprintf(stderr,"PLOT Could not create PlotInput thread\n");
         }

 return 0 ;
}


int  initthreadPlotMain()
{
int ret ;

         ret = pthread_create( &threadPlotMain, NULL, (void *)&PlotMain, NULL);
         if (ret ) {
          fprintf(stderr,"PLOT Could not create PlotMain thread\n");
         }

 return 0 ;
}


int  initthreadPlotReceive()
{
int ret ;

         ret = pthread_create( &threadPlotReceive, NULL, (void *)&PlotReceive, NULL);
         if (ret ) {
          fprintf(stderr,"PLOT Could not create PlotReceive thread\n");
         }
 return 0 ;
}


int  initthreadRecordServer()
{
int ret ;

         ret = pthread_create( &threadRecordServer, NULL, (void *)&RecordServer, NULL);
         if (ret ) {
          fprintf(stderr,"PLOT Could not create RecordServer thread\n");
         }
 return 0 ;
}

int makedaemon1()
{
// make the process a daemon  part 1
// create a child and destroy the parent
pid_t childpid, childsid;


        childpid = fork();
        if (childpid < 0) {
              fprintf(stderr, "ACQ:  fork error\n");
                exit(EXIT_FAILURE);
        }
        if (childpid > 0) {
         // this is the parent & destroy the parent 
           exit(EXIT_SUCCESS);
        }
        else
        {
           childsid = setsid();
           if (childsid < 0)
           {
            //session not possible exit 
              fprintf(stderr, "ACQ:  SID could not  be obtained\n");
              exit(EXIT_FAILURE);
           }
          // go ahead 
        }

return 0 ;
}
int makedaemon2()
{
// make the process a daemon  part 2
// handle all the file descriptors 
   fclose (stdin);
   fclose (stdout);
   fclose (stderr);

}


int initfiles()
{
// close the sdterr and stdout
// open the log file
char fname[581] ;
struct timeval curt ;
struct timezone tz;
char tstr[25] ;


    sprintf(fname,"/tmp/%s", logfilename);
    if (prooption == 2 ) sprintf(fname,"%s/%s", onlinedir,logfilename);
    umask(~filemode);
    getonlinelogfp  = fopen(fname, "a");
    if (!getonlinelogfp)
    {
      fprintf(stderr, " Error opening log file %s", fname);
      exit(0);
    }

     gettimeofday(&curt, &tz);
     strncpy(tstr,ctime(&curt.tv_sec),24);
     tstr[24] = '\0' ;
     fprintf(getonlinelogfp,"\n********* %s **********\n", tstr);
     fflush(getonlinelogfp);
}


int initplot(int argc, char *argv[])
{
int i ;
        prooption = 0 ;
        strncpy(proname, argv[0], 80) ;
        initfiles();
                                                                                
        if  (strstr(proname,"mxgplot") != NULL ) { prooption = 1 ; }
        if  (strstr(proname,"mxdonline") != NULL ) { prooption = 2 ; }
        if  (strstr(proname,"mxponline") != NULL ) { prooption = 3 ; }

        if ( prooption == 0 )
        {
         fprintf(stderr, "Calling name of the program is unknown... \n"); exit(0);
        }



        /* assumes 10millisecond sampling */
        /* it might overflow if the record sends more data */
        Rclient.buffersize = PLOTTIMESPAN*60/(0.01)*4 ;
        Rclient.datasize =  -1;
        Rclient.buffer = NULL ;
        Rserver.openagain = 0 ;
        Rserver.filereset = 1 ;

       for ( i =0 ; i < MAXCHAN ; i++)
       {
          pdat[i].dataptr = NULL ;
          pdat[i].timeptr = NULL ;
          pdat[i].fp = NULL ;
          pdat[i].fillsize = 0 ;
       }

       if  ( prooption != 2 )
       {
       parseoptionsplot(argc, argv);
#ifdef DEBUG
       fprintf(stderr,"arguments ok...\n");
#endif 
       }


       if  ( prooption == 1 )
       {
#ifdef DEBUG
       fprintf(stderr,"opening gnuplot...\n");
#endif 
        gnuplotopen();
#ifdef DEBUG
       fprintf(stderr,"gnuplot success...\n");
#endif 
       }

       if  ( prooption == 2 )
       {
       /* this is mxdonline */
       /* daemonise the process */
        makedaemon1();
        makedaemon2();
       }
}




int main(int argc,  char *argv[])
{

       initplot(argc, argv);
       mtrace();

       initthreadRecordServer();
#ifdef DEBUG
       fprintf(stderr,"threadRecordServer ok ...\n");
#endif 

       initthreadPlotInput();
#ifdef DEBUG
       fprintf(stderr,"threadPlotInput ok ...\n");
#endif 

       initthreadPlotReceive();
#ifdef DEBUG
       fprintf(stderr,"threadPlotReceive ok ...\n");
#endif 
       initthreadPlotMain();
#ifdef DEBUG
       fprintf(stderr,"threadPlotMain ok ...\n");
#endif 

       pthread_join( threadPlotMain, NULL);
       pthread_join( threadPlotReceive, NULL);
       pthread_join( threadPlotInput, NULL);
       pthread_join( threadRecordServer, NULL);
       fprintf(stderr,"PLOT: to end here \n");
}


