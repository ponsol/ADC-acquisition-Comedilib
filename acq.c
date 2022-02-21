/*  ACQ PACKAGE
**  Written by S. Jeyakumar 28-10-2006
**
**  All routines for acqusition
**  Uses COMEDI command routines
**  
**  Time keeping does not work properly for 
**  sampling rate smaller than  0.1 millisecond 
**  needs clean up of time keeping 
*/

#include <stdio.h>
#include <comedilib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <stdlib.h>

#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>



#include "acqcommon.h"
#include "acq.h"


//#define DEBUG 

int initcard()
{
int ret ;
int bs ;


	dev = comedi_open(devname);
	if(!dev){
		comedi_perror(devname);
		exit(1);
	}
   fprintf(acqlogfile,"ACQ:  device open successful\n");


/* set the buffer size here */
    ret = comedi_get_max_buffer_size(dev,  subdev);
    fprintf(acqlogfile,"ACQ:  Maximum buffer size %d\n", ret);
    ret = comedi_get_buffer_size(dev,subdev);
    fprintf(acqlogfile,"ACQ:  Old buffer size %d\n", ret);
    bs = 64*1024 ;
    //bs = 2*(5000/sampling)*nlist ;
    //if (bs < 8*1024) bs=8*1024 ;
    //if (bs > 64*1024) bs=64*1024 ;
    //fprintf(acqlogfile,"ACQ:  sampling %d nlist %d new buffer size %d\n", sampling, nlist, bs);
    //ret = comedi_set_buffer_size(dev,  subdev, bs);
    //fprintf(acqlogfile,"ACQ:  New buffer size %d\n", ret);
}


int initchanlist()
{
comedi_range *rangeval ; 
int ret, i ;
int nchan ;
int chan ;

        ahead->sampling = sampling ;

        for(i=0; i < nlist; i++)
        {
          chanlist[i]=CR_PACK(listofchan[i], frange,aref);
          ahead->channels[i] = listofchan[i] ;
        }
        nchan = nlist ;
        ahead->nchan = nlist ;
    fprintf(acqlogfile,"ACQ: Number of channels %d\n", nchan);
    fprintf(acqlogfile,"ACQ: Channels: ");
    for(i=0; i < nlist; i++)
    { 
    fprintf(acqlogfile," %d", listofchan[i]);
    }
    fprintf(acqlogfile,"\n");
    fprintf(acqlogfile,"ACQ: Sampling rate                 : %g\n", 
                                                ahead->sampling);

   chan = 0 ;
   ret= comedi_get_n_ranges(dev, subdev, chan);
   fprintf(acqlogfile,"ACQ: Number of Ranges: %d\n", ret);
   for ( i = 0 ; i < ret ; i++ )
   {
     rangeval = comedi_get_range(dev, subdev, chan, i);
     fprintf(acqlogfile,"ACQ: Parameters of range %d is: %f %f %d\n", 
     i, rangeval->min , rangeval->max, rangeval->unit );
   }
   frange = 0 ;
   fprintf(acqlogfile,"ACQ: Selected Range                : %d\n", frange);
   rangeval = comedi_get_range(dev, subdev, chan, frange);

   maxdata=comedi_get_maxdata(dev,subdev,chan);


   frangeval = rangeval ;

      ahead->ctovolts  = (frangeval->max -frangeval->min)/maxdata ;
      ahead->adclowervalue   = frangeval->min ;
   fprintf(acqlogfile,"ACQ: Selected Range mininum data   : %d\n", ahead->adclowervalue);
   fprintf(acqlogfile,"ACQ: Selected Range count to volts : %g\n", ahead->ctovolts);



   return 0 ;
}


int initcommand()
{
comedi_cmd *cmd  = &fcmd ;

	memset(cmd,0,sizeof(*cmd));

	cmd->subdev =	subdev;

	cmd->flags = 0;

	cmd->flags |= TRIG_DITHER ;
	//cmd->flags |= TRIG_WAKE_EOS;
	
	//cmd->flags |= TRIG_RT;

	cmd->start_src =	TRIG_NOW;
	cmd->start_arg =	0;

	cmd->scan_begin_src =	TRIG_TIMER;
	cmd->scan_begin_arg =	(int)(sampling*1000*1000) ;


	cmd->convert_src =	TRIG_TIMER;
	//cmd->convert_src =	TRIG_NOW;
	//cmd->convert_arg =	3276750 ;
	 cmd->convert_arg =	5000 ;

	cmd->scan_end_src =	TRIG_COUNT;
	cmd->scan_end_arg =	nlist;		

	cmd->stop_src =		TRIG_NONE;
	cmd->stop_arg =		0;

	cmd->chanlist =		chanlist;
	cmd->chanlist_len =	nlist;


/*
        cmd->data     = buffer ;
        cmd->data_len = nsample ;
*/


	return 0;
}





