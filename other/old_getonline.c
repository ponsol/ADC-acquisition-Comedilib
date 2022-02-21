// Usage:  plot -c  channel1 channel2 ....   


#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>
//#include <comedilib.h>

#include <sys/poll.h>
#include <pthread.h>

//#include <sys/ipc.h>
//#include <sys/shm.h>
#include <math.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


//#define DEBUG

#include "server.h"
#include "cmd.h"
#include "cmd.c"
#include "plotgnu.c"
#include "writeonline.c"


#define  SOCKETOPEN  2
#define  STREAMING  1
#define  MAXCHAN 64
#define  STOPPLOT 1
char onlinedir[] = "/database/actual" ;

static mode_t fm = S_IWUSR| S_IRUSR | S_IWGRP| S_IRGRP | S_IROTH | S_IWOTH ;

char   proname[81] ;
int   prooption ;

// the following to hold the plotting channels
int    tpnchan ;
int    tpchannels[MAXCHAN];

float  sfactor=5 ;
int    nrunmean = 1 ;
int    readjust = 0 ;
int    plotlock = 0 ;
float    zeroref = 0.0 ;

int    tpnchan ;
int    tpchannels[MAXCHAN];

int      stopplot ;

pthread_t   threadplotinput;
pthread_t   threadplotmain;



typedef struct  {
           FILE  *fp ;
           int  fd ;
           int   streamsock ;
           int   fillsize ;
           int   status ;
           int   offset ;
           float *dataptr ;
           float *timeptr ;
           float *lstptr ;
           float rms ;
           float mean ;
           float totmean ;
           float totrms ;
           float xmin, xmax, ymin,ymax ;
          } _pdatstruct ;


_pdatstruct  pdat[MAXCHAN] ;
int      datasize ;
int      buffersize ;
float    *buffer ;
float    *timeptr ;
float    sampling ;


FILE *gp ;

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
                    tpchannels[j]  = ival ;
                    j++ ;
                }
              }  
              if ( strstr(argv[i],"-c") != NULL )
              {
                 optionfound = 1 ; 
                 if ( sscanf(argv[i]+2,"%d", &ival) == 1 )
                 {
                  tpchannels[j] = ival ; j++ ; 
                 }
              }
           }

           tpnchan = j ;


          if (argc == 1)
          { 
            // assume channel 0 
            tpchannels[0]  = 0 ;
            tpnchan = 1 ;
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
int getplotinputevent( int *pchannels, char *cmd )
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
    if (tpnchan != -1 )
    {
         pnchan = tpnchan ; 
       for ( i= 0; i < pnchan ; i++ )
       {
         pchannels[i] = tpchannels[i] ;
         cmd[i] = 's' ;
       }
        tpnchan = -1 ;
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
               if (ct == 2 ) sfactor = fval ;
               pnchan = -2 ;
             }
             if( c =='r')
             {
               ct =sscanf(str,"%c %f", &c, &fval);
               if (ct == 2 ) nrunmean = (int)fval ;
               pnchan = -3 ;
             }
             if( c =='o')
             {
               ct =sscanf(str,"%c %f", &c, &fval);
               if (ct == 2 ) zeroref = fval ;
               pnchan = -4 ;
             }

      }

 return pnchan ;
}

