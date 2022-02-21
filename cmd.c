/*  CMD PACKAGE
**  Written by S. Jeyakumar 28-10-2006

**  defines routines used by record, scan, getonline
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include <arpa/inet.h>


#include <sys/socket.h>
#include <netinet/in.h>

#ifndef __LOGMESSAGE__
#define __LOGMESSAGE__
int logmessage(char *fstr, char *str)
{
 fprintf(stderr,"%s: %s", fstr, str);
}
#endif

int socketcmdopenloop(char *prog, int port)
{
char str[181];
int                  sock ;
struct sockaddr_in  sockserver;
struct in_addr      sockserverip ; 
int ret ;
int rec ;
          
 
       if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) 
       {
          sprintf(str,"%s: socket client: socket error\n",prog); 
          logmessage("CMD", str);
          exit(-1) ;
       }
       memset(&sockserver, 0, sizeof(sockserver)); 
       sockserver.sin_family = AF_INET ;
       if(!inet_aton(recordserverip, &sockserverip) )
       {
          sprintf(str,"%s: socket client: server IP error\n",prog); 
          logmessage("CMD", str);
          exit(-1);
       }
       sockserver.sin_addr = sockserverip ;
       sockserver.sin_port = htons(port);
       ret = connect(sock, (struct sockaddr *)&sockserver, sizeof(sockserver));

       if ( ret < 0)
       {
        // no record running 
        sock  = -1 ;
       } 

       if ( ret == 0)
       {
         // record running 
         // do nothing
       } 
               
  return sock ;
}


int socketcmdopen(char *prog, int port)
{
char str[181];
int                  sock ;
struct sockaddr_in  sockserver;
struct in_addr      sockserverip ; 
int ret ;
int rec ;
          
 
       if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) 
       {
          sprintf(str,"%s: socket client: socket error\n",prog); 
          logmessage("CMD", str);
          exit(-1);
       }
       memset(&sockserver, 0, sizeof(sockserver)); 
       sockserver.sin_family = AF_INET ;
       if(!inet_aton(recordserverip, &sockserverip) )
       {
          sprintf(str,"%s: socket client: server IP error\n",prog); 
          logmessage("CMD", str);
          exit(-1);
       }
       sockserver.sin_addr = sockserverip ;
       sockserver.sin_port = htons(port);
       ret = connect(sock, (struct sockaddr *)&sockserver, sizeof(sockserver));

       if ( ret < 0)
       {
        // no record running 
          sprintf(str,"%s:  record is not found\n", prog); 
          logmessage("CMD", str);
            exit(-1);
       } 

       if ( ret == 0)
       {
         // record running 
         // do nothing
       } 
               
  return sock ;
}


// send the command                  //
int socketcmdsend( int sock, char *mes, int len)
{
char str[181];
int rec ;


   rec = send(sock, mes, len, 0);
   if ( rec != len)
   {
       sprintf(str,"SCAN: error while sending command to record\n" );
          logmessage("CMD", str);
        exit(-1);
   }
   else
   {
// reset the input string
    memset(mes,0, len);
   }
return   rec ;
}


int  socket_recv(char *prog, int port)
{
int                 ssock ;
struct sockaddr_in  sockserver ;
struct in_addr      sockserverip ;
int ret ;

      if ((ssock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
      {
       logmessage("CMD", " socket server: socket error\n" );
       exit(-1);
      }

       memset(&sockserver, 0, sizeof(sockserver));
       sockserver.sin_family = AF_INET ;
       if(!inet_aton(recordserverip, &sockserverip) )
       {
         logmessage("CMD", "socket server: server IP error\n" );
         exit(-1);
       }
       sockserver.sin_addr = sockserverip ;
       sockserver.sin_port = htons(port);
       ret = bind(ssock, (struct sockaddr *)&sockserver, sizeof(sockserver));
       if (ret < 0 )
       {
         logmessage("CMD", "socket server: bind error\n" );
         exit(-1);
       }
       ret = listen(ssock, 5) ;
       if (ret < 0 )
       {
          logmessage("CMD", "socket server: listen error\n" );
          exit(-1);
       }

 return ssock ;
}

int socket_read_data(int sock,  char *buff, int buffsize )
{
int rec ;
      rec = -1 ;
      memset(buff,0,buffsize);
      rec = recv(sock, buff, buffsize, 0) ;
      //rec = read(sock, buff, buffsize) ;
return rec ;
}

int socket_recv_data(int ssock,  char *mes, int ndat )
{
char str[181] ;
int csock ;
int ret, rec ;
socklen_t clientlen ;
struct sockaddr_in  client;
int go ;


     clientlen = sizeof(client);
     go = 1 ;
     while (go)
     {
        memset(&client, 0, sizeof(struct sockaddr_in));
        csock = accept(ssock, (struct sockaddr *)&client, &clientlen);
        if (csock < 0 )
        {
          logmessage("CMD" ," socket server: connetion accept error\n" );
          exit(-1);
        }

      //get the message 
        rec = -1 ;
        memset(mes,0,ndat);
        rec = recv(csock, mes, ndat, 0) ;
        close(csock);
        if (rec < 0)
        {
           logmessage("CMD" , "socket server:  message receive error\n");
           go = 0 ;
        }

        if (rec > 0)
        {
           sprintf(str, "recived bytes %d", rec );
           logmessage("CMD" , str);
        }

     }
     close(ssock);
     logmessage("CMD", "socket server:  exiting thread\n");
}
