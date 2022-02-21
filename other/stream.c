/*   STREAM PACKAGE
**   S. Jeyakuamr
**
**   All routines  to stream the data over a  port 
**   Stream  a maximum of 8 KBy/s 
**   if the data rate is too fast average even number 
**   of samples such that the above criteria is met. 
**   Clients receive the modified sampling rate 
*/

// and the stream ports
int serversettings()
{
int i ;

  for( i =0; i < MAXCHAN ; i++ )
  {
    recordstreamport[i] = (recordcmdport + i + 1) ;
  }
}



int decodestream(struct sockaddr_in * client, int lmes,  int ichan, char *message)
{
char mstr[581] ;
streams *stp, *stpnew ;
int    i ;
int repeat ;

        if ( strstr(message,"achan") != NULL )
        {
          return 1;
        }

        if( lmes == 2 )
        {

           sprintf(mstr,"socket server: stream %d received ",ichan );
           logmessage("STREAM", mstr);
               repeat = 0 ;
               stp = recordinfo.stream[ichan] ;
           while ( stp != NULL )
           {
               //check client
               if( stp->client.sin_addr.s_addr == client->sin_addr.s_addr )
               {
                // same client  is asking for same channel //
                  repeat = 1 ;
               }
              if ( stp->next == NULL ) break ;
              stp = stp->next ; 
           }


            // new connection requested 
            // queue this if possible
            if (  repeat == 0 ) 
            {

                recordinfo.channelstat[ichan] |= STREAMING   ;

               if ( recordinfo.nstream[ichan] < MAXSTREAM ) 
               {
 
                  stpnew = (streams *)malloc(sizeof(streams));
                  if (stpnew)
                  {
                        stpnew->firstyes = 1 ; 
                    
                     if (stp)
                     {
                      // additional stream 
                         stp->next = stpnew ; 
                         stpnew->prev = stp ;
                         stp = stpnew ;
                     }
                     else
                     {
                      // this is the first stream 
                        stp = stpnew ;
                        recordinfo.stream[ichan] = stp ;
                        stp->prev=0;
                     }
                         stp->next = 0 ;
                         stp->client =  *client ;
//to be locked
                    recordinfo.streamoffset[ichan] = abuf->offset ;
                    //increment the stream counter //
                    recordinfo.nstream[ichan]++ ;
                  }
                  else
                  {
                   logmessage("STREAM", "stream memory allocation error ");
                  }
               }
               else
               {
                sprintf(mstr,"Cannot stream more than %d clients ", MAXSTREAM);
                logmessage("STREAM", mstr);
               }
            }
            else
            {
                logmessage("STREAM","already streaming to this client ");
            }
        }
        else
        {
          logmessage("STREAM","socket server: scan stream  error ");
          sprintf(mstr,"%s ", message);
          logmessage("STREAM",mstr);
        }

return 0;
}


int  openstreamsock(int ichan)
{
int ret ;
int  sock ;
struct sockaddr_in streamer ;
struct in_addr     streamerip ;
       

 
   // open a socket for  the channel //
      if ( !recordinfo.streamsock[ichan] ) 
      {
         if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
         { 
            logmessage("STREAM","streamer socket error " ); 
            return -1 ;
         }

         memset(&streamer, 0, sizeof(streamer));
         if(!inet_aton(recordserverip, &streamerip) )
         { 
           logmessage("STREAM", "streamer  server IP error " );
           return -1 ;
         }

         streamer.sin_family = AF_INET ;
         streamer.sin_addr = streamerip ;
         streamer.sin_port = htons(recordstreamport[ichan]);
         ret = bind(sock, (struct sockaddr *)&streamer, sizeof(streamer));
         if ( ret < 0)
         {
           // bind not possible  //
            logmessage("STREAM", "streamer  socket bind error " );
            return -1 ;
         }

         recordinfo.streamsock[ichan] =  sock ;

         ret = listen(recordinfo.streamsock[ichan], 5);
         if (ret < 0 )
         {
           logmessage("STREAM", "listen error " );
           close(recordinfo.streamsock[ichan]) ;
          return  -1 ;
         }
      }
      else
      {
           logmessage("STREAM", "port already open " );
      }
       
return 0 ;
}