int start_acq()
{
comedi_insn setupins[3];
lsampl_t pars[10];
comedi_insnlist il;
int ret ;
int chan ;

        chan = listofchan[0] ;
        il.n_insns=1;
        il.insns=setupins;

        /* Instruction 0: perform a gettimeofday() */
        setupins[0].subdev=subdev;
        setupins[0].chanspec=CR_PACK(chan,frange,aref);
        setupins[0].insn=INSN_CONFIG;
        setupins[0].insn=INSN_GTOD;
        setupins[0].n=2;
        setupins[0].data=pars ;

             ret=comedi_do_insnlist(dev,&il);
        if(ret<0){
                fprintf(acqlogfile,"ACQ: ERROR in instructions %d\n", ret);
                exit(0);
        }
        else
        {
             //   fprintf(errorfile,"ACQ: time %s \n", ctime(&pars[0]));
        }

}

int close_memory(FILE *fp)
{
struct shmid_ds ds1, ds2 ; 

     if (fp) fprintf(fp,"ACQ:  Delete the shared memory\n"); 
     shmctl(shmid1, IPC_RMID, &ds1);
     shmctl(shmid2, IPC_RMID, &ds2);
     if (fp) fprintf(fp,"ACQ:  Clean stop\n");
}



int stop_acq(comedi_t *dev)
{
int ret ;
struct shmid_ds ds1, ds2 ; 
    if (comedi_close(dev) < 0 )
    { 
     fprintf(acqlogfile,"ACQ:  Device close error %d\n", ret);
     exit(0);
    } 
     comedi_cancel(dev,subdev);
     close_memory(acqlogfile);
}

int checkcommand()
{
int ret;

 ret = comedi_get_cmd_generic_timed(dev,subdev,&ffcmd,1e7);
        if(ret<0){
                fprintf(acqlogfile,"ACQ:  comedi_get_cmd_generic_timed failed\n");
                return ret;
        }
/*
   ret = comedi_get_cmd_src_mask(dev, subdev, &ffcmd);
*/

   ret = comedi_command_test(dev, &fcmd);
   fprintf(acqlogfile,"ACQ:  cmd test return:  : %d\n", ret);

   fprintf(acqlogfile,"ACQ:  cmd flags:        : %d %d\n", 
                     fcmd.flags, ffcmd.flags);
   fprintf(acqlogfile,"ACQ:  cmd start_src:    : %X %X\n", 
                     fcmd.start_src, ffcmd.start_src);
   fprintf(acqlogfile,"ACQ:  cmd start_arg:    : %d %d\n", 
                  fcmd.start_arg, ffcmd.start_arg);
   fprintf(acqlogfile,"ACQ:  cmd scan_begin_src: %X %X\n", 
                     fcmd.scan_begin_src, ffcmd.scan_begin_src);
   fprintf(acqlogfile,"ACQ:  cmd scan_begin_arg: %d %d\n", 
                  fcmd.scan_begin_arg, ffcmd.scan_begin_arg);
   fprintf(acqlogfile,"ACQ:  cmd covert_src:   : %X %X\n", 
                     fcmd.convert_src, ffcmd.convert_src);
   fprintf(acqlogfile,"ACQ:  cmd convert_arg:  : %d %d\n", 
                  fcmd.convert_arg, ffcmd.convert_arg);
   fprintf(acqlogfile,"ACQ:  cmd scan_end_src: : %X %X\n", 
                     fcmd.scan_end_src, ffcmd.scan_end_src);
   fprintf(acqlogfile,"ACQ:  cmd scan_end_arg: : %d %d\n", 
                  fcmd.scan_end_arg, ffcmd.scan_end_arg);
   fprintf(acqlogfile,"ACQ:  cmd stop_src:     : %X %X\n", 
                     fcmd.stop_src, ffcmd.stop_src);
   fprintf(acqlogfile,"ACQ:  cmd stop_arg:     : %d %d\n", 
                  fcmd.stop_arg, ffcmd.stop_arg);

 return ret ;
}

