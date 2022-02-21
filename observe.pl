#!/usr/bin/perl -w

#use strict ;

   ($infile, $simulate) = @ARGV;
  if (scalar(@ARGV) < 1 ) {
   print "Usage: observe.pl inputfile [simulate]\n";
   exit(0);
  }
  open INFILE, "<$infile" or die "Cannot open input file: $infile\n" ;

   $sim = 0 ;
   if (scalar(@ARGV) == 2 ) {
    if ($simulate =~ /simulate/ ) { $sim=1 ; }
   }

       $chan = 0 ;

     print "\n";
     $i  = 0 ;
   while ( <INFILE> ) {
      $line = $_ ;
     if ($line !~ /#/) {
      @dummy=split /#/,$line;

      if ( !defined $dummy[1] ) {
         $dummy[1] = "none" ;
      }
      push (@comment, $dummy[1]);

      @array=split /\s+/,$dummy[0];
      if ( scalar(@array) != 13 )
      {
        if ($sim == 1 )
        {
          printf "Line %d is  missing some values", $i  ;
        }
        else
        {  
          exit(0);
        }
      }
      else
      {
       push (@source,  $array[0]) ;
       push (@rahr,    $array[1]);
       push (@ramin,   $array[2]);
       push (@rasec,   $array[3]);
       push (@decdeg,  $array[4]);
       push (@decmin,  $array[5]);
       push (@decsec,  $array[6]);
       push (@btimehr, $array[7]);
       push (@btimemin, $array[8]);
       push (@btimesec, $array[9]);
       push (@etimehr,  $array[10]);
       push (@etimemin, $array[11]);
       push (@etimesec, $array[12]);
          $ttime = $btimehr[$i]+$btimemin[$i]/60.0+$btimesec[$i]/60.0/60.0 ;
       push (@sbtime, $ttime);
          $ttime = $etimehr[$i] + $etimemin[$i]/60.0 + $etimesec[$i]/60.0/60.0 ;
       push (@setime, $ttime);

          $tt = ($setime[$i] - $sbtime[$i] )*60 ;
          $tt = ($tt+0.5  - $tt%1) ;
       push(@scanmin, $tt);
             $setime[$i] =  $sbtime[$i] + $scanmin[$i]/60.0 ;
          if ($scanmin[$i] <= 0 )
          {
             printf "Scan minutes of the source %s is negative\n", 
                                   $source[$i] ;
             exit(-1);
          }
          #print $tt, $scanmin[$i] ;

             $ds = "+" ;
           if ( $decdeg[$i] =~ /-/ ) {
             @tt = split /-/, $decdeg[$i]  ;
             $decdeg[$i] = $tt[1] ;
             $ds = "-" ;
           }
           push(@decsign, $ds);

          #below is dummy  to avoid warning
            if ( $comment[$i] =~ /row/ ){ ;}

            $tt = $rahr[$i] + $ramin[$i]/60.0 + $rasec[$i]/60.0/60.0 ;
            if ($decsign[$i] =~ /-/ )
            {
            $tt = $decdeg[$i] + $decmin[$i]/60.0 + $decsec[$i]/60.0/60.0 ;
            }
          #above is dummy dummy to avoid warning
            
       $i++ ;
      }

     } # if ~/#/ loops ends here
   } #while loop ends here

     $nsource =  scalar(@source) ;
     printf "\n";
     printf "Total number of sources: %d \n", $nsource ;

     for ($i = 1 ; $i < $nsource ; $i++ )
     {
         if ( $sbtime[$i] < $sbtime[$i-1] ) 
         {
          printf "the source %s transits later than %s\n",
                             $source[$i], $source[$i-1] ; 
          exit(0);
         }
     }

     if ( $sim == 1 ) { 
       for ($i = 1 ; $i < $nsource ; $i++ )
       {
         if ( $sbtime[$i] < $setime[$i-1] ) 
         {
          printf "begin time of %s conflicts with end time of %s\n",
                              $source[$i], $source[$i-1] ; 
         }
       }
     }
    

# if this is only a simulation exit here 
     if ( $sim == 1 ) { 
     print "simulation success...\n\n" ;
     exit(0);
     }


        $ncur = 0 ;
        $prestopped =  0 ;
     while ( $ncur < $nsource) {
       $date = `date`;
       @darr = split /\s+/, $date ;
       @tarr = split /:/, $darr[3] ;
       $time =  $tarr[0]+$tarr[1]/60.0+$tarr[2]/60.0/60.0 ;
       #print "$time \n";

        if ( ($time <= $sbtime[$ncur]-6/60.0/60.0)  &&
             ($time >= $sbtime[$ncur]-7/60.0/60.0) 
           ) {
           #give and end cmd 2 secs before the next soruce
           if ( $prestopped ==  0) 
           {
             $cmd = sprintf "mxscan end $chan %d", 
                           $scanmin[$ncur] ;
             $scanout = `$cmd`;
               $prestopped = 1 ;
           }
        }
        if ( ($time <= $sbtime[$ncur]            )  &&
             ($time > $sbtime[$ncur]-5/60.0/60.0) 
           ) {
          #schedule this source
           $cmd = sprintf "mxscan scan $chan %d %s", 
                           $scanmin[$ncur], $source[$ncur] ; 
           $scanout = `$cmd`;
           $prestopped = 0 ;
              $err =  $?;
              if (! defined $scanout ) { $scanout = ""};
           if ( $err == 0 ) {
            printf "%s at %d %d %d scheduled successfully cmd: %s\n", 
                             $source[$ncur],
                             $btimehr[$ncur],
                             $btimemin[$ncur],
                             $btimesec[$ncur],
                             $cmd ;
           }
           else {
            printf "source %s at %d %d %d schedule error\n",  
                             $source[$ncur],
                             $btimehr[$ncur],
                             $btimemin[$ncur],
                             $btimesec[$ncur] ;
           }
          $ncur++ ;
        }
        else {
           if ( ($time >  $sbtime[$ncur] ) ) {
            printf "source %s at %d %d %d already pssed zenith\n",
                             $source[$ncur] ,
                             $btimehr[$ncur],
                             $btimemin[$ncur],
                             $btimesec[$ncur];
            $ncur++ ;
           }
        }
   
     }   

