#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include <sys/stat.h>

#include <sys/file.h>

/* if nrunmean is = 0 or nrunmean > ndat then assume 
   nrunmean = ndat/10 ;
*/
int runmeanrms(int nrunmean, int ndat, float **dptr, float *minmean, float *maxmean, float *minrms, float *maxrms)
{
int i,j ;

float tminmean, tmaxmean  ;
float tminrms, tmaxrms  ;
float rmean, rrms ;
float *y ;
int k ;


       y = *dptr ;
       tminmean = 0 ;
       tmaxmean = 0 ;
       tmaxrms = 0 ;
       tminrms = 0 ;


      if  ( (nrunmean == 0 ) || (nrunmean > ndat ) ) { nrunmean = ndat / 10 ; }
      if ( nrunmean < 0 ) { nrunmean = ndat / (-nrunmean); }
      

      for (i= 0 ; i < ndat-nrunmean; i=i+nrunmean )
      {
          
             rmean = 0.0;
             k = 0 ;
          for (j = i ; j < i+nrunmean; j++ )
          {
             //if( fabsf( *(y+j)) > 1.E-45 )
             {
               rmean += (*(y+j)) ;
               k++ ;
             }
          }
           printf("hh B: %d %d %f %f \n", i, k, rmean, rrms );
           if( k != 0 ) { rmean /= k ; }
           printf("hh A: %d %d %f %f \n", i, k, rmean, rrms );

             rrms = 0.0 ;
             k = 0 ;
          for (j = i ; j < i+nrunmean; j++ )
          {
             //if( fabsf( *(y+j)) > 1.E-45 )
             {
             rrms +=  ( ( *(y+j) - rmean )*( *(y+j) - rmean ) ) ;
             k++ ;
             }
          }

         if ( (rrms != 0.0) && (k != 0 ) ) { rrms = sqrt( rrms/k) ; }


         if ( i==0 ) 
         {
            tminmean = rmean ;
            tmaxmean = rmean ;
            tminrms = rrms ;
            tmaxrms = rrms ;
         }
           printf("hh: %d %d %f %f \n", i, k, rmean, rrms );

         if ( rmean < tminmean  ) {  tminmean = rmean ; }
         if ( rmean > tmaxmean  ){ tmaxmean = rmean ; }
         if ( rrms < tminrms  )  { tminrms = rrms ; }
         if ( rrms > tmaxrms  ) { tmaxrms = rrms ; }

     }
     
   if ( ndat  <=  nrunmean ) { return -1 ; }

    //printf("after call %f %f \n", tminmean, tmaxmean );

    *minmean = tminmean ;
    *maxmean = tmaxmean ;
    *minrms = tminrms ;
    *maxrms = tmaxrms ;

    //printf("retval %f %f %f %f \n",  *minmean, *maxmean, *maxrms, *minrms );
 return 0 ;
}


// assume input comes with previous values
int getdataminmax (char *fname, int nrunmean, float scalerms,  float *min, float *max )
{
FILE *fin;
int  fd ;
int ndat ;
int ret ;
struct stat statbuf ;
float x1, x2, *y ;
float minmean, maxmean, minrms, maxrms ;
int i ;

struct flock fl = { F_WRLCK, SEEK_SET, 0,       0,     0 };


     fin =  fopen(fname,"r");
     if (!fin) return -4 ;


     fd  = fileno(fin);
     fstat(fd, &statbuf);
     ndat = (int)statbuf.st_size ;

       fl.l_pid = getpid();
       while (fl.l_type != F_UNLCK)
       {
        fcntl(fd, F_GETLK, &fl) ;
       }


     ndat = ndat / ( 3*sizeof(float) );
     //printf ("ndat, %d \n", ndat);
     y = (float *)calloc(ndat,  sizeof(float) ); 
     if ( y == NULL)
     {
       return -3 ;
     }
    for (i = 0 ; i < ndat ; i++ )
    {
      fread(&x1, sizeof(float),1,fin);
      fread(&x2, sizeof(float),1,fin);
      fread(y+i, sizeof(float),1,fin);
    }
    //printf("val %f \n", *y);
    //printf("val %f \n", *(y+ndat-1));
    fclose(fin);
     
    minmean = *min ;
    maxmean = *max ;
    minrms = 0.0 ;
    maxrms = 0.0 ;
    //printf("before call %f %f \n", minmean, maxmean );
    ret = runmeanrms(nrunmean, ndat, &y, &minmean, &maxmean, &minrms, &maxrms);
    

   if (ret == 0 )
   {
     *min = minmean - scalerms*minrms ;
     *max = maxmean + scalerms*minrms ;
     if (minrms < 1.E-45 )  
     {
      *min = minmean ;
      *max = maxmean + scalerms*(maxrms/3.0) ;
     }
   }
    printf("retval %d %f %f %f %f %f %f\n", ret, *min, *max, minmean, maxmean, maxrms, minrms );
   free(y);
 return ret  ;
}


