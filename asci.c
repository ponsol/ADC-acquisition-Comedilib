/*  ASCI PACKAGE
**  Written by S. Jeyakumar 28-10-2006

**  converts to ascii
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "data.h"

float bhour, ehour ;
int    optcount ;
char   timearg[81];

int getsampletime(struct timeval *begtime, struct timeval *sampletime, float sampling)
{
struct timeval  itime ;
long int basetime ;
long int usec, busec ;
int i ;

        itime = *begtime ;
        basetime = itime.tv_sec - itime.tv_sec%2 ;
        itime.tv_sec -= basetime ;
        busec = itime.tv_sec*1000*1000 + itime.tv_usec ;
    
       for (i=0; i < 1024; i++)
       {
          usec = busec + (int)(i*sampling*1000) ;
          sampletime[i].tv_usec = usec%(1000*1000) ;
          sampletime[i].tv_sec =  (usec - sampletime[i].tv_usec)/(1000*1000) ;
          sampletime[i].tv_sec += basetime ;
       }

 return 0 ;
}

int usage()
{
  fprintf(stderr,"Usage: mxasci [-x d | h]  filename [begin-time  end-time]\n");
}

int main(int argc, char *argv[] )
{
mexartheader dataheader ;
char fname[581] ;
char fnameout[581] ;
int ct ;
struct tm  *tmt;
struct timeval scantime ;
struct timeval stime[1024] ;
int    recno ;
float rvolts[1024] ;
float  fsec ;
float  fhour ;
mexartscan mscan ;
int fd ;
FILE *fout ;
int i ;
int ret ;
int  oct;

 

    optcount = 0 ;
    while ( (oct = getopt(argc, argv, "x:") ) != -1 ) 
    {
      if  (oct == 'x' )
      {
       strcpy(timearg, optarg);
       optcount++ ;
      }
    }


    bhour = 0.0  ;
    ehour = 24.0 ;

    if ( argc-optind < 1 ) { usage(); exit(0); }
    ct = sscanf(argv[optind],"%s", fname);
     
    if (ct != 1)  { usage(); exit(0); }

    if (argc - optind > 1) { 
     ct = sscanf(argv[optind+1],"%f", &bhour);
     if (ct != 1)  { usage(); exit(0); }
    }
    if (argc -optind > 2) { 
      ct = sscanf(argv[optind+2],"%f", &ehour);
      if (ct != 1)  { usage(); exit(0); }
    }

     if ( strcmp(fname+strlen(fname)-3, "mxd") == 0  )
     {
      strncpy(fnameout, fname, strlen(fname)-3);
      strcat(fnameout, "txt");
     }
     else
     {
      strncpy(fnameout, fname, strlen(fname));
      strcat(fnameout, ".txt");
     }


     fd = open(fname, O_RDONLY) ;
     fout = fopen(fnameout, "w+") ;
     if ( fd < 0 ) 
     {
       fprintf(stderr,"Error opening file %s\n", fname);
       exit(0);
     }

//to obtain the number of records
      ct = read(fd, &dataheader, 2048);
      fprintf(fout,"#Filename     : %s\n", fname);
      fprintf(fout,"#Sampling (ms): %g\n", dataheader.sampling);
      fprintf(fout,"#Observer     : %s\n", dataheader.observer);
      fprintf(fout,"#Comments     : %s\n", dataheader.other);
      fprintf(fout,"#Begin hour   : %f  End hour %f\n", bhour, ehour);
       i = 0 ;
     while ( ct > 0 )
     {
        while ( (ct = read(fd, &mscan,sizeof(mexartscan)) ) > 0 ) 
        {
         i++ ; 
         fprintf(fout,"#Scan %04d at %d %d\n", mscan.scanno, 
              mscan.scantime.tv_sec, mscan.scantime.tv_usec);
        }
     }
      fprintf(fout,"#Number of records %d\n", i );
      close(fd);
//close and reopen later 
          
        fd = open(fname, O_RDONLY) ;
        if ( fd < 0 ) 
        {
          fprintf(stderr,"Error opening file %s\n", fname);
          exit(-1);
        }


      ct= read(fd, &dataheader, 2048);
     while ( ct > 0 )
     {
       ct= read(fd, &recno, sizeof(int));
       ct= read(fd, &scantime, sizeof(struct timeval));
       ct= read(fd, rvolts, sizeof(float)*1024);
       getsampletime(&scantime, stime, dataheader.sampling) ;
       //printf("%s\n", ctime(&scantime.tv_sec) );
       
       for ( i=0; i < 1024 ; i++ )
       {
          tmt =  localtime(&stime[i].tv_sec);
         
          //fprintf(stdout,"%#02d %#02d ", 
          //             tmt->tm_hour, 
          //             tmt->tm_min
          //        );
              fsec = (float)(tmt->tm_sec) + (float)stime[i].tv_usec/1.E6;
              fhour = (float)(tmt->tm_hour) +  (float)tmt->tm_min/60.0;
              fhour = fhour + fsec/60.0/60.0 ;
          //fprintf(stdout,"%#09.6f ", fsec);
              if (strcmp(timearg,"d") == 0 ) 
              {
                 fhour /= 24.0 ;
              }
              

          if ( ( fhour > bhour ) && (fhour <= ehour ) )
          {
           fprintf(fout,"%#028.25f ", fhour);
           //fprintf(stdout,"%#021.18f\n", fhour);
           fprintf(fout,"%f\n", rvolts[i] );
          }
          //printf("%s\n", asctime(tmt) );
       }
     }
}
