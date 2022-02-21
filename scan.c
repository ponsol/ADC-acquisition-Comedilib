/*  SCAN PACKGE
**  Written by S. Jeyakumar 28-10-2006

**  wrapper for scanmain routine 
*/

#include  "scanmain.c"



int usage()
{
   fprintf(stderr, "Usage: mxscan [begin end] channel [source]\n");
   fprintf(stderr, "       mxscan  scan channel endmin source\n");
   exit(-1);
}

int main(int argc, char *argv[])
{
    if ( scanmain(argc, argv) < 0 ) usage();
 return 0 ;
}