int streamdata(int firstyes,  float *volts, int ndat,  int ichan, struct timeval * btime)
{
char mstr[581] ;
sampl_t         *dataptr ;
int i ;
int curpos, oldpos, pos ;
int res ;
int nchan ;
int dataoffset ;
int width ;
int nsamplesoldpos ;
int nsamplescurpos ;
struct timeval stime, ctime ;
long int usec, basetime ;
int  tcurpos ;
int  oldchan, oldoffchan;
int  curchan, curoffchan ;
long int foldtime ;
int  topos ;



       // the folliwng  three statments have to be atomic //
       // otherwise race condition can occur              //
// to be locked
       curpos  = abuf->offset ;
       stime   = ahead->starttime ;
       oldpos = recordinfo.streamoffset[ichan] ;

       //if first yes == 1
       //first time send all the data //
       if (firstyes)
       {
        topos = (ahead->nchan)*ndat ;
        oldpos = curpos - topos ;
         while (oldpos < 0) { oldpos = BUFFERPAGES*1024 + oldpos ; }
        sprintf(mstr,"Stream topos %d oldpos %d", topos,  oldpos ); 
        logmessage("STREAM", mstr);
       }
       nchan = ahead->nchan ;

         tcurpos = curpos ;
        if (curpos < oldpos) 
        {
          tcurpos=tcurpos+(BUFFERPAGES*1024) ;
#ifdef DEBUG
          logmessage("STREAM", "folding time here ");
#endif 
        } 

        
        oldchan = oldpos%nchan ;
        nsamplesoldpos = (oldpos-oldchan)/nchan ;
        curchan =  tcurpos%nchan  ;
        nsamplescurpos = (tcurpos-curchan)/nchan ;

        oldoffchan = ichan - oldchan ;
        if (oldoffchan)
        { 
         oldoffchan = oldoffchan - nchan ;
         nsamplesoldpos -= 1 ;
        } 

        curoffchan = ichan - curchan ;
        if( curoffchan > 0 ) 
        { 
            curoffchan = curoffchan  - nchan ;
            nsamplescurpos -= 1 ;
        } 
        
       if (curpos == oldpos)  return 0 ;

       width = nsamplescurpos - nsamplesoldpos ;
       
       if (firstyes)
       {
        sprintf(mstr,"Stream begin  data size %d memsize %d", width/ahead->nchan, nvolts ); 
        logmessage("STREAM", mstr);
       } 


       //make the start time here 
       basetime = stime.tv_sec - stime.tv_sec%2 ;
       stime.tv_sec -= basetime ;
       usec = stime.tv_sec*1000*1000 + stime.tv_usec ;
           foldtime = 0 ;
       if (curpos < oldpos )
       {
         foldtime = (int)((BUFFERPAGES+1)/(ahead->nchan)*1024)*
                     ahead->sampling*1000;
       }

       usec = usec + (int)nsamplesoldpos*ahead->sampling*1000-foldtime ;
       ctime.tv_usec = (usec)%(1000*1000) ;
       ctime.tv_sec = (usec - ctime.tv_usec)/(1000*1000) ;
       ctime.tv_sec += basetime ;
       if ( ctime.tv_usec < 0 )
       { ctime.tv_sec -= 1 ; ctime.tv_usec = 1000*1000 - ctime.tv_usec; }
       //starttime there


//       if ( ndat < width*4 ) 
//       { logmessage("STREAM", "not enough memeory in volts "); return 0; }



       if(volts )
       {
         pos = oldpos+oldoffchan ;
         i = 0 ;
         while ( i  < width )
         {
           dataoffset = pos ;
           if (pos > BUFFERPAGES*1024)
           dataoffset = pos - BUFFERPAGES*1024 ;

//to be locked
           dataptr = abuf->buf + dataoffset ;
           *(volts+i) = ahead->adclowervalue + ahead->ctovolts*(*dataptr);
          pos+=nchan;  i++;
         }

       }

     
      recordinfo.streamoffset[ichan]  = curpos%(BUFFERPAGES*1024) ;
      *btime = ctime ;

#ifdef DEBUG
       sprintf(mstr,"time :%d: :%d: width %d ", btime->tv_sec, btime->tv_usec, width); 
       logmessage("STREAM",mstr);
#endif

return width ;
}

