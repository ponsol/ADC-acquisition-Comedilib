#ifndef __GETONLINE_H
#define __GETONLINE_H


#include <sys/stat.h>
#include <fcntl.h>

#include "server.h"

#define  SOCKETOPEN  2
#define  STREAMING   1
#define  MAXCHAN    64
#define  STOPPLOT    1

// program control parameters
char   proname[81] ;
int    prooption ;

// record server parameters
typedef struct {
     int openagain ;
     int filereset ;
     int RecordServerSock ;
     int RecordNchan ;
     int RecordCchan ;
     int RecordChannels[MAXCHAN] ;
} _onlineserverpar ;

_onlineserverpar  Rserver ;

// writeonline paramters 
//char   onlinedir[] = "/database/actual" ;
static mode_t fm = S_IWUSR| S_IRUSR | S_IWGRP| S_IRGRP | S_IROTH | S_IWOTH ;



// the following to hold the plotting channels
typedef struct {
     int      pnchan ;
     int      pchannels[MAXCHAN];
     int      datasize ;
     int      buffersize ;
     float    *buffer ;
     float    *timeptr ;
     float    sampling ;
     int      stopplot ;
     int      dataready ;
   struct timeval writetime ;
}  _onlineclientpar ;

_onlineclientpar  Rclient ;


typedef struct  {
           FILE  *fp ;
           int   fd ;
           int   streamsock ;
           int   fillsize ;
           int   status ;
           int   offset ;
           float zeroref ;
           float *dataptr ;
           float *timeptr ;
           float *lstptr ;
           float rms ;
           float mean ;
           float totmean ;
           float totrms ;
           float minmean ;
           float maxmean ;
           float minrms ;
           float maxrms ;
           float etime ;
           float xmin, xmax, ymin,ymax ;
          } _pdatstruct ;


_pdatstruct  pdat[MAXCHAN] ;

// plot control variables
typedef struct {
   float    timeinterval   ;
   float    drate   ;
   float   sfactor   ;
   int     nrunmean  ;
   int     nmeandiv  ;
   int     readjust  ;
   int     plotlock  ;
   float   zeroref  ;
   float   dmin  ;
   float   dmax  ;
   float   xmin  ;
   float   xmax  ;
} _plotpar ;


FILE *getonlinelogfp ;
//pthread_t   threadPlotReceive;
//pthread_t   threadRecordServer;

//pthread_t   threadPlotMain;
//pthread_t   threadPlotInput;

static mode_t filemode = S_IWUSR| S_IRUSR | S_IWGRP| S_IRGRP | S_IROTH ;

#endif
