/*  SCANMAIN PACKAGE
**  Written by S. Jeyakumar 28-10-2006

**  aimed at command line interface for acquisition
**  interacts with record  over the network
**  to start/stop recording channels to the disk 

**  See scan package for usage
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include  "server.h"
#include  "cmd.h"

#include  "cmd.c"



int  scanparsearguments(int argc, char *argv[])
{
char source[81] ;
int i ;
int ct ;
int ival ;
int beghr, begmin, begsec ;
int endhr, endmin, endsec ;


           memset(source,0,80);

    if ( argc == 1 )
    {
      return -1 ;
    }

    if ( argc >= 3 )
    {
       if( 
          (strstr(argv[1],"dump") != NULL ) ||
          (strstr(argv[1],"end") != NULL )  ||
          (strstr(argv[1],"stream") != NULL ) 
         )
       {
         if( (ct=sscanf(argv[2],"%d", &ival)) == 1 )
         {
           sprintf(message,"%s %d", argv[1], ival);
           return 0 ;
         }
         else
         {
           return -1;   
         }

       }

       if( (strstr(argv[1],"begin") != NULL ) &&
           (argc >= 3) 
         )
       {
              strncpy(source,"TMSERIES",8);
         if( (ct=sscanf(argv[2],"%d", &ival)) == 1 )
         {
            if( argc >= 4 )
            if((ct=sscanf(argv[3],"%8s", source)) != 1 )
            {
              strncpy(source,"TMSERIES",8);
            }
           sprintf(message,"begin %d %s", ival, source);
           return 0 ;
         }
         else
         {
           return -1;   
         }
       }



       if( (strstr(argv[1],"scan") != NULL ) && 
           (argc == 5)
         )
       {
           if( 
              ((ct=sscanf(argv[2],"%d", &ival)) == 1 )  &&
              ((ct=sscanf(argv[3],"%d", &endmin)) == 1 ) 
             )
             {
               
                if( (ct=sscanf(argv[4],"%8s", source)) == 1 )
                {
                 sprintf(message,"%s %d %d %s", 
                                    argv[1], ival, 
                                    endmin, source);
                 return 0 ;
                }
                else { return -1;   }
             }
             else { return -1 ; }
       }


    }

  return -1 ;
}




int scanmain(int argc, char *argv[])
{
int sock ;
     if ( scanparsearguments(argc, argv) < 0  ) return -1 ;
     sock = socketcmdopen("SCAN", recordcmdport);
     socketcmdsend(sock, message, 180);
 return 0 ;
}