int writestream(streams *stp, float *volts, int width, struct timeval btime)
{
char tmes[17] ;
char mstr[581];
int ret ;
int sock ;
float sampling ;
int ct ;
int nt ;
extern int errno ;

   
    if(width == 0 ) return 0 ;

   if (stp == NULL) 
   {  logmessage("STREAM","stp somehow is zero " ); return 0 ; }
   

    sock = stp->sock ;

    sampling=ahead->sampling ;
    yessigpipe = 0;
    memcpy(tmes, "TIME", 4);
    memcpy(tmes+4, &btime, 8);
    memcpy(tmes+12, &sampling, 4);
    ret = send(sock, tmes, 16, 0);
    if (yessigpipe == 1 ) return -1 ;

   if(0)
   {
    /* split the data to 1K packets and loop till all are sent */
    /* sending all the data at once is not debugged properly */
    ct = 0 ;
    while ( (ct < width*4) && (yessigpipe == 0 ) )
    {
      yessigpipe = 0;
      nt = 1024*4;
      ret = send(sock, (volts+(ct/4)), nt, 0);
      ct += ret ;
#ifdef DEBUG
        sprintf(mstr,"sent bytes %d %d\n", ret, errno);
        logmessage("STREAM",mstr);
#endif
    }
      if ( ct < width*4)
      {
        logmessage("STREAM","data to stream too large " );
      }
      if (yessigpipe == 1 ) 
      {
         return -1 ;
      }
   }

   if(1)
   {
      yessigpipe = 0;
      ret = send(sock, volts, width*4, 0);
      if (yessigpipe == 1 ) return -1 ;

     if ( ret != width*4)
     {
        logmessage("STREAM","data to stream too large " );
     }
   }

return ret  ;
}


// remove the stream out of the queue //
streams *dropstream( streams *stp, int ichan ) 
{
streams *stp1, *stp2 ;

  // only one entry //
  if ( (stp->prev == NULL) &&
       (stp->next == NULL)
     )
  {
    free(stp) ;
    return  NULL ;
  }
  // first  entry //
  if (stp->prev == NULL)
  {
    stp1 = stp->next ;
    stp1->prev = NULL ;
    free(stp) ;
    recordinfo.stream[ichan] = stp1 ;
    return stp1 ;
  }
  // last  entry //
  if (stp->next == NULL)
  {
    stp1 = stp->prev ;
    stp1->next = NULL ;
    free(stp) ;
    return stp1 ;
  }

   stp1 = stp->prev ;
   stp2 = stp->next ;
   stp1->next = stp2 ;
   stp2->prev = stp1 ;
   free(stp) ;
   return stp2 ;
}  

// free the memory of the streams //
int streamend()
{
int i, j ;
streams *stp, *stp1 ;

       for ( i=0 ; i < recordinfo.nchan ; i++ )
       {
               stp = recordinfo.stream[i] ;
           for (j=0; j < recordinfo.nstream[i]; j++ )
           {
              if( stp != NULL)
              {
                close(stp->sock);
                stp1 = stp->next ;
                free(stp) ;
                stp = stp1;
              }
              else
              {
                logmessage("STREAM", "unexpected null stp ");
              }
           }
         close(recordinfo.streamsock[i]) ;
       }
}