int readsetupfile( char *inpfile )
{
char *st ;
char line[181];
char str[181] ;
char tstr[581] ;
char *sptr ;
int ival ;
float fval ;
char *c ;
time_t curt ;
struct tm *lttime ;
int parcount ;


        parcount = 0 ;
       acqinpfile = fopen(inpfile, "r");
       if (!acqinpfile)
       {
        fprintf(stderr, "Error opening file %s \n", inpfile );
        exit(0);
       }

       fgets(line, 180, acqinpfile);
     while (!feof(acqinpfile) )
     { 

       if( *line != '#' )
       {
         strcpy(str, line); sptr = str ;
         st = strsep(&sptr,"=");

          if ( strstr(st, "samplingrate") != NULL )
          { 
             st = strsep(&sptr,"=");
             if (sscanf(st,"%f", &fval) ) 
             {
              sampling = fval ;
              parcount++ ;
             }
          }
          if ( strstr(st, "acqlogfile") != NULL )
          { 
             st = strsep(&sptr,"=");
             parcount++ ;
             if ( sscanf(st,"%s", acqlogfilename) != 1 )
             {
              strcpy(acqlogfilename, "acq.log");
              parcount-- ;
             }
          }

          if ( strstr(st, "recorddir") != NULL )
          {
             st = strsep(&sptr,"=");
             if (sscanf(st,"%s", tstr) == 1 )\
             {
               parcount++ ;
               strcpy(recorddir,tstr);
             }
          }


          if ( strstr(st, "channels") != NULL )
          { 
             nlist = 0 ;
             while( sptr != NULL)
             {
              st = strsep(&sptr,",");
              if (sscanf(st,"%d", &ival) ) 
              { listofchan[nlist] = ival ; nlist++ ; }
             }
             if (nlist > 0 ) parcount++ ;
              

          }
       }
       fgets(line, 180, acqinpfile);
     }

     if ( parcount != 4 )
     {
       fclose(acqinpfile);
       fprintf(stderr,"ACQ: Error in input file %d\n", parcount); 
       exit(0);
     }

     strcpy(tstr, recorddir);
     strcpy(tstr+strlen(tstr), "/");
     strcpy(tstr+strlen(tstr), acqlogfilename);
     strcpy(acqlogfilename,tstr);

    curt = time(0);
    //tmt = gmtime(&curt);
    c = ctime(&curt);
    //sprintf(str, "%2s%3s%4s/", c+8, c+4, c+20 );
    memset(str, '\0', 181);

//    strncpy(str,c+8,2);
//    if (*str == ' ') *str='0' ;
//    strncpy(str+strlen(str),c+4,3);
//    strncpy(str+strlen(str),c+20,4);


    lttime =  localtime(&curt);
    sprintf(str, "%04d-%02d-%02d",
                       lttime->tm_year+1900,
                       lttime->tm_mon+1,
                       lttime->tm_mday );


     strcpy(acqlogfilename+strlen(acqlogfilename), ".");
     strcpy(acqlogfilename+strlen(acqlogfilename), str);

}

int bufferoverflow(int flag)
{
   fprintf(acqlogfile,"ACQ: ERROR : shared memory buffer overlow \n");
   if (flag == 2 )
   fprintf(acqlogfile,"ACQ: ERROR : shared memory too small  \n");
   if (flag == 1 )
   fprintf(acqlogfile,"ACQ: ERROR : bug \n");
}

struct timeval getscantime(int off, struct timeval *curtime, long int *diffmilli )
{
struct timeval  curtimeold ;
struct timeval starttime ;

int nchan ;
long int  basetime ;
unsigned long int  remtime ;
unsigned long int  usec;
int stat ;
float samplingrate ;
           
           samplingrate = ahead->sampling ;

           nchan = ahead->nchan ;
           starttime = ahead->starttime ;
           basetime = starttime.tv_sec - starttime.tv_sec%2 ;

//           stat = (abuf->offset - (abuf->offset)%(nchan*1024))/(nchan*1024) ;
           stat = (off - (off)%(nchan*1024))/(nchan*1024) ;
           remtime = (off)%(nchan*1024)/(nchan) ;
           remtime *=  (int)samplingrate*1.E3 ;
           curtime->tv_sec -= basetime ;
           usec = (curtime->tv_sec*1.E6 + curtime->tv_usec - remtime) ;
           curtime->tv_usec = (long) (usec%(1000UL*1000UL));
           curtime->tv_sec =  ((usec - curtime->tv_usec)/1.E6) ;