void plotinput(char *argv)
{
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


   while ( stopplot != STOPPLOT )
   {
    
      pnchan = getplotinputevent(pchannels, cmd);
#ifdef DEBUG
      fprintf(stderr,"PLOT: recevied chan %d %c\n", pnchan, cmd[0]);
#endif

       if(pnchan == -1 ) stopplot = STOPPLOT ;

        if (pnchan <  0 )
        {
          tcn  = getplotchannels(tcnls);
          plotgnu(NULL, tcn, tcnls);
        }

      for ( i =0 ; i < pnchan ; i++)
      {
             achan = pchannels[i] ;
        if (cmd[i] == 's' )
        {
          // start the channel here
          if ( !( pdat[achan].status & SOCKETOPEN ) )
          {
          
             if (pdat[achan].dataptr == NULL)
              pdat[achan].dataptr = (float *)calloc(datasize/4,4); 
             if (pdat[achan].dataptr == NULL)
             {
              fprintf(stderr,"PLOT: calloc dataptr error %d\n", achan); exit(0); 
             }
             if (pdat[achan].timeptr == NULL)
              pdat[achan].timeptr = (float *)calloc(datasize/4,4); 
             if (pdat[achan].timeptr == NULL)
             {
              fprintf(stderr,"PLOT: calloc timeptr error %d\n", achan); exit(0); 
             }

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


//ends here close all the socks
   for ( i =0 ; i < MAXCHAN ; i++)
   {
      close(pdat[i].streamsock);
   }

      fprintf(stderr,"PLOT: to end here \n");
      pclose(gp);
      exit(0);

}


int changebuffer(int pnchan, int *pchannels )
{
int i ;
int achan ;
       datasize = 4*(20*60)/sampling*1000 ;
       for ( i =0 ; i < pnchan ; i++)
       {

             achan = pchannels[i] ;
         if ( pdat[achan].dataptr != NULL )
         {
            free(pdat[achan].dataptr);
            pdat[achan].dataptr = (float *)calloc(datasize/4,4);

             if (pdat[achan].dataptr == NULL)
             {
              fprintf(stderr,"PLOT: calloc dataptr error %d\n", achan); exit(0);
             }
         }
         if (pdat[achan].timeptr != NULL )
         {
            free(pdat[achan].timeptr);
            pdat[achan].timeptr = (float *)calloc(datasize/4,4);
             if (pdat[achan].timeptr == NULL)
             {
              fprintf(stderr,"PLOT: calloc timeptr error %d\n", achan); exit(0);
             }
         }
       }

      readjust = 1 ;

 return readjust ;
}


int gettimeaxis(struct timeval btime, float *xtime, int offset)
{
long int usec, busec ;
long int basetime ;
int i ;
struct tm *tmbtime ;
struct timeval stime ;

               basetime = btime.tv_sec - btime.tv_sec%2 ;
               btime.tv_sec -= basetime ;

                 busec = btime.tv_sec*1000*1000 + btime.tv_usec ;


              for (i= 0 ; i < datasize/4 ; i++ )
              {

                 usec = busec + sampling*1000*(i-offset) ;

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

int gnuplotopen()
{

            gp = popen("/usr/bin/gnuplot -geometry 1250x300+10+90", "w");
            fprintf(gp,"set term x11 0\n") ;
            //fprintf(gp,"set multiplot\n") ;
            fprintf(gp,"set key out below Left box \n") ;
            //fprintf(gp,"set xdata time\n") ;
            //fprintf(gp,"set timefmt \"%H:%M\"\n") ;
            //fprintf(gp,"set format x \"%H:%M\"\n") ;
            //fprintf(gp,"set xla \"(hour:min)\"\n") ;
            fprintf(gp,"set xla \"Hour\"\n") ;
            fprintf(gp,"set yla \"Volts\"\n") ;
            fprintf(gp,"set tmargin -1 \n") ;
            fprintf(gp,"set xzeroaxis lt -2 lw 2.000   \n") ;
            fprintf(gp,"set x2zeroaxis lt -2 lw 2.000   \n") ;
            fprintf(gp,"set yzeroaxis lt -2 lw 2.000   \n") ;
            fprintf(gp,"set yzeroaxis lt -2 lw 2.000   \n") ;
            fprintf(gp,"plot [0:1] [-1:1] 0  \n") ;
            fflush(gp);
            //fprintf(gp,"set size ratio 0.4\n") ;
            //fprintf(gp,"set timefmt \"%s\"\n") ;
}

int plotgnu(struct timeval *stime,  int pnchan, int *pchannels)
{
char key[MAXCHAN][281] ;
float doffset[MAXCHAN] ;
int i ;
float xtime[datasize] ;
char popt ;
int j ;
int achan ;
float xpos, ypos ;
float  maxmean, minmean;
float  maxrms, minrms;
float xsmin, xsmax, dx ;
  

            // for (j= 0 ; j < pnchan; j++ )
            // {
            //   achan = pchannels[j] ;
            //  gettimeaxis(stime[j],  pdat[achan].timeptr, pdat[achan].offset) ;
            // }

              if ( plotlock == 1 ) return 0 ;
              plotlock = 1 ;

                 achan = pchannels[0] ;
                maxrms = pdat[achan].totrms;
                minrms = pdat[achan].totrms;
                maxmean = pdat[achan].totmean;
                minmean = pdat[achan].totmean;

                xsmin = pdat[achan].timeptr[0] ;
                xsmax = pdat[achan].timeptr[datasize/4-1] ;
                dx = (xsmax - xsmin)/100.0 ;
                xsmin = xsmin - 2*dx;
                xsmax = xsmax + 2*dx;
                fprintf(gp,"set xr [%f:%f]\n", xsmin, xsmax );

             for (j= 0 ; j < pnchan; j++ )
             {
                 achan = pchannels[j] ;
                if ( pdat[achan].totmean != 0 )
                {
                  if (pdat[achan].totmean > maxmean )
                  {  
                   maxmean = pdat[achan].totmean ; 
                   maxrms = pdat[achan].totrms; 
                  }
                  if (pdat[achan].totmean < minmean )
                  {  
                   minmean = pdat[achan].totmean ; 
                   minrms = pdat[achan].totrms; 
                  }
                }

             }
   
             maxmean = maxmean;

            if (maxrms != 0 )
            {
             fprintf(gp,"set yr [%f:%f]\n", maxmean+zeroref - sfactor*maxrms, 
                                            maxmean+zeroref +sfactor*maxrms);
            }
         
                for (j= 0 ; j < pnchan; j++ )
                {
                 achan = pchannels[j] ;
                 doffset[j] = pdat[achan].totmean - maxmean  ;
//                 sprintf(key[j], "\"Chan:%d Zoom:%4.2f Offset:%g Mean: %f Rms: %f Tmean: %f  Trms: %f \"", 
 //                     achan, sfactor, doffset[j], pdat[achan].mean, 
  //                    pdat[achan].rms, pdat[achan].totmean, 
   //                   pdat[achan].totrms );
//                 sprintf(key[j], "\"Chan:%2d Refoffset:%6.3g  Zoom:%4.2f Offset:%8.3g Mean: %8.3g Rms: %8.3g Tmean:%8.3g Trms:%8.3g\"", 
 //                 achan, zeroref, sfactor, doffset[j], pdat[achan].mean, 
  //                pdat[achan].rms, pdat[achan].totmean, pdat[achan].totrms);
                 sprintf(key[j], "\" Chan:%2d  Refoffset:%6.3g  Zoom:%4.2f Offset:%8.3g Mean: %8.3g Rms: %8.3g \"", 
                  achan, zeroref, sfactor, doffset[j], pdat[achan].mean, 
                  pdat[achan].rms );
//                  printf("%s\n", key[j] );
                }
                 fflush(gp);


             for (j= 0 ; j < pnchan; j++ )
             {
               if (j==0 ) 
               {
                 fprintf(gp,"plot \"-\" u %d:%d title %s w l lw 2",
                                       j+1, pnchan+j+1, key[j]);
               }
               else
               fprintf(gp,", \"-\" u %d:%d title %s w l lw 2",
                                       j+1, pnchan+j+1, key[j]);

             }
             fprintf(gp,"\n" );


   if( pnchan != 0 )
   {
       for (i= 0 ; i < datasize/4; i++ )
       {
                 //fprintf(stdout,"%lf ", xtime[i] ) ;
                for (j= 0 ; j < pnchan; j++ )
                {
                 achan = pchannels[j] ;
                 fprintf(gp,"%lf ", pdat[achan].timeptr[i] ) ;
                }
                for (j= 0 ; j < pnchan; j++ )
                {
                 achan = pchannels[j];
                 fprintf(gp,"  %f ", pdat[achan].dataptr[i]-doffset[j] ) ;
                }
                 fprintf(gp,"\n" );
                // fprintf(stdout,"\n" );
       }

   }
        fprintf(gp,"e\n");

                
           fflush(gp);

        plotlock = 0 ;

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



int writepipe(struct timeval *stime,  int pnchan, int *pchannels)
{
int fd ;
FILE *fp ;
int i ;
char tstr[181] ;
char line[181] ;
char data[65536];
float xtime[datasize] ;
char popt ;
int ret ;
int j ;
int achan ;
float xpos, ypos ;
  

             for (j= 0 ; j < pnchan; j++ )
             {

                 achan = pchannels[j] ;

                 // open the file here
                  umask(~fm);
                 if ( pdat[achan].fp == 0 ) 
                 {
                   strcpy(tstr, onlinedir);
                   strcpy(tstr+strlen(tstr), "/");
                   sprintf(tstr+strlen(tstr), "chan%0d", achan);
                   sprintf(tstr, "/tmp/_chan%02d", achan);
                   //pdat[achan].fp = open(tstr, O_WRONLY|O_NONBLOCK);
                   ret = mkfifo(tstr, fm);

               if ( ret < 0 )
               {
                 if ( errno == EEXIST)
                 {

                   pdat[achan].fp = fopen(tstr, "w+");

			   if( !pdat[achan].fp )
			   { fprintf(stdout,"online write error\n"); }
		 }
                 else
		 {
                       printf("pipe open failed\n");
                       exit(0);
		 }
	       }
                  }
             }

	   if( pnchan != 0 )
	   {
	       for (j= 0 ; j < pnchan; j++ )
	       {
		 achan = pchannels[j] ;
		 fp = pdat[achan].fp ;
       
         if (fp)  
         {
                //rewind(fp);
           fprintf(stdout,"writing.... %d ....%f  %f\n",
                   datasize/4 - pdat[achan].offset,
                   pdat[achan].timeptr[0], 
                   pdat[achan].timeptr[datasize/4-1]);

            fd = fileno(fp);
            for (i= pdat[achan].offset ; i < datasize/4; i++ )
            {
         
             fprintf(fp,"%lf  %lf ", pdat[achan].timeptr[i]  
                                     ,pdat[achan].dataptr[i]) ;
//             write(fd,&pdat[achan].timeptr[i],4);  
//             write(fd,&pdat[achan].dataptr[i],4);  
            }
             fprintf(fp,"END\n");
           fflush(fp);
           rewind(fp);
         }

       }
   }

}

int writeonline(struct timeval *stime,  int pnchan, int *pchannels)
{
FILE *fp ;
int fd; 
int i ;
char tstr[181] ;
float xtime[datasize] ;
char popt ;
int j ;
int achan ;
float xpos, ypos ;
  

             for (j= 0 ; j < pnchan; j++ )
             {

                 achan = pchannels[j] ;

                 // open the file here
                 if ( pdat[achan].fp == NULL ) 
                 {
                   strcpy(tstr, onlinedir);
                   strcpy(tstr+strlen(tstr), "/");
                   sprintf(tstr+strlen(tstr), "chan%0d", achan);
                   pdat[achan].fp = fopen(tstr,"w");
                   if( !pdat[achan].fp )
                   { fprintf(stdout,"online write error\n"); }
                 }
             }

   if( pnchan != 0 )
   {
       for (j= 0 ; j < pnchan; j++ )
       {
         achan = pchannels[j] ;
         fp = pdat[achan].fp ;
       
         if (fp)  
         {
                rewind(fp);

             fd = fileno(fp);
            for (i= 0 ; i < datasize/4; i++ )
            {
            write(fd, &pdat[achan].timeptr[i],sizeof(float));  
             write(fd, &pdat[achan].timeptr[i],sizeof(float));  
             write(fd, &pdat[achan].dataptr[i],sizeof(float));  
            }
#ifdef DEBUG
            fprintf(stderr, "writeonline ndat %d  \n", datasize/4 );  
            fprintf(stderr, "writeonline first value %f  %f\n", pdat[achan].timeptr[0], pdat[achan].dataptr[i] );  
            fprintf(stderr, "writeonline last  value %f  %f\n", pdat[achan].timeptr[0], pdat[achan].dataptr[i] );  
#endif
           fflush(fp);
         }

       }
   }

}

int getrms()
{
int j,i ;
int pnchan ;
int achan ;
int pchannels[MAXCHAN];
float *dp, *xptr ;
float rms ;
float mean ;
float totmean, totrms ;
float ymax, ymin ;
float xmax, xmin ;
int fillsize ;
int jk, rnmax ;
float rmean ;

               pnchan = 0 ;
             for (j= 0 ; j < MAXCHAN; j++ )
             {
                if ( pdat[j].status & STREAMING )
                {
                   pchannels[pnchan] = j ;
                   pnchan++ ;
                }
             }

          for (j= 0 ; j < pnchan; j++ )
          {
               achan = pchannels[j] ;
               dp = pdat[achan].dataptr ;
               xptr = pdat[achan].timeptr ;
               fillsize = pdat[achan].fillsize ;

                totmean = 0.0;
                ymax = *dp ;
                ymin = *dp ;
                xmax = *xptr ;
                xmin = *xptr ;
               mean = 0.0;
             for (i= datasize/4-fillsize; i < datasize/4; i++ )
             {
                totmean += *(dp+i); 
                if ( *(dp+i) > ymax ) ymax = *(dp+i) ;
                if ( *(dp+i) < ymin ) ymin = *(dp+i) ;
                if ( *(xptr+i) > xmax ) xmax = *(xptr+i) ;
                if ( *(xptr+i) < xmin ) xmin = *(xptr+i) ;
                 if ( i >= pdat[achan].offset )
                 {
                   mean += *(dp+i); 
                 }
             }
                totmean = totmean / (fillsize);
                mean = mean / (datasize/4-pdat[achan].offset);


               totrms = 0.0 ;
               rms = 0.0;
               jk = 0 ;
             for (i= datasize/4-fillsize ; i < datasize/4; i++ )
             {
               totrms += pow( (*(dp+i) - totmean ), 2.0 );
                 if ( i >= pdat[achan].offset )
                 {
                  rms += pow( (*(dp+i) - mean ), 2.0 );
                 }
                 rmean = 0.0 ;
                 rnmax = i+nrunmean <= datasize/4 ? i+nrunmean : datasize/4 ;
                for (jk = i ; jk < rnmax ; jk++ )
                {
                  rmean += *(dp+jk) ;
                }
                  rmean /= (rnmax-i) ;
                  *(dp+i) = rmean ;
             }
              totrms = totrms/(fillsize - 1) ;
              totrms = sqrt(totrms);
              rms = rms/(datasize/4 - pdat[achan].offset-1 ) ;
              rms = sqrt(rms);

              pdat[achan].rms = rms ;
              pdat[achan].mean = mean ;
              pdat[achan].totrms = totrms ;
              pdat[achan].totmean = totmean ;
              pdat[achan].xmin = xmin ;
              pdat[achan].xmax = xmax ;
              pdat[achan].ymin = ymin ;
              pdat[achan].ymax = ymax ;
          }
            
 return 0 ;
}


void plotmain(void *argv)
{
int    asock ;
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


      if (buffer == NULL)
      buffer = (float *)malloc(buffersize); 

      if (!buffer)
      {
       fprintf(stderr,"PLOT: bufffer error \n"); exit(0); 
      }

      stopplot = 0 ;
     while(stopplot != STOPPLOT)
     {
          //get pchannels and pnchan 
               pnchan = 0 ;
              for ( i =0 ; i < MAXCHAN ; i++)
              {
                if ( pdat[i].status & STREAMING )
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

         ret = poll(ufds, pnchan, 1000);
         if (ret)
         {
               maxbyte = 0 ;
               maxbytechan = 0 ;
             for ( i =0 ; i < pnchan ; i++)
             {
               pnrec[i] = 0 ;
              if( (ufds[i].revents & POLLOUT) ||
                  (ufds[i].revents & POLLOUT) 
                )
              {
                achan = pchannels[i] ;
                asock = pdat[achan].streamsock ;
                fillsize = pdat[achan].fillsize ;
                //fprintf(stderr,"data received for chan %d %d\n", achan,rec );
                rec = socket_read_data(asock, (char *)buffer, buffersize);
#ifdef DEBUG
                fprintf(stderr,"data received for chan %d %d\n", achan,rec );
#endif

                 if(rec>0)
                 {
                           foundtime = 0;
                      jk=rec-1;
                      while( jk >= 0)
                      {

                        if( (strncmp((char*)buffer+jk,"TIME",4) == 0)) 
                        {
                           //read the time first 
                          memcpy(&pstime[i], ((char *)buffer)+jk+4,8);
                          memcpy(&sampling, ((char *)buffer)+jk+12,4);
                          if (readjust == 0 )
                          {
                             changebuffer(pnchan, pchannels);
                             readjust = 1 ;
                          }
                          pretime[achan] = pstime[i] ;
#ifdef DEBUG
                          fprintf(stderr,"TIME received %d %d %d %f\n", 
                                achan, pstime[i].tv_sec, pstime[i].tv_usec, sampling);
#endif
                           rec = rec-16 ;
                           memmove( ((char *)buffer)+jk, 
                                    ((char *)buffer)+jk+16,rec-jk);
                             
                           foundtime = 1;
                        }
                       jk--;
                      }

                      if ( ( foundtime == 0 ) && rec != 0 ) pstime[i] = pretime[achan] ;
 
                      pnrec[i] = rec ;
                      rem = (datasize - rec );
                      pdat[achan].offset =  rem/ 4 ;
       
 
                       dp = pdat[achan].dataptr ;
                       if( dp)
                       {
                        memmove(dp,dp+rec/4, rem);
                        memmove(dp+rem/4,buffer,rec);
                        fillsize += rec/4 ;
                       }
                      if (rec > maxbyte)
                      {
                        maxbyte = rec ;
                        maxbytechan = achan ;
                      }

                   pdat[achan].fillsize =
                       fillsize < datasize/4 ? fillsize : datasize/4 ;
                }
                if (rec < 0 )
                {
                  // close the sock //
                  close(asock);
                  pdat[achan].status &= ~SOCKETOPEN ;
                  pdat[achan].status &= ~STREAMING ;
                  if (pdat[achan].dataptr != NULL ) free(pdat[achan].dataptr);
                  if (pdat[achan].timeptr != NULL ) free(pdat[achan].timeptr);
                  pdat[achan].fillsize = 0 ;
                }

              }

             }
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


             for (j= 0 ; j < pnchan; j++ )
             {
              achan = pchannels[j] ;
              gettimeaxis(pstime[j],  pdat[achan].timeptr, pdat[achan].offset) ;
             }
             getrms();

            if  ( prooption == 1 )
            {
#ifdef DEBUG
              fprintf(stderr,"plotgnu called pnchan %d", pnchan);
#endif
              plotgnu(pstime, pnchan, pchannels);
            }
            if  ( prooption == 3 )
            {
#ifdef DEBUG
              fprintf(stderr,"writepipe called pnchan %d", pnchan);
#endif
             writepipe(pstime,pnchan, pchannels);
            }

            if  ( prooption == 2 )
            {
#ifdef DEBUG
              fprintf(stderr,"writeonline called pnchan %d", pnchan);
#endif
              writeonline(pstime, pnchan, pchannels);
            }
          

         } //ret check


     }//while stopplot loop

}

int  initthreadplotmain()
{
int ret ;

         ret = pthread_create( &threadplotmain, NULL, (void *)&plotmain, NULL);
         if (ret ) {
          fprintf(stderr,"PLOT Could not create plotmain thread\n");
         }

 return 0 ;
}


int  initthreadplotinput()
{
int ret ;

         ret = pthread_create( &threadplotinput, NULL, (void *)&plotinput, NULL);
         if (ret ) {
          fprintf(stderr,"PLOT Could not create plotinput thread\n");
         }
 return 0 ;
}


int main(int argc,  char *argv[])
{
int i;

       prooption = 0 ;
        strncpy(proname, argv[0], 80) ;
                                                                                
        if  (strstr(proname,"mxgplot") != NULL ) { prooption = 1 ; }
        if  (strstr(proname,"mxdonline") != NULL ) { prooption = 2 ; }
        if  (strstr(proname,"mxponline") != NULL ) { prooption = 3 ; }

        if ( prooption == 0 )
        {
         printf("Calling name of the program is unknown... \n"); exit(0);
        }



        buffersize = 4*1024*4 ;
        datasize = 8*buffersize ;
        buffer = NULL ;

       for ( i =0 ; i < MAXCHAN ; i++)
       {
          pdat[i].dataptr = NULL ;
          pdat[i].timeptr = NULL ;
          pdat[i].fp = NULL ;
          pdat[i].fillsize = 0 ;
       }

#ifdef DEBUG
       fprintf(stderr,"opening gnuplot...\n");
#endif 
       if  ( prooption == 1 )
       {
        gnuplotopen();
       }
#ifdef DEBUG
       fprintf(stderr,"gnuplot success...\n");
#endif 
       parseoptionsplot(argc, argv);
#ifdef DEBUG
       fprintf(stderr,"arguments ok...\n");
#endif 
       initthreadplotinput();

#ifdef DEBUG
       fprintf(stderr,"threadplotinput ok ...\n");
#endif 
       initthreadplotmain();

#ifdef DEBUG
       fprintf(stderr,"threadplotmain ok ...\n");
#endif 
       pthread_join( threadplotinput, NULL);
       pthread_join( threadplotmain, NULL);
       fprintf(stderr,"PLOT: to end here \n");
}


