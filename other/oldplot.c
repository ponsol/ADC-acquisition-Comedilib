// Usage:  plot -c  channel1 channel2 ....   
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <comedilib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <math.h>

#include "acqcommon.h"
#include "acq.h"

#define NBLOCK  6
#define MAXCHANNELS  8

#define ERROR -1 

struct timeval getscantime(int scan, struct tm *tmtime)
{
char *st ;
struct timeval  itime ;
struct tm  *tmt ;
int nchan ;
long int usec ;
long int basetime ;

          nchan = ahead->nchan ;
         
         itime = ahead->starttime ;
         basetime = itime.tv_sec - itime.tv_sec%2 ;
         itime.tv_sec -= basetime ;
         usec = itime.tv_sec*1000*1000 + itime.tv_usec ;

          if (scan == (BUFFERPAGES/nchan -1) )
          {
             usec -= (1024*ahead->sampling*1000) ;
          }
          else
          {
            usec += scan*1024*ahead->sampling*1000 ;
          }

          itime.tv_usec = usec%(1000*1000) ;
          itime.tv_sec =  (usec - itime.tv_usec)/(1000*1000) ;
         
          itime.tv_sec += basetime ;
         st = ctime(&itime.tv_sec) ;
         tmt =  gmtime(&itime.tv_sec);
         //fprintf(stderr,"%s", st);

         if( tmtime != 0 )
         { *tmtime = *tmt ; }

return  itime ;
}

int getfilename(char *fname, int ichan, int scan )
{
int rchan ;
struct tm  tmtime ;

           getscantime(scan, &tmtime);
           rchan = ahead->channels[ichan] ;
           sprintf(fname,"scan-%4d-%02d-%02d-%02d%02d.chan_%d", 
                                    tmtime.tm_year+1900,
                                    tmtime.tm_mon,
                                    tmtime.tm_mday,
                                    tmtime.tm_hour,
                                    tmtime.tm_min,
                                    rchan
                                    );

          puts(fname);

}

int initmemory()
{
         shmid1 = shmget(shmkey1, totalshmsize, 0) ;
         shmid2 = shmget(shmkey2, shmparsize, 0) ;

         if( (shmid1 ==-1 ) )
         {
           fprintf(stderr,"SCAN: Could not see shared memory buffer \n"); 
           exit(0);
         }

         if( (shmid2 ==-1 ) )
         {
           fprintf(stderr,"SCAN: Could not see shared memory par\n"); 
           exit(0);
         }

        abuf = (acqbuf *) shmat(shmid1, NULL, SHM_RDONLY);

        if ( (abuf == NULL )  )
        {
          fprintf(stderr,"SCAN:  buffer 1 attachement error %d %d\n",
                             abuf, shmid1);
          exit(0);
        }

        ahead=NULL ;
        ahead = (acqhead *)shmat(shmid2, NULL, SHM_RDONLY);

        if ( (ahead == NULL )  )
        {
          fprintf(stderr,"SCAN:  buffer 2 attachement error %d %d\n",
                            ahead, shmid2);
          exit(0);
        }
}

struct timeval getdata(int scanmin, int scanmax, int ichan, float *data)
{
double     volts[1024] ;
float  rvolts[1024] ;
int        i, j ;
int  scanno ; 
struct tm tmtime ;
struct timeval  scantime ;
sampl_t    *dataptr ;


             scanno = 0 ; 
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
                  *(data+i) = rvolts[i] ;
                }
            }

 return scantime ;
}


