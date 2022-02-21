
int writepipe( int pnchan, int *pchannels)
{
int fd ;
FILE *fp ;
int i ;
char tstr[181] ;
char line[181] ;
char data[65536];
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
                   Rclient.datasize/4 - pdat[achan].offset,
                   pdat[achan].timeptr[0], 
                   pdat[achan].timeptr[Rclient.datasize/4-1]);

            fd = fileno(fp);
            for (i= pdat[achan].offset ; i < Rclient.datasize/4; i++ )
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

int writeonline( int pnchan, int *pchannels)
{
FILE *fp ;
int fd; 
int i ;
char str[181] ;
char tstr[181] ;
char popt ;
int j ;
int achan ;
float tf ;
float xpos, ypos ;
int ndat ;
  

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
                   { logmessage("MXONLINE", "online write error\n"); }
                 }
               if (Rserver.filereset == 1 )
               { ftruncate(fileno(pdat[achan].fp),0);  Rserver.filereset=0; }
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
               
             ndat = Rclient.datasize/4 ;
            for (i= 0 ; i < ndat; i++ )
            {
             write(fd, &pdat[achan].timeptr[i],sizeof(float));  
             write(fd, &pdat[achan].timeptr[i],sizeof(float));  
             tf = pdat[achan].dataptr[i] - pdat[achan].zeroref ;
             write(fd, &tf,sizeof(float));  
            }
//            sprintf(str, "writeonline ndat %d  \n", ndat );  
//            logmessage("MXONLINE", str);
//            sprintf(str, "writeonline first value %f  %f\n", 
//                               pdat[achan].timeptr[0], pdat[achan].dataptr[ndat-1] );  
//            logmessage("MXONLINE", str);
//            sprintf(str, "writeonline last  value %f  %f\n", 
//                             pdat[achan].timeptr[ndat-1], pdat[achan].dataptr[ndat-1] );  
//            logmessage("MXONLINE", str);

           fflush(fp);
         }

       }

      /* write the configuration file */ 
                   strcpy(tstr, onlinedir);
                   strcpy(tstr+strlen(tstr), "/");
                   sprintf(tstr+strlen(tstr), "chan.conf");
                  fp = fopen(tstr,"w");
               if (fp)
               {
                   fprintf(fp, "CHAN  ");
                 for (j= 0 ; j < pnchan; j++ )
                 {
                   fprintf(fp, "%d", j);
                   if (j <= (pnchan - 2)) fprintf(fp, ",");
                 }
                   fprintf(fp, "\n");

                  fprintf(fp, "YMIN  %f\n", plotpar.dmin);
                  fprintf(fp, "YMAX  %f\n", plotpar.dmax);
                  fprintf(fp, "X1LA  CST\n");
                  fprintf(fp, "X2LA  LST\n");
                  fprintf(fp, "YLA   Volt\n");
                 for (j= 0 ; j < pnchan; j++ )
                 {
                  achan = pchannels[j] ;
                  fprintf(fp, "INFO  Chan-%d-offset: %g\n", achan, 
                                  pdat[achan].zeroref - plotpar.zeroref );
                 }
                 fclose(fp);
               }
   }

}
