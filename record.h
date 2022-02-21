/*  RECORD HEADER
**  Written by S. Jeyakumar 28-10-2006

**  defines  all variables for record
*/


#ifndef _RECORD_H
#define _RECORD_H


int recordstatus ;

#define STARTRECORD  1
#define STOPRECORD   2
#define STATUSRECORD 3

#define MAXSTREAM   5



mexartheader dataheader ;


pthread_t   threadrecord ;
pthread_t   threadserver;
pthread_t   threadstream;
pthread_t   threadclient;

//linked list for streaming //

 struct  stream_sock {
          struct   sockaddr_in  client ;
          int      sock ;
          int      firstyes ;
          struct   stream_sock *prev ;
          struct   stream_sock *next ;
         } ;

typedef struct stream_sock  streams ;

typedef struct {
           char  source[MAXCHAN][81] ;
           char  fname[MAXCHAN][581] ;
           char  cmd[MAXCHAN] ;
           int   fd[MAXCHAN] ;
           int   nchan ;
  struct timeval begintime[MAXCHAN] ;
  struct timeval endtime[MAXCHAN] ;
           int   channelstat[MAXCHAN] ;
           int   recno[MAXCHAN] ;
           int   curscan[MAXCHAN] ;
           int   oldscan[MAXCHAN] ;

           int     streamoffset[MAXCHAN] ;
           int     streamsock[MAXCHAN] ;
           int     nstream[MAXCHAN] ;
           streams *stream[MAXCHAN] ;
         } recinfo ;



typedef struct {
           char command ;
           int  channel ;
           int  begintime ;
           int  endtime ;
           char source[181] ;
         } scancmd ;


scancmd  scancommand ;  

recinfo recordinfo;


#define RECORDING     1
#define STREAMING     2


acqhead   *ahead ;
acqbuf    *abuf ;


int serverok = 0 ;
int  recordstreamport[MAXCHAN]  ;

char currentdir[181] ;
char recordlogfilename[581] ;
char recordinpfilename[581] = "acq.inp";
FILE  *reclogfp ;

int  yessigpipe ; 



float *volts ;
int   nvolts ;

/* copy of acq variables  */
int     copyacqoffset;
int     copyacqstat;
struct timeval copyacqstime ;


#endif  /* _RECORD_H */