           remtime = (int)stat*1024*samplingrate*1.E3 ;
           starttime.tv_sec -= basetime ;
           usec = starttime.tv_sec*1.E6 + starttime.tv_usec + remtime ;
           curtimeold.tv_usec = usec%(1000*1000);
           curtimeold.tv_sec  = (usec - curtimeold.tv_usec)/1.E6 ;
           *diffmilli =  (curtime->tv_sec  - curtimeold.tv_sec )*1000 ;
           *diffmilli += (curtime->tv_usec - curtimeold.tv_usec)/1000 ;
           curtimeold.tv_sec +=  basetime ;
           curtime->tv_sec    +=  basetime ;
           starttime.tv_sec  +=  basetime ;

 return  curtimeold ;
}


int  bufferdatanew()
{
// directly buffer the data in the shared memory.     //
// When the data overflows the shared memory copy     //
// the bytes beyond the boundary to the begining.       // 
// A size of 8K int is reserved for overflowing bytes //
// If 8K is not sufficient do an exit.                //
//
// Well, that is a segmentation fault, so find a way to //
// avoid that //

/* locking mechanism is added to the abuf and ahead variables */
/* ahead locked   =1 means locked by acq   */
/* ahead locked <=0 means unlocked by acq   */
/* if ahead locked < 0, acq is overwriting while some other  */
/* process is accessing the memory                           */
/* this assumes that other processes set to a positive value  */
/*  other than 1                                              */
/* In the next iteration the error status of ahead locked is cleared */

sampl_t  buffer[16*1024] ;
sampl_t  *tbuf1 ;
int  *bstat[2] ;
sampl_t  *ptr[2] ;
int nbytes, rem ;
int nchan ;
struct timeval curtime, starttime, curtimeold ;
int off ;

long int  diffmilli ;


char tstr[25] ;




        nchan = ahead->nchan ;

        abuf->stat = 0 ;
        abuf->offset = 0 ;
        starttime = ahead->starttime ;

    while( ahead->acqstatus != STOPACQ )
    {
      nbytes=read(comedi_fileno(dev),abuf->buf+abuf->offset,16*1024);
      gettimeofday(&curtime,0);
      
      if( nbytes == 0 )
      {  break ; }

      if( nbytes > 0 )
      {
               strncpy(tstr,ctime(&curtime.tv_sec),24);
               tstr[24] = '\0' ;
           fprintf(acqlogfile,"ACQ: %s Read bytes %d\n",tstr, nbytes );
           off = abuf->offset + nbytes/2;
           curtimeold = getscantime(off, &curtime, &diffmilli);

           if ( ahead->locked <= 0) ahead->locked = 1;
           else  ahead->locked *= -1;

           abuf->offset = off;
           if ( 2*off > shmsize )
           {
//      data beyond the boundary  //
            rem = (2*off-shmsize) ;
            memmove(abuf->buf, abuf->buf+shmsize/2, rem) ;
// reset the time to the expected value //
            abuf->offset = rem/2  ;
            ahead->starttime = curtimeold ;
           } 
           if ( abuf->offset == shmsize ) abuf->offset = 0 ;
           abuf->stat = (abuf->offset - (abuf->offset)%(nchan*1024))/(nchan*1024) ;
           if ( ahead->locked == 1) ahead->locked = 0;
    
        
#ifdef DEBUG
           fprintf(acqlogfile,"ACQ:  starttime %d %d \n",
                      starttime.tv_sec, starttime.tv_usec );
           fprintf(acqlogfile,"ACQ:  curtime from starttime %d %d \n",
                      curtimeold.tv_sec, curtimeold.tv_usec );
           fprintf(acqlogfile,"ACQ:  curtime %d %d \n",
                      curtime.tv_sec, curtime.tv_usec );
           fprintf(acqlogfile,"ACQ:  current time is off by: %d (milliseconds)\n",
                                                       diffmilli);
           fprintf(acqlogfile,"ACQ:  Bytes  read %d\n",nbytes );
           fprintf(acqlogfile,"ACQ:  buffer last value %d\n", *(abuf->buf+abuf->offset-1));
           fprintf(acqlogfile,"ACQ:  buffer pointer value %d\n", *(abuf->buf+abuf->offset));
           fprintf(acqlogfile,"ACQ:  buffer status %d\n", abuf->stat );
#endif

         fflush(acqlogfile) ;
      }
       
   }


}

