/*  ACQ COMMON HEADER
**  Written by S. Jeyakumar 28-10-2006

**  defines all variables common between acq and record
*/

#ifndef _ACQCOMMON_H
#define  _ACQCOMMON_H


#ifndef _COMEDI_H
typedef unsigned short sampl_t;
#endif



#define STARTACQ  1
#define STOPACQ   2
#define STATUSACQ 3


// common variables between record and acq//
// shared memory variables                //
int       shmid1, shmid2 ;
key_t     shmkey1 = 0x41 ;
key_t     shmkey2 = 0x42 ;

// The BUFFERPAGES should be in multiples of nchan //
//#define        BUFFERPAGES  24
#define        BUFFERPAGES  120
int            shmsize      =  (BUFFERPAGES*1024*2);
// 8K integers for overflow and 2 integers for staus and offset //
int            totalshmsize =  (BUFFERPAGES*1024*2) + 4+ (8*1024*2) ;
int            shmparsize = 2*1024 ;


#define     MAXCHAN  16

typedef struct {
         int             acqstatus ;
         int             locked ;
         struct timeval  starttime ;
         float           sampling ;
         unsigned int    nchan ;
         unsigned int    channels[MAXCHAN] ;
         int             adclowervalue ;
         double          ctovolts ;
         char            acqlogfilename[581] ;
} acqhead ;


typedef struct {
         int     stat ;
         int     offset ;
         sampl_t buf[] ;
} acqbuf ;

acqhead *ahead ;


acqbuf  *abuf ;

static mode_t dirmode = S_IRWXU | S_IRWXG | S_IXOTH | S_IROTH ;
static mode_t filemode = S_IWUSR| S_IRUSR | S_IWGRP| S_IRGRP | S_IROTH ;

char recorddir[581] = "./" ;
#endif   /*  _ACQCOMMON_H */
