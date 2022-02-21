#include <stdio.h>
#include <math.h>
#include <stdlib.h>


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