int initfiles()
{
struct timeval curt ;
struct timezone tz;
char tstr[25] ;
/*  global files */

      umask(~filemode);
      acqlogfile  = fopen(acqlogfilename, "a");
     if( !acqlogfile )
     { 
       fprintf(stderr,"ACQ: Could not open acq out file \"%s\" \n",acqlogfilename); exit(0); }
      acqinpfile  = fopen(acqinpfilename, "r");

     if( !acqinpfile )
     { fprintf(stderr,"ACQ: Could not open acq inp file \"%s\"\n", acqinpfilename); exit(0); }

     fclose(acqinpfile);


     gettimeofday(&curt, &tz);
     strncpy(tstr,ctime(&curt.tv_sec),24);
     tstr[24] = '\0' ;
     fprintf(acqlogfile,"\n********* %s **********\n", tstr);

}


int initmemory(int option)
{
// check all the files can be opened  //
// check whether input file exists    //
// open the shared memory and attach //
struct shmid_ds bufds ; 
struct shmid_ds bufshmds;
struct shmid_ds bufshmpards ;
int smid ;
int i ;



//check for running acq 
            fprintf(stderr,"ACQ: checking for running acq .....\n" );
         smid = shmget(shmkey2, shmparsize, 0) ;

       if (smid != -1 )
       {
        //process already running
             ahead =  (acqhead *)shmat(smid, NULL, 0);
             if ( ahead < 0   ) 
             {
               fprintf(stderr,"ACQ: shared memory could not be attached\n");
               exit(0);
             }

          if ( option == STARTACQ )
          {
            fprintf(stderr,"ACQ: acq is already running\n" );
            if ( ahead->acqstatus == STOPACQ )
            {
             fprintf(stderr,"ACQ: but acq was marked to stop\n" );
             fprintf(stderr,"ACQ: so restarting after 10 seconds\n" );
             usleep(10*1000*1000);
             fprintf(stderr,"ACQ:  restarting  now... \n" );
            }
            else {
               shmdt(ahead);
               exit(0);
            }
          }
          if ( option == STOPACQ )
          {

            ahead->acqstatus = STOPACQ;
            shmdt(ahead);
            fprintf(stderr,"ACQ:  sent stop message!\n" ); 
            exit(0);
          }

          if ( option == STATUSACQ )
          {
            fprintf(stderr,"ACQ: acq is already running\n" );
            fprintf(stderr,"ACQ: Logfilename %s\n", ahead->acqlogfilename );

             fprintf(stderr,"ACQ: Acqchannels ");
            for (i = 0; i < ahead->nchan; i++)
            {
             fprintf(stderr," %d", ahead->channels[i] );
            }
             fprintf(stderr,"\n");
            shmdt(ahead);
            exit(0);
          }

       }



       if (smid == -1 )
       {
        // process is not  running
          if ( option == STARTACQ )
          {
            fprintf(stderr,"ACQ: acq not-running. Creating  new..\n" );
          }

          if ( option == STATUSACQ )
          {
            fprintf(stderr,"ACQ: acq not-running.\n" );
            fprintf(stderr,"ACQ: Logfilename\n" );
            exit(0);
          }

          if ( option == STOPACQ )
          {
            fprintf(stderr,"ACQ: acq is not already running\n" ); 
            exit(0);
          }
       }
// check for running acq over




//no running acq found 
// proceed normal
       readsetupfile(acqinpfilename);
       initfiles();

        shmid2 = shmget(shmkey2, shmparsize, IPC_CREAT|IPC_EXCL|0666);
       if(shmid2 < 0) 
       {
         // If the memory exists already free it and allocated new //
         if((shmid2 = shmget(shmkey2, shmparsize, 0)) == -1)
         {
          fprintf(stderr,"ACQ: Could not get shared memory for par \n"); exit(0); 
         }
         else
         {
           fprintf(acqlogfile,"ACQ: Delete and redo allocation for shm par\n"); 
           shmctl(shmid2, IPC_RMID, &bufds);
           shmid2 = shmget(shmkey2, shmparsize, IPC_CREAT|IPC_EXCL|0666);
         }
       }


       shmid1 = shmget(shmkey1, totalshmsize, IPC_CREAT|IPC_EXCL|0666);
       if(shmid1 < 0) 
       {
         // If the memory exists already free it and allocated new //
         if((shmid1 = shmget(shmkey1, totalshmsize, 0)) == -1)
         {
          fprintf(stderr,"ACQ: Could not get shared memory for buffer\n"); exit(0); 
         }
         else
         {
           fprintf(acqlogfile,"ACQ: Delete and redo allocation for shm buffer\n"); 
           shmctl(shmid1, IPC_RMID, &bufds);
           shmid1 = shmget(shmkey1, totalshmsize, IPC_CREAT|IPC_EXCL|0666);
         }
       }

      

         if( (shmctl(shmid1, IPC_STAT, &bufshmds) == 0 ) )
         {
          fprintf(acqlogfile, "ACQ:  shared buffer memory: allocated : %d requested: %d\n", 
                               bufshmds.shm_segsz, totalshmsize );
         }
         else
         {
           fprintf(acqlogfile, "ACQ: Could not get info on buffer shared memory\n");
         }
         if( (shmctl(shmid2, IPC_STAT, &bufshmpards) == 0 ) )
         {
          fprintf(acqlogfile, "ACQ:  shared par memory: allocated : %d requested: %d\n", 
                               bufshmpards.shm_segsz, shmparsize );
         }
         else
         {
           fprintf(acqlogfile, "ACQ: Could not get info on par shared memory\n");
         }


        abuf = (acqbuf  *) shmat(shmid1, NULL, 0);

        ahead =  (acqhead *)shmat(shmid2, NULL, 0);


        if ( (abuf < 0 )  ) 
        {
          fprintf(stderr,"ACQ: buffer shared memory  attachement error %d %d\n", 
                             abuf, shmid1);
          exit(0);
        }

        if ( (ahead < 0 )  ) 
        {
          fprintf(stderr,"ACQ: par shared memory  attachement error %d %d\n", 
                             ahead, shmid2);
          exit(0);
        }
// clear all the memory 
         memset(abuf->buf,0,shmsize);
         memset(ahead,0,shmparsize);

         ahead->acqstatus = STARTACQ ;
         strcpy(ahead->acqlogfilename, acqlogfilename);
         fprintf(stderr,"ACQ: Logfilename %s\n", ahead->acqlogfilename );
}


