/*  RECORDTHREAD PACKAGE
**  Written by S. Jeyakumar 28-10-2006

**  routines to handle the threads for record 
**  also keeps tracks of the clients requesting to stream data 

*/

extern void recordmain(void * ) ;

//check serverport 
//if server exists send a dummy command 
int checkserver (int *option)
{
	int opt ;
	int                  sock ;
	struct sockaddr_in  sockserver;
	struct in_addr      sockserverip ; 
	int ret ;

	       opt = (int) *((int *)option);
	       if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) 
	       {fprintf(stderr,"RECORD: socket client: socket error\n" ); exit(0);}
	       memset(&sockserver, 0, sizeof(sockserver)); 
	       sockserver.sin_family = AF_INET ;
	       if(!inet_aton(recordserverip, &sockserverip) )
	       {fprintf(stderr,"RECORD: socket client: server IP error\n" ); exit(0);}
	       sockserver.sin_addr = sockserverip ;
	       sockserver.sin_port = htons(recordcmdport);
	       ret = connect(sock, (struct sockaddr *)&sockserver, sizeof(sockserver));

	      if (ret == 0 ) send(sock, "check", 4, 0);
              close(sock);
return ret ;
}


// This client opens  a connection to the cmdport 
// if that port is open server is already running 
void  socketclient( void *option)
{
char message[2081] ;
int                  sock ;
struct sockaddr_in  sockserver;
struct in_addr      sockserverip ; 
int ret ;
int rec ;
int opt ;
int gotmesg ;
	  
       opt = (int) *((int *)option);
       if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) 
       {fprintf(stderr,"RECORD: socket client: socket error\n" ); exit(0);}
       memset(&sockserver, 0, sizeof(sockserver)); 
       sockserver.sin_family = AF_INET ;
       if(!inet_aton(recordserverip, &sockserverip) )
       {fprintf(stderr,"RECORD: socket client: server IP error\n" ); exit(0);}
       sockserver.sin_addr = sockserverip ;
       sockserver.sin_port = htons(recordcmdport);
       ret = connect(sock, (struct sockaddr *)&sockserver, sizeof(sockserver));

       if ( (int) *((int *)option) == STARTRECORD )
       {
	  if ( ret < 0)
	  {
	    // can be started 
	    // ask the other thread to go ahead
	    fprintf(stderr,"RECORD: checking for running record .....\n" ); 
	    fprintf(stderr,"RECORD: running record not found... \n" ); 
	    fprintf(stderr,"RECORD: Creating  new..\n" ); 
	    close(sock);
	    // do nothing
	  } 
	  if ( ret == 0)
	  {
	    // can not be started 
	    // kill this process
	    fprintf(stderr,"RECORD: record  is already running\n" ); 
	    send(sock, "close", 4, 0);
	    close(sock);
	    recordstatus=STOPRECORD ;
	    exit(0);
	  } 
	
       }
       if ( *(int *)option == STOPRECORD )
       {
	  if ( ret < 0)
	  {
	   // can not be  stopped 
	   // kill this process
	    fprintf(stderr,"RECORD: record  is not already running\n" ); 
	    close(sock);
	    exit(0);
	  } 
	  if ( ret == 0)
	  {
	    // can  be stopped &  stop the old one & kill this process
	     rec = send(sock, "stop", 4, 0);
	     if ( rec != 4) 
	     {
	      fprintf(stderr,"RECORD: error while stoping record\n" ); 
	       close(sock);
	      //exit current process
	      exit(0);
	     }               
	      close(sock);
	      recordstatus = STOPRECORD ;
	      fprintf(stderr,"RECORD:  stop message sent!\n" ); 
	    //exit current process
	      exit(0);
	  } 
	
       }

       if ( *(int *)option == STATUSRECORD )
       {
	  if ( ret < 0)
	  {
	   // not running
	   // kill this process
	    fprintf(stderr,"RECORD: record  is not-running\n" ); 
	    fprintf(stderr,"RECORD: Logfilename \n" ); 
	    close(sock);
	    exit(0);
	  } 
	  if ( ret == 0)
	  {
	    // running process 
	     fprintf(stderr,"RECORD: record  is running\n" ); 
	     send(sock, "stat", 4, 0);
	    

	   //get the message 
	     rec = -1 ;
	       memset(message,0,2081);
	       rec = recv(sock, message, 2080, 0) ;

	     if (rec < 0) 
	     {
	      fprintf(stderr, "RECORD: socket client message receive error\n"); 
	      exit(0);
	     }
	     if (rec > 0) 
	     {
	       fprintf(stderr,"RECORD: Logfilename %s \n", message ); 
	     }

	     rec = -1 ;
	     memset(message,0,2081);
	      rec = recv(sock, message, 2080, 0) ;
	     if (rec < 0) 
	     {
	      fprintf(stderr, "RECORD: socket client message receive error\n"); 
	      exit(0);
	     }
	     if (rec > 0) 
	     {
	     fprintf(stderr,"RECORD: Comment: %s \n", message ); 
	     }

	     rec = -1 ;
	     memset(message,0,2081);
	      rec = recv(sock, message, 2080, 0) ;
	     if (rec < 0) 
	     {
	      fprintf(stderr, "RECORD: socket client message receive error\n"); 
	      exit(0);
	     }
	     if (rec > 0) 
	     {
	     fprintf(stderr,"RECORD: Observer %s \n", message ); 
	     }

	     rec = -1 ;
	     memset(message,0,2081);
	      rec = recv(sock, message, 2080, 0) ;
	     if (rec < 0) 
	     {
	      fprintf(stderr, "RECORD: socket client message receive error\n"); 
	      exit(0);
	     }
	     fprintf(stderr,"RECORD: Sampling %s \n", message ); 

	     rec = -1 ;
	     memset(message,0,2081);
	      rec = recv(sock, message, 2080, 0) ;
	     close(sock);
	     if (rec < 0) 
	     {
	      fprintf(stderr, "RECORD: socket client message receive error\n"); 
	      exit(0);
	     }
	     fprintf(stderr,"RECORD: Recording %s \n", message ); 

	     exit(0);
	  } 
	    exit(0);
	
	  // stop the process querying 
	  // recordstatus = STOPRECORD ;
       }

    
}


