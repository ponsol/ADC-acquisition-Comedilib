/*  SERVER HEADER
**  Written by S. Jeyakumar 28-10-2006

**  defines server and port numbers of server 
**  running the record
**  Note that portnumber to portnumber+64 ports can be opened 
*/

#ifndef __SERVERH_
#define __SERVERH_
// The ip number of the machine running recrod//
//char  recordserverip[] = "127.0.0.1" ;
static char  recordserverip[] = "132.248.208.37" ;
static int   recordcmdport = 60000 ;


/* the following is for getonline and record */
#define  PLOTTIMESPAN   20  /*time in minutes */
#endif
