

/* if nrunmean is = 0 or nrunmean > ndat then assume 
   nrunmean = ndat/10 ;
   if nrunmean is negative then divide the window by that divisions 
*/
int runmeanrms(int nrunmean, int ndat, float **dptr, float *minmean, float *maxmean, float *minrms, float *maxrms, float *ptotmean, float *ptotrms)
{
int i,j ;

float tminmean, tmaxmean  ;
float tminrms, tmaxrms  ;
float rmean, rrms ;
float *y ;
int k ;
int kt ;
float totmean, totrms ;

              if (ndat < 2 ) return -1 ;


       y = *dptr ;
       tminmean = 0 ;
       tmaxmean = 0 ;
       tmaxrms = 0 ;
       tminrms = 0 ;


      if  ( (nrunmean == 0 ) || (nrunmean > ndat ) ) { nrunmean = ndat / 10 ; }
      if ( nrunmean < 0 ) { nrunmean = ndat / (-nrunmean); }
      

             kt = 0 ;
             totmean = 0.0 ;
             totrms = 0.0 ;
#ifdef DEBUG
          fprintf(stderr,"RUNMEAN: first %f  last %f\n", *(y+0), *(y+ndat-1) ); 
#endif
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
           totmean += rmean ;
           //printf("hh B: %d %d %f %f \n", i, k, rmean, rrms );
           if( k != 0 ) { rmean /= k ; }

           //printf("hh A: %d %d %f %f \n", i, k, rmean, rrms );
     

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
           totrms += rrms ;

         if ( (rrms != 0.0) && (k != 0 ) ) { rrms = sqrt( rrms/k) ; }


         if ( i==0 ) 
         {
            tminmean = rmean ;
            tmaxmean = rmean ;
            tminrms = rrms ;
            tmaxrms = rrms ;
         }
          // printf("hh: %d %d %f %f \n", i, k, rmean, rrms );

         if ( rmean < tminmean  ) {  tminmean = rmean ; }
         if ( rmean > tmaxmean  ){ tmaxmean = rmean ; }
         if ( rrms < tminrms  )  { tminrms = rrms ; }
         if ( rrms > tmaxrms  ) { tmaxrms = rrms ; }

          kt++ ;
     }
      totmean = totmean/ndat ;
      totrms  = sqrt(totrms/(ndat-1));
     
   if ( ndat  <=  nrunmean ) { return -1 ; }

    //printf("after call %f %f \n", tminmean, tmaxmean );

    *minmean = tminmean ;
    *maxmean = tmaxmean ;
    *minrms = tminrms ;
    *maxrms = tmaxrms ;
    *ptotmean = totmean ;
    *ptotrms = totrms ;

    //printf("retval %f %f %f %f \n",  *minmean, *maxmean, *maxrms, *minrms );
 return 0 ;
}


/* dp points to the begining of the array */
/* offset selects the starting point */ 
int getrms(int offset, int ndat, float *dp, float *pmean, float *prms)
{
int i ;
float mean, rms ;


               mean = 0.0;
             for (i= offset; i  < ndat ; i++ )
             {
                   mean += *(dp+i); 
             }
               mean = mean / (ndat - offset);


               rms = 0.0;
             for (i= offset; i  < ndat; i++ )
             {
                  rms += pow( (*(dp+i) - mean ), 2.0 );
             }
              if ( (ndat - offset-1) > 0 ) rms = rms/(ndat - offset-1) ;
              if (rms > 0 ) rms = sqrt(rms);

  *pmean = mean ;
  *prms = rms ;

 return 0 ;
}


