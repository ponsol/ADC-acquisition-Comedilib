/*  ACQ HEADER
**  Written by S. Jeyakumar 28-10-2006

**  defines  all variables for acq
*/




#ifndef _ACQ_H
#define _ACQ_H
/* ALL GLOBALS HERE */
/* control variables */

int acqstatus = STARTACQ ;
/* general */
char acqinpfilename[581] = "acq.inp" ;
char acqlogfilename[581] ;

FILE  *acqlogfile ; 
FILE  *acqinpfile ;


/* device */
char            devname[] =  "/dev/comedi0" ;
comedi_t        *dev ;
unsigned int    subdev=0 ; 
//unsigned int    aref = AREF_GROUND; 
unsigned int    aref = AREF_DIFF; 
unsigned int    frange;
int             maxdata ;
comedi_range    *frangeval ; 
int             buffersize ;
void            *buffermap ;
comedi_cmd      fcmd ;
comedi_cmd      ffcmd ;


/* Default obs parameters */
int            nsample = 512+4 ;
float          sampling = 10 ;  /* in milli seconds */

#define MAXCHAN 16
unsigned int    listofchan[MAXCHAN] = {0} ; 
unsigned int    nlist = 1;
unsigned int    chanlist[MAXCHAN];

/* ALL GLOBALS ABOVE */

#endif  /* _ACQ_H */
