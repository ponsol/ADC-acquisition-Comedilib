
FILE *gp ;

int gnuplotopen()
{

            gp = popen("/usr/bin/gnuplot -geometry 1250x300+10+90", "w");
//            fprintf(gp,"set term x11 0\n") ;
            fprintf(gp,"set term x11 enhanced font \"lucidasans-bold-18\"\n");
            //fprintf(gp,"set multiplot\n") ;
            fprintf(gp,"set key out below Left box \n") ;
            //fprintf(gp,"set xdata time\n") ;
            //fprintf(gp,"set timefmt \"%H:%M\"\n") ;
            //fprintf(gp,"set format x \"%H:%M\"\n") ;
            //fprintf(gp,"set xla \"(hour:min)\"\n") ;
            fprintf(gp,"set xla \"Hour\"\n") ;
            fprintf(gp,"set yla \"Volt\"\n") ;
            fprintf(gp,"set tmargin -1 \n") ;
            fprintf(gp,"set xzeroaxis lt -2 lw 2.000   \n") ;
            fprintf(gp,"set x2zeroaxis lt -2 lw 2.000   \n") ;
            fprintf(gp,"set yzeroaxis lt -2 lw 2.000   \n") ;
            fprintf(gp,"set yzeroaxis lt -2 lw 2.000   \n") ;
            fprintf(gp,"set mxtics 5 \n") ;
            fprintf(gp,"set mytics 2 \n") ;
            fprintf(gp,"set grid lt 1\n") ;
            fprintf(gp,"set grid mxtics lt 0\n") ;
            fprintf(gp,"set grid mytics lt 0\n") ;
            fprintf(gp,"plot [0:1] [-1:1] 0  \n") ;
            fflush(gp);
            //fprintf(gp,"set size ratio 0.4\n") ;
            //fprintf(gp,"set timefmt \"%s\"\n") ;
}

int plotgnu(int pnchan, int *pchannels)
{
char key[MAXCHAN][281] ;
float doffset[MAXCHAN] ;
int i ;
float xtime[Rclient.datasize] ;
char popt ;
int j ;
int achan ;
float xpos, ypos ;
float  maxmean, minmean;
float  maxrms, minrms;
  

            // for (j= 0 ; j < pnchan; j++ )
            // {
            //   achan = pchannels[j] ;
            //  gettimeaxis(stime[j],  pdat[achan].timeptr, pdat[achan].offset) ;
            // }

              
              if ( plotpar.plotlock == 1 ) return 0 ;
              plotpar.plotlock = 1 ;

                fprintf(gp,"set xr [%f:%f]\n", plotpar.xmin, plotpar.xmax );
                fprintf(stderr,"set xr [%f:%f]\n", plotpar.xmin, plotpar.xmax );
                fprintf(gp,"set yr [%f:%f]\n", plotpar.dmin, plotpar.dmax );
                fprintf(stderr,"set yr [%f:%f]\n", plotpar.dmin, plotpar.dmax );

                for (j= 0 ; j < pnchan; j++ )
                {
                 achan = pchannels[j] ;
                 doffset[j] = pdat[achan].zeroref  ;
                 sprintf(key[j], "\" Chan:%2d  Refoffset:%6.3g  Zoom:%4.2f Offset:%8.3g Mean: %8.3g Rms: %8.3g \"", 
                  achan, plotpar.zeroref, plotpar.sfactor, doffset[j], pdat[achan].mean, 
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
       for (i= 0 ; i < Rclient.datasize/4; i++ )
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

        plotpar.plotlock = 0 ;

}


int endgnu()
{
int i ;
//ends here close all the socks
   for ( i =0 ; i < MAXCHAN ; i++)
   {
      close(pdat[i].streamsock);
   }

      fprintf(stderr,"PLOT: to end here \n");
      pclose(gp);
      exit(0);

}