int scaledata(int pnchan, int *pchannels)
{
float minmean, maxmean, minrms, maxrms ;
float totmean, totrms ;
int j ;
float mrms;
int achan ;
float fmean, zref ;
float tt ;
float xmin , xmax ;
float mean, rms ;
int nt, nn, of, ndat;
float *p ;
float ffil ;
  

             for (j= 0 ; j < pnchan; j++ )
             {
                 achan = pchannels[j] ;
                 ndat =  pdat[achan].fillsize;
                 of =  (Rclient.datasize/4 -pdat[achan].fillsize);
                 ffil =    1.0*pdat[achan].fillsize / (Rclient.datasize/4);
                 nt =  (int)(plotpar.nmeandiv* ffil );
//                  fprintf(stderr, "ndat %d nt %d nn %d\n", ndat, nt, nn ); 
                 if ( nt == 0) nt = 2 ; /*2 divisions */
                  nn = -nt ;
                 
#ifdef DEBUG
                  fprintf(stderr, "ndat %d ffil %f nn %d of %d\n", ndat, ffil, nn, of ); 
#endif
                 
                 getrms(pdat[achan].offset, Rclient.datasize/4, pdat[achan].dataptr, &mean, &rms);
                 pdat[achan].mean = mean ;
                 pdat[achan].rms = rms ;

#ifdef DEBUG
                 fprintf(stderr, "MEAN %f RMS %f \n", mean, rms ); 
#endif 
                 
                 p = pdat[achan].dataptr+of;
                 runmeanrms(nn, ndat, &p, &minmean, &maxmean, 
                                   &minrms, &maxrms, &totmean, &totrms);
                 pdat[achan].maxrms  =  maxrms;
                 pdat[achan].minrms   =  minrms;
                 pdat[achan].maxmean  =  maxmean;
                 pdat[achan].minmean  =  minmean;
                 pdat[achan].totmean  =  totmean;
                 pdat[achan].totrms   =  totrms;
              pdat[achan].xmin = pdat[achan].timeptr[0] ;
              pdat[achan].xmax = pdat[achan].timeptr[Rclient.datasize/4 -1] ;
                
             }


               fmean   =  0.0 ;
               maxrms  =  0.0 ;
               minmean = 0.0 ;
               maxmean = 0.0 ;
               xmin    = 0.0 ;
               xmax    = 0.0 ;
             for (j= 0 ; j < pnchan; j++ )
             {

                 achan = pchannels[j] ;
                 tt = (pdat[achan].minmean + pdat[achan].maxmean)/2.0 ;
                if ( j == 0 )
                 { 
                  fmean = tt ;
                  pdat[achan].zeroref  = 0.0 ;
                  minmean = pdat[achan].minmean -tt ;
                  maxmean = pdat[achan].maxmean -tt  ;
                  maxrms = pdat[achan].maxrms  ;
                  minrms = pdat[achan].minrms  ;
                  xmin  = pdat[achan].xmin ;
                  xmax  = pdat[achan].xmax ;
                 } 
                if ( j > 0 ) 
                { 
                  pdat[achan].zeroref  = tt - fmean ;
                  zref = pdat[achan].zeroref ;
                } 
                 if (maxrms < pdat[achan].maxrms  ) { maxrms = pdat[achan].maxrms; }
                 if (minrms > pdat[achan].minrms  ) { minrms = pdat[achan].minrms; }
                 if (minmean > (pdat[achan].minmean - tt) ) { minmean = (pdat[achan].minmean - tt); }
                 if (maxmean < (pdat[achan].maxmean - tt) ) { maxmean = (pdat[achan].maxmean - tt); }

                  if ( xmin > pdat[achan].xmin ) xmin = pdat[achan].xmin ;
                  if ( xmax < pdat[achan].xmax ) xmax = pdat[achan].xmax ;
             }


           plotpar.dmin = fmean + minmean - plotpar.zeroref - plotpar.sfactor*maxrms ;
           plotpar.dmax = fmean + maxmean - plotpar.zeroref + plotpar.sfactor*maxrms ;
#ifdef DEBUG
          fprintf(stderr,"minmean %f\n ",  fabsf( fabsf(minmean) -10.0) );
#endif
          if (   (fabsf(fabsf(minmean)-10.0) < 1.E-28) 
	      || (fabsf(fabsf(minmean)-0.0) < 1.E-28)
             )
          {
           plotpar.dmin = fmean + maxmean - plotpar.zeroref - plotpar.sfactor*maxrms ;
          }
           plotpar.xmin = xmin ;
           plotpar.xmax = xmax ;
#ifdef DEBUG
        fprintf(stderr,"minmean %f maxmean %f maxrms  %fdmin  %fdmax %f\n ", 
                minmean, maxmean, maxrms,  plotpar.dmin, plotpar.dmax);
#endif
}