int streamadd(int csock, int ichan, struct sockaddr_in client)
{
char mstr[581] ;
streams *stp , *stpnew;
int i ;

  if ( recordinfo.nstream[ichan] == MAXSTREAM ) 
  {
     logmessage("STREAM", "no more stream possible ");
     close(csock);
     return -1 ;
  }

    stpnew = (streams *)malloc(sizeof(streams));
    if (stpnew == NULL)
    {
     logmessage("STREAM", "malloc error ");
     return -1 ;
    }

    i = 0 ;
    stp = recordinfo.stream[ichan] ;
    while ( i < recordinfo.nstream[ichan] )
    {
      if(stp != NULL )
      {
         if (stp->next == NULL )
         {
          //sprintf(mstr,"i value stp stpnext %d %d %d\n", i, stp, stp->next);
          //logmessage("STREAM", mstr);
          stpnew->prev = stp ;
          stpnew->next = NULL ;
          stpnew->sock = csock;
          stpnew->firstyes = 1 ;
          stpnew->client = client;
          stp->next = stpnew ;
         }
         else stp = stp->next ;
       i++ ;
      }
    }

    if ( i == 0 ) 
    {
      // first one 
      stp = stpnew ;
      stp->prev = NULL ;
      stp->next = NULL ;
      stp->sock = csock;
      stp->firstyes = 1;
      stp->client = client;
      recordinfo.stream[ichan] = stp  ;
//to be locked
      recordinfo.streamoffset[ichan] = abuf->offset  ;
      i = 1 ;
    }
      recordinfo.nstream[ichan]++ ;

      sprintf(mstr,"Now %d streams on channel %d ",
                            recordinfo.nstream[ichan], ichan);
      logmessage("STREAM", mstr);
 return 0 ;
}

void streammain(void *arg)
{
char mstr[581];
int i, j ;
streams *stp ;
int ssock ;
int csock ;
struct sockaddr_in client;
socklen_t  clen ;
int nchan ;
struct timeval timeout ;
struct timeval btime ;
int activesock ;
int activechan ;
int  ret ;
int  ndat ;
struct pollfd ufds[MAXCHAN];
int firstyes ;

    // set the server port numbers //
    serversettings();

    //open the stream ports for all the channels
     nchan = ahead->nchan; 
    for (i=0; i < nchan; i++ )
    {
        recordinfo.stream[i] = 0 ;
        recordinfo.nstream[i]  = 0 ;
        recordinfo.streamsock[i] = 0 ;
        openstreamsock(i);
    }


  // stream till the record  program is closed//
  while( recordstatus != STOPRECORD )
  {
         sleep(1);

        for (i=0; i < nchan; i++ )
        {
            ufds[i].fd = (recordinfo.streamsock[i]);
            ufds[i].events = POLLIN | POLLOUT | POLLERR  | POLLPRI ;
            ufds[i].revents =  0;
        }

        ret = poll(ufds, nchan, 3000);

       if (ret)
       {
          for (i=0; i < nchan; i++ )
          {
            if(ufds[i].revents & POLLIN)
            {
              //add this client to the streams queue//
              activesock = recordinfo.streamsock[i] ;
              activechan = i ;
              sprintf(mstr, "connection requested for chan %d ", i); 
              logmessage("STREAM", mstr);
              clen = sizeof(client);
              csock = accept(activesock, (struct sockaddr *)&client, &clen);
              if (csock ) streamadd(csock, activechan, client);
            }
          }
       }

         
        // stream the data to the clients // 
        for (i=0; i < nchan; i++ )
        {
           if ( recordinfo.nstream[i] )
           {
             j = 0 ;
             stp =  recordinfo.stream[i] ;
                 firstyes =  stp->firstyes;
                 ndat = streamdata(firstyes, volts, nvolts, i, &btime);
                 if (firstyes == 1 )  stp->firstyes = 0 ;
             while ( j < recordinfo.nstream[i] ) 
             {
               if (writestream( stp, volts, ndat, btime) < 0 )
               {
                  stp = dropstream(stp, i);
                  recordinfo.nstream[i]-- ;
                  sprintf(mstr,"Now %d streams on channel %d ",
                    recordinfo.nstream[i], i);
                  logmessage("STREAM", mstr);
               }
               else
               {
                stp = stp->next ;
                j++ ;
               }

             } //while j loop

           } // nstream check
        } //for loops

  }// while recordstatus ends here //

    streamend();
}

void sigpipecatch(int num)
{
  yessigpipe = 1 ;
}
// open a thread  to run streamdata                    //
// this thread is terminated if the client stops       // 
// reading the stream port                             //
int  initthreadstream()
{
int ret ;


         signal(SIGPIPE, sigpipecatch);

         ret = pthread_create( &threadstream, NULL, (void *)&streammain, NULL);
         if (ret ) {
          logmessage("STREAM", "Could not create streamdata  thread ");
         }

 return 0 ;
}