struct timeval maketime(int hr, int min,int sec)
{
struct timeval at ;
struct tm *tmt ;
	   gettimeofday(&at, 0);
	   tmt = gmtime(&at.tv_sec);
	   tmt->tm_hour = hr ;
	   tmt->tm_min  = min ;
	   tmt->tm_mday = sec ;
	   tmt->tm_sec  = sec ;
	   at.tv_sec  = mktime(tmt);
	   at.tv_usec = 0 ;

 return at ;
}


int decodestr(char *str, char substr[][81], int nmax)
{
char line[581] ;
char *p , *c ;
int n,i ;

      memset(line,'\0',581);
      strcpy(line, str);
      p = line ;

     for (i=0 ; i < nmax ; i ++ )
     {
       memset(&substr[i][0],0,80);
     }

	c = strsep(&p, " ");
       n= 0 ;
      while ( c != NULL )
      {
	 n++ ;
	  if ( n <= nmax )
	  {
	    strcpy( &substr[n-1][0], c);
	  }
	c = strsep(&p, " ");
      }
  return n ;
}

int decodemesg(char *message, struct sockaddr_in *client, int csock)
{
char   mstr[581] ;
char   sname[181] ;
char   submes[10][81] ;
int    lmes ;
int    ichan ;
int    beghr, begmin, begsec ;
int    endmin ;
int    dummysec ;
int   ct ;
int   retstream ;
int   achan, sret ; 

streams *stp, *stpnew ;
int    i ;
int repeat ;


	    logmessage("RECORD", message); 
     if( strstr(message,"stop") != NULL ) 
     {
	    // great  stop the record
	    logmessage("RECORD", "  socket server stop mesg. received....."); 
	    return 0 ;
     }


	 lmes = decodestr(message, submes, 10) ;

	 if (lmes >= 2 )
	 {
	     ct = sscanf(submes[1],"%d", &ichan);
	 }

	 if (lmes == 3 )
	 {
	   sscanf(submes[2],"%8s", sname);
	 }
	 if (lmes == 4 )
	 {
	   sscanf(submes[2],"%d", &endmin);
	   sscanf(submes[3],"%8s", sname);
	 }

        if( strstr(submes[0],"scan") != NULL ) 
        {
          if( (lmes == 4 ) &&
	    (!(recordinfo.channelstat[ichan] & RECORDING ))
            )
           {
         // got all (but sname)
           if (lmes==4) strncpy(recordinfo.source[ichan], sname,8) ;

           recordinfo.cmd[ichan] = 's' ;
           recordinfo.recno[ichan] = 0 ;
           recordinfo.begintime[ichan].tv_sec  =  0 ;
           recordinfo.begintime[ichan].tv_usec =  0 ;
           dummysec  = endmin*60 ;
            recordinfo.endtime[ichan].tv_sec   =  -dummysec ;
            recordinfo.endtime[ichan].tv_usec  =  0 ;
           recordinfo.channelstat[ichan] |= RECORDING ;
          sprintf(mstr," %s", message);
          logmessage("RECORD", mstr);
        }
        else
        {
          logmessage("RECORD", " socket server: scan scan error");
          sprintf(mstr," %s", message);
          logmessage("RECORD", mstr);
           if ((recordinfo.channelstat[ichan] & RECORDING ))
           logmessage("RECORD", " recording is ON already in this channel");
        }
     }

     if( strstr(submes[0],"end") != NULL ) 
     {
        if( (lmes >= 2 ) &&
            ((recordinfo.channelstat[ichan] & RECORDING ))
          )
        {
            recordinfo.channelstat[ichan] &= (~RECORDING) ;
            recordinfo.recno[ichan] = 0 ;
            if( recordinfo.fd[ichan] ) close(recordinfo.fd[ichan]) ;
            recordinfo.fd[ichan] = 0 ;
            recordinfo.cmd[ichan] = 'e' ;
            
          sprintf(mstr," %s", message);
          logmessage("RECORD", mstr);
        }
        else
        {
          logmessage("RECORD", " socket server: scan end error");
          sprintf(mstr," %s", message);
          logmessage("RECORD", mstr);
           if (!(recordinfo.channelstat[ichan] & RECORDING ))
           logmessage("RECORD", " recording is not ON in this channel");
        }
     }

     if( strstr(submes[0],"begin") != NULL ) 
     {
        if( (lmes >= 2 ) &&
            (!(recordinfo.channelstat[ichan] & RECORDING ))
          )
        {
            if (lmes < 3) strncpy(sname,"TMSERIES", 8); 
           recordinfo.cmd[ichan] = 'b' ;
          if ( ! (recordinfo.channelstat[ichan] & RECORDING) )
          {
            recordinfo.endtime[ichan].tv_usec = 0 ;
            recordinfo.endtime[ichan].tv_sec = 0 ;
            recordinfo.recno[ichan]  = 0 ;
            strncpy(recordinfo.source[ichan], sname,8); 
            recordinfo.channelstat[ichan] |= RECORDING ;
          sprintf(mstr," %s", message);
          logmessage("RECORD", mstr);
          }
        }
        else
        {
          logmessage("RECORD", " socket server: scan begin  error");
          sprintf(mstr," %s", message);
          logmessage("RECORD", mstr);
           if ((recordinfo.channelstat[ichan] & RECORDING ))
           logmessage("RECORD", " recording is ON already in this channel");
        }

     }

     if( strstr(submes[0],"dump") != NULL ) 
     {
        if( (lmes == 2 ) &&
            (!(recordinfo.channelstat[ichan] & RECORDING ))
          )
        {
           recordinfo.cmd[ichan] = 'd' ;
            recordinfo.endtime[ichan].tv_usec = 0 ;
            recordinfo.endtime[ichan].tv_sec = 0 ;
            recordinfo.recno[ichan]  = 0 ;
            strncpy(recordinfo.source[ichan], "TMSERIES",8); 
            recordinfo.channelstat[ichan] |= RECORDING ;
          sprintf(mstr," %s", message);
          logmessage("RECORD", mstr);
        }
        else
        {
          logmessage("RECORD", " socket server: scan dump  error");
          sprintf(mstr," %s", message);
          logmessage("RECORD", mstr);
           if ((recordinfo.channelstat[ichan] & RECORDING ))
           logmessage("RECORD", " recording is ON already in this channel");
        }

     }

     if( strstr(submes[0],"nchan") != NULL ) 
     {
            //send the number of channels available
            achan = ahead->nchan ;
            sret = send(csock, &achan, sizeof(int), 0);
            if ( sret > 0 ) logmessage("STREAM:", "nchan sent ");
     }

#ifdef STREAM
     if( strstr(submes[0],"stream") != NULL ) 
     {
        decodestream(client, lmes, ichan, message);
     }
#endif
               
return 1 ;
}