int makedaemon1()
{
// make the process a daemon  part 1
// create a child and destroy the parent
pid_t childpid, childsid;


        childpid = fork();
        if (childpid < 0) {
              fprintf(stderr, "ACQ:  fork error\n");
                exit(EXIT_FAILURE);
        }
        if (childpid > 0) {
         // this is the parent & destroy the parent 
           exit(EXIT_SUCCESS);
        }
        else
        {
           childsid = setsid();
           if (childsid < 0)  
           {
            //session not possible exit 
              fprintf(stderr, "ACQ:  SID could not  be obtained\n");
              exit(EXIT_FAILURE);
           }
          // go ahead 
        }

return 0 ;
}

int makedaemon2()
{
// make the process a daemon  part 2
// handle all the file descriptors 
   fclose (stdin);
   fclose (stdout);
   fclose (stderr);

}

int parsearguments(int argc, char *argv[] )
{

    if ( argc  == 1 )
    {
    // assume start
     return STARTACQ ;
    }

    if ( argc  > 2 )
    {
      strncpy(acqinpfilename,argv[2],580);
    }

    if ( argc  > 1 )
    {
       if (strstr(argv[1], "stop" ) != NULL ) return STOPACQ ;
       if (strstr(argv[1], "start" ) != NULL ) return STARTACQ ;
       if (strstr(argv[1], "status" ) != NULL ) return STATUSACQ ;
    }
   
    // if this reached here invalid option and exit
  
    fprintf(stderr, "Usage:  acq start|stop|status    [input file name]\n");
    exit(0) ;

}

int main(int argc, char *argv[])
{
int    ret  ;
struct timeval start ;
int    optionstart ; 

  
      optionstart = parsearguments(argc, argv);


// intialise the files and shared memory buffers //
// check for runnig process                      //
// do  according to the optionstart              //
      initmemory(optionstart);
    
      makedaemon1();
      makedaemon2();


/*    Initialise the card    */
/*    subdevice, data range info */ 
      initcard();
/* set up  list of channels for daq */
      initchanlist();


/* for the moment start_acq does nothing */
/* apart from cheking the possibility of getting the time */
       start_acq();

/* set up  the command call   */
       initcommand();


/* write the intended command and that is possible by the device */
   if ( (ret = checkcommand()) != 0 ) 
   {
    fprintf(acqlogfile,"ACQ:  Error in daq command %d \n", ret);
    exit(0) ;
   }




/* run the actual command */
   fflush(acqlogfile);
   gettimeofday(&start, 0);
   ahead->starttime = start ;
   ret = comedi_command(dev, &fcmd);

  
/*  get the data by polling   */
   bufferdatanew();

   stop_acq(dev);

}