int plot(int pnchan, int *pchannels)
{
char filename[181] ;
int   i ;
int   ichan ;
int    lscan, lscanold ;
int    scanmax, scanmin ;
int    startyes ;
float  data[MAXCHANNELS][NBLOCK*1024] ;
double xtime[NBLOCK*1024] ;
struct timeval tbtime ;
struct tm  *tmbtime ;
struct timeval begintime ;
struct timeval stime[NBLOCK*1024] ;
int j ;
FILE *gp ;
struct timeval  scantime ;
long int basetime ;
long int usec ;
int xtimehr ;
int xtimemin ;
int xtimesec ;
double xx ;



            gp = popen("gnuplot", "w");
            //fprintf(gp,"set term vttek\n") ;
            fprintf(gp,"set nokey\n") ;
            //fprintf(gp,"set xdata time\n") ;
            //fprintf(gp,"set timefmt \"%H:%M\"\n") ;
            //fprintf(gp,"set format x \"%H:%M\"\n") ;
            //fprintf(gp,"set xla \"(hour:min)\"\n") ;
            fprintf(gp,"set xla \"hour\"\n") ;
            fprintf(gp,"set yla \"counts\"\n") ;
            fprintf(gp,"set tmargin -1 \n") ;
            fprintf(gp,"set xzeroaxis lt -2 lw 2.000   \n") ;
            fprintf(gp,"set x2zeroaxis lt -2 lw 2.000   \n") ;
            fprintf(gp,"set yzeroaxis lt -2 lw 2.000   \n") ;
            fprintf(gp,"set yzeroaxis lt -2 lw 2.000   \n") ;
            //fprintf(gp,"set size ratio 0.4\n") ;
            //fprintf(gp,"set timefmt \"%s\"\n") ;


           gettimeofday(&tbtime,0);
           ctime(&tbtime.tv_sec);
           tmbtime = gmtime(&tbtime.tv_sec);
           tmbtime->tm_min = 0 ;
           tmbtime->tm_hour = 0 ;
           tmbtime->tm_sec  = 0 ;
           begintime.tv_sec = mktime(tmbtime);
           begintime.tv_usec = 0 ;

       lscanold =  abuf->stat ;
       startyes = 0 ;

       while(1) 
       {
           lscan = abuf->stat ;
          if (lscan != lscanold )
          {
                    scanmin = lscanold ;
                  scanmax = lscan ;
               if ( lscanold > lscan ) 
               {
                  scanmax = lscan+(BUFFERPAGES/ahead->nchan);
               }

              if (startyes == 0 )
              {
                 startyes=1 ;
                 memset(&data[0][0],0, NBLOCK*1024*4);
              }

           
              for (i= 0 ; i < pnchan; i++ )
              {
               ichan = pchannels[i] ;
               memmove(&data[ichan][0], &data[ichan][1024], (NBLOCK-1)*1024*4);
               scantime = getdata(scanmin, scanmax, ichan, 
                                 &data[ichan][(NBLOCK-1)*1024]);
              }



               basetime = scantime.tv_sec - scantime.tv_sec%2 ;
               scantime.tv_sec -= basetime ;

              for (i= 0 ; i < NBLOCK*1024; i++ )
              {

                 usec = scantime.tv_sec*1000*1000 + 
                         scantime.tv_usec -
                         ((NBLOCK*1024-i)*(ahead->sampling)*1000) ;

                 stime[i].tv_usec = usec%(1000*1000);
                 stime[i].tv_sec = (usec - stime[i].tv_usec)/(1000*1000);

                 stime[i].tv_sec += basetime ;
                 //stime[i].tv_sec += (basetime-begintime.tv_sec) ;

                  xtime[i] = - begintime.tv_sec ;
                  xtime[i] = stime[i].tv_sec + stime[i].tv_usec/1.E6 ;
                  xtime[i] = xtime[i] /(60 * 60) ;

                     tmbtime = localtime(&stime[i].tv_sec);
                  xtime[i] = 
                       tmbtime->tm_hour*1.0 +
                       tmbtime->tm_min/60.0 +
                       (tmbtime->tm_sec + stime[i].tv_usec/1.E6)/60.0/60.0;
                
              }
               scantime.tv_sec += basetime ;


              for (i=0; i < pnchan; i++)
              {
               if (i==0 ) fprintf(gp,"plot \"-\" u 1:%d  w l lw 2",i+2);
               else
               fprintf(gp,", \"-\" u 1:%d  w l lw 2",i+2);
              }
               fprintf(gp,"\n");

              for (i= 0 ; i < NBLOCK*1024; i++ )
              {
                //  xtimehr = (int)floor( xtime[i] );
                 // xtimemin = (int)floor( (xtime[i] - xtimehr)*60.0) ;
                //  xtimesec = (int)floor(((xtime[i] -xtimehr)*60 - xtimemin)*60) ;
              //  fprintf(gp,"%d:%d:%d %lf ", xtimehr, xtimemin, xtimesec, xtime[i] ) ;
               // fprintf(stdout,"%d:%d:%d %lf ", xtimehr, xtimemin, xtimesec, xtime[i] ) ;
                //fprintf(gp,"%ld.%ld ", stime[i].tv_sec,stime[i].tv_usec ) ;
                //fprintf(stdout,"%ld.%ld ", stime[i].tv_sec,stime[i].tv_usec ) ;
                 //fprintf(gp,"%f:%f", xtimehr, xtimemin) ;
                 fprintf(gp,"%lf ", xtime[i] ) ;
                 //fprintf(stdout,"%lf ", xtime[i] ) ;
                for (j= 0 ; j < pnchan; j++ )
                {
                    ichan = pchannels[j];
                 fprintf(gp,"  %f ", data[ichan][i] ) ;
                }
                 fprintf(gp,"\n" );
                // fprintf(stdout,"\n" );
              }
              fprintf(gp,"e\n");
            //fprintf(stderr,"%s", ctime(&begintime.tv_sec));
            //fprintf(stderr,"%s", ctime(&stime[0].tv_sec));
            //fprintf(stderr,"%s", asctime(tmbtime));


            lscanold = lscan ;
           
          }

       }



return 0 ;
}


int main(int argc,  char *argv[])
{
int  ichan;
int  optionfound ;
int  ival ;
int i, j ;
char *c ;
int  ct  ;
int  pnchan ;
int  pchannels[10];

          
          
                 j = 0 ;
                 optionfound = 0 ; 
           for ( i =1 ; i < argc ; i++)
           {

              if (optionfound ) 
              {  
                if ( ct = sscanf(argv[i],"%d", &ival) )
                {
                    pchannels[j]  = ival ;
                    j++ ;
                }
              }  
              if ( strstr(argv[i],"-c") != NULL )
              {
                 if (sscanf(argv[i]+2,"%d", &ival) == 1 )
                 {
                    pchannels[j]  = ival ;
                    j++ ;
                 }
                 optionfound = 1 ; 
              }
           }

           pnchan = j ;

          if (argc == 1)
          { 
            // assume channel 0 
            pchannels[0]  = 0 ;
            pnchan = 1 ;
          } 
          else
          {
            if (optionfound == 0 ) 
            { 
              fprintf(stderr,"PLOT: bad option\n"); exit(0); 
            } 
          }
        initmemory();

      for ( i =0 ; i < pnchan ; i++)
      {
         if ( i > ahead->nchan )
         {
           fprintf(stderr, "PLOT: channel %d does not exist\n", ichan );
            return  ERROR;
         }
      }

      plot(pnchan, pchannels);

}