void  socketserver(void *arg)
{
char message[81] ;
int                  ssock, csock ;
struct sockaddr_in  sockserver, client;
struct in_addr      sockserverip ; 
int ret, rec ;
socklen_t clientlen ;
int go ;
int sret ;
int i ;

      if ((ssock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) 
      { 
       logmessage("RECORD", "  socket server: socket error" ); 
       exit(0);
      }

       memset(&sockserver, 0, sizeof(sockserver)); 
       sockserver.sin_family = AF_INET ;
       if(!inet_aton(recordserverip, &sockserverip) )
       {
         logmessage("RECORD", "  socket server: server IP error" ); 
         exit(0);
       }
       sockserver.sin_addr = sockserverip ;
       sockserver.sin_port = htons(recordcmdport);

      do
      { 
        ret = bind(ssock, (struct sockaddr *)&sockserver, sizeof(sockserver));
        if (ret < 0 )
        {
          logmessage("RECORD", "  socket server: bind error" ); 
          sleep(2);
        }
        else
        {
          logmessage("RECORD", "  socket server: success" ); 
          serverok = 1 ;
        }
      } while ( ret < 0 ) ;


       ret = listen(ssock, 1) ;
       if (ret < 0 )
       {
          logmessage("RECORD", "  socket server: listen error" ); 
          exit(0);
       }

         clientlen = sizeof(client);


     go = 1 ;
     while (go)
     {
       

         memset(&client, 0, sizeof(struct sockaddr_in)); 
        csock = accept(ssock, (struct sockaddr *)&client, &clientlen);
        if (csock < 0 )
        {
          logmessage("RECORD", "socket server connetion accept error" ); 
          exit(0);
        }

//        initthreadTalkCommandport();
    
      //get the message 
        rec = -1 ;
        memset(message,0,80);
        rec = recv(csock, message, 80, 0) ;
              logmessage("RECORD MESG", message); 
           
        if (rec < 0) 
        {
           logmessage("RECORD", "socket server  message receive error"); 
           go = 0 ;
        }

        if (rec > 0) 
        {
             if( strstr(message,"stat") != NULL ) 
             {
              logmessage("RECORD", "message status revceived"); 
              sret = send(csock, recordlogfilename, strlen(recordlogfilename),0);
              if ( sret > 0 ) logmessage("RECORD", "Logfile name sent!"); 
              usleep(50000);
              sret = -1 ;
              sret = send(csock, dataheader.other, 
                        strlen(dataheader.other),0);
              logmessage("RECORD:", dataheader.other);
              if ( sret > 0 ) logmessage("RECORD", "Comment sent!"); 

              usleep(50000);
              sret = -1 ;
              sret = send(csock, dataheader.observer, 
                        strlen(dataheader.observer),0);
              if ( sret > 0 ) logmessage("RECORD", "Observer sent!"); 

              usleep(50000);
              sret = -1 ;
              sprintf(message, "%g", dataheader.sampling);
              sret = send(csock, message, 
                        strlen(message),0);
              if ( sret > 0 ) logmessage("RECORD", "Samplingrate sent!"); 

              usleep(50000);
              sret = -1 ;
                   sprintf(message, "  ");
               for (i = 0 ; i < MAXCHAN; i++)
               {
                 if (recordinfo.channelstat[i] & RECORDING )
                 { 
                   sprintf(message+strlen(message)," %d ", i);
                 } 
               }
              sret = send(csock, message, 
                        strlen(message),0);
              logmessage("RECORD:", dataheader.other);
              if ( sret > 0 ) logmessage("RECORD", "Record status sent!"); 

             }
             else 
             {
               go =  decodemesg(message, &client, csock );
             }
        }
        //close(csock);
        if (go == 0 ) 
        {
         // close(ssock);
        }
     }

     logmessage("RECORD", "  socket server:  exiting thread"); 
     recordstatus = STOPRECORD ;
}


// open a client thread   to run socketclient  //
// this thread is terminated  // 
int initthreadclient(int *optionstart)
{
int ret ;


         ret = pthread_create( &threadclient, NULL,  (void *) &socketclient, (void *)optionstart);
         if (ret )
         {
           fprintf(stderr, " Could not create socket client thread\n");
           exit(0) ;
         }
}


// make the process a daemon  //
int makedaemon1()
{
pid_t childpid, childsid;


        childpid = fork();
        if (childpid < 0) {
              fprintf(stderr, "  fork error\n");
                exit(EXIT_FAILURE);
        }
        if (childpid > 0) {
         // this is the parent & kill the parent 
         // exit(EXIT_SUCCESS);
        }
        else
        {
           childsid = setsid();
           if (childsid < 0)  
           {
            //session not possible exit 
              fprintf(stderr, "  SID could not  be obtained\n");
              exit(EXIT_FAILURE);
           }
          
        }

// go ahead 
// do other processes 
 return childpid  ;
}

int initfiles()
{
// close the sdterr and stdout
// opne the log file
struct timeval curt ;
struct timezone tz;
char tstr[25] ;


    umask(~filemode);
    reclogfp  = fopen(recordlogfilename, "a");
    if (!reclogfp)
    {
      fprintf(stderr, " Error opening log file %s", recordlogfilename);
      exit(0);
    }

     gettimeofday(&curt, &tz);
     strncpy(tstr,ctime(&curt.tv_sec),24);
     tstr[24] = '\0' ;
     fprintf(reclogfp,"\n********* %s **********\n", tstr);
}


int makedaemon2(pid_t pid)
{
// close the sdterr and stdout
// opne the log file
   //fprintf(stderr, "RECORD: daemon success...\n");
   fflush(stderr);
   fflush(stdout);
   fclose(stderr);
   fclose(stdout);
   logmessage("RECORD", "daemon success...");
}


// open a server thread  to run socketserver     //
// this thread is terminated by the stop command // 
int  initthreadserver()
{
int ret ;


         ret = pthread_create( &threadserver, NULL, (void *)&socketserver, NULL);
         if (ret ) {
          logmessage("RECORD", " Could not create socket server thread");
         }
 return 0 ;
}


// open a thread  to run recordmain              //
// this thread is terminated by the stop command // 
// or on write error                             //
int  initthreadrecord()
{
int ret ;

         ret = pthread_create( &threadrecord, NULL, (void *)&recordmain, NULL);
         if (ret ) {
          logmessage("RECORD", " Could not create recordmain  thread");
         }
 return 0 ;
}

