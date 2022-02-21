/*  DATA HEADER
**  Written by S. Jeyakumar 28-10-2006

**  defines  mexart dataheader
**  keep track of the size of the "comments"
**  text stored in "other" 
*/

typedef struct  {
        char    magic[11]  ;
        char    obsdate[25] ;
        char    source[9] ;
        int     chan ;
        float     sampling ;
        char    observer[21] ;
        char    other[2048-28];
      } mexartheader ;

typedef struct  {
        int        scanno ;
   struct timeval  scantime ;
        float      data[1024] ;
      } mexartscan ;
