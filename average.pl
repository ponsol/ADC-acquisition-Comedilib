#!/usr/bin/perl
#usage : average.pl asci_filename command opt tbegin tend
# commands:
#     rmean : take a running mean of ndat=opt points
#    l

use strict ;

use Astro::Time;
use Time::Local; 

sub fx {
  my ($x) = @_ ;
#crab values 
  my ($a, $m, $s, $b) ;
  $a=0.0444636; $m=10.6016; $s=0.0280375; $b=1.17489 ;

#virgo values
 $a= 0.0388304 ; $m= 17.5134   ; $s= 0.0276317 ; $b= 0.948727 ;
 my $f = $a*exp(-($x-$m)*($x-$m)/($s*$s))+$b;
  return $f ;
}

sub getmeanrms {

  my ($nmean, @all) = @_ ;

   my $time = $all[0] ;
   my $val = $all[1];

   my $ndat = scalar(@$time);
 
   my $mean ;
   my $rms ;
   my @ftime ;
   my @fval ;
 if ($nmean > $ndat )
 {
   return (0,0);
 }
    if  ( $nmean > 0 )
    {
          $mean = 0 ;
        for (my $m = 0 ; $m < $ndat; $m++ ) {
          $mean = $mean + $$val[$m]  ;
        }
           $mean = $mean/$ndat ;
          $rms = 0 ;
        for (my $m = 0 ; $m < $ndat; $m++ ) {
          $rms += ($mean - $$val[$m])*($mean - $$val[$m])  ;
        }
           $rms = sqrt($rms/($ndat-1)) ;
         my $t =  ($$time[$ndat-1] + $$time[0] )/2.0;
         push(@ftime, $t);
         push(@fval, $mean) ;
    }

   $ndat = scalar(@ftime);
   $ndat = scalar(@fval);

  return ($rms, \@ftime,\@fval);
}



sub getmean {

  my ($nmean, @all) = @_ ;

   my $time = $all[0] ;
   my $val = $all[1];

   my $ndat = scalar(@$time);
 
   my $mean ;
   my $rms ;
   my @ftime ;
   my @fval ;
   my @fvalr ;
 if ($nmean > $ndat )
 {
   return (0,0);
 }
    if  ( $nmean > 0 )
    {
          $mean = 0 ;
        for (my $m = 0 ; $m < $ndat; $m++ ) {
          $mean = $mean + $$val[$m]  ;
        }
           $mean = $mean/$ndat ;

          $rms = 0 ;
        for (my $m = 0 ; $m < $ndat; $m++ ) {
          $rms = $rms + ($$val[$m]-$mean)*($$val[$m]-$mean)  ;
        }
           $rms = sqrt($rms/($ndat-1)) ;

         my $t =  ($$time[$ndat-1] + $$time[0] )/2.0;
         push(@ftime, $t);
         push(@fval, $mean) ;
         push(@fvalr, $rms) ;
    }

   $ndat = scalar(@ftime);
   $ndat = scalar(@fval);

  return (\@fvalr, \@ftime,\@fval);
}

sub  totrms() {

 my ($fname, $tbegin, $tend) = @_;


 my @time ;
 my @val ;
 my $i= 0 ;
 my $btime ;
 my $etime ;


 my $mean=0;
 open fp, $fname or die "cannot open file" ;
 while (<fp>) { 
  
 if ( $_ !~ /#/  )
 {
   $btime = 0 ;
   $etime = 0 ;
  my @vals=split /\s+/, $_;
   if ( $tbegin != -1 )
   {
     if ( $vals[0] >= $tbegin )
      { 
         $btime = 1; 
      }
   }
   else
   {
     $btime = 1 ;
   }
    if ( $tend != -1 )
    {
      if ($vals[0] <= $tend ) 
      { 
         $etime = 1; 
      }
    }
    else
    {
      $etime = 1 ;
    }

     if( ($btime == 1 ) && ( $etime == 1 ) )
     {
       $mean += $vals[1] ;
       $i++ ;
     }
  }

 }
 close(fp);
   $mean = ($mean / ($i)) ;


 my $rms=0;
 open fp, $fname or die "cannot open file" ;
 while (<fp>) { 
  
 if ( $_ !~ /#/  )
 {
   $btime = 0 ;
   $etime = 0 ;
  my @vals=split /\s+/, $_;
   if ( $tbegin != -1 )
   {
     if ( $vals[0] >= $tbegin )
      { 
         $btime = 1; 
      }
   }
   else
   {
     $btime = 1 ;
   }
    if ( $tend != -1 )
    {
      if ($vals[0] <= $tend ) 
      { 
         $etime = 1; 
      }
    }
    else
    {
      $etime = 1 ;
    }

     if( ($btime == 1 ) && ( $etime == 1 ) )
     {
       my $tmean = &fx($val[0]);
       $rms += ($tmean - $vals[1] )*($tmean - $vals[1] ) ;
       $i++ ;
     }
  }

 }
 close(fp);
   $rms = sqrt($rms / ($i-1)) ;

 return $rms ;
}


sub clip_above_nsigma {

  my ($clip,@all) = @_ ;

  my $time = $all[0] ;
  my $val = $all[1] ;
  my $trms = ${$all[2]} ;
  my $n = scalar(@$time) ;

    my $i = 0;
    my $j = 0;
    my @fval ;
    my @ftime ;
    while($i < $n ) {
       my $f = &fx($$time[$i]);
       my $tt = abs($f-$$val[$i]) ;
      if ( $tt < $clip*$trms ) {
        push(@ftime, $$time[$i] );
        push(@fval,  $$val[$i] );
        #push(@fval,  $f );
        $j++ ;
      }
      $i++ ;
    }

  my $nc =  $n+1-$j ;
 return ($nc, \@ftime, \@fval);
}

sub rav {

  my  ($nav,$clip,@all) = @_ ;

  my $time = $all[0];
  my $val = $all[1];
  my $n = scalar(@$time) ;

  #print "$n\n" ;
  if ($nav == 0 )   { return (0,0); }

  my $i =  $nav/2;
  my $j = 0 ;

  my @tt = &getmeanrms($nav, @all);
   my $mean = ${$tt[2]};
   my $rms, $tt[0];

  my @ftime ;
  my @fval ;
  my @frmean ;
  my @frrms ;

         my $i=0 ;
       foreach my $tval ( @$val )
       {
         my $st = abs($mean-$$val[$i]) ;
         if ( $st < $clip*$rms ) {
           push(@ftime, $$time[$i] );
           push(@fval,  $$val[$i] );
         }
        $i++ ;
       } 
      my $nrem =  $nav - $i ;

 return ($nrem , \@ftime, \@fval);
}

sub  getlst () {
my ($hour) = @_;

  my $sec ;
  my $min ;
  my $usec ;

        my  $tday = $main::day ;
        if ( $hour > 24 )
        {
          $tday += 1;
        }

         $min = $hour - $hour%24 ;
         $min = $min*60 ;
         $hour = $hour%24 ;
         $sec = $min - $min%60 ;
         $min = $min%60 ;
         $sec = $sec*60 ;
         $usec = $sec - $sec%60 ;
         $usec = $usec*1000*1000 ;
         $sec = $sec%60 ;

         my $wday ;
         my $yday ;
         my $isdst=1;
     my $localsec =  timelocal($sec,$min,$hour,$tday-1,$main::month-1,$main::year-1900,undef,undef,$isdst);
     my ($utsec,$utmin,$uthour,$utday,$utmonth,$utyear,$utwday,$utyday,$utisdst)     = gmtime($localsec);

     $utday   = $utday+1 ;
     $utmonth = $utmonth+1 ;
     $utyear  = $utyear+1900 ;

     
     my $longitude  = -(101.0+41.0/60.0) ;
     my $lturn = deg2turn($longitude);
     my $uth ;
     $uth = $uthour + $utmin/60.0 + $utsec/60.0/60.0 ;
     $uth += $usec/1000/1000/60.0/60.0 ;
     $uth /= 24.0 ;
     my $lst = cal2lst($utday, $utmonth, $utyear, $uth, $lturn);

     

     my $str = turn2str($lst, 'H', 2 , ' ');
     
     $lst *= 24.0 ;
     my $lsth = $lst%24 ;
     my $lstm = ($lst - $lst%24)*60 ;
     my $lsts = ($lstm - $lstm%60)*60 ;
        $lstm = $lstm%60 ;

    #print "UT LST: $uthour $utmin $utsec $utyear $utmonth $utday\n";
    #print "LT LST: $hour $min $sec $main::year\n";
    #print "HH : $str :: $lsth $lstm $lsts\n";

 return $lst ;
}


sub main {
 my ($fname, $command, $opt, $opt1, $tbegin, $tend) = @_;

open fp, $fname or die "cannot open file" ;

  my @time ;
  my @val ;
  my $i= 0 ;
  my $btime ;
  my $etime ;

  my $totrms ;


         if ( "$command" eq "rmean" )
         {
           if ($opt == -1) { $opt = 1000 ; }
           printf "#Running mean of %d points\n", $opt;
         }

         if ( "$command" eq "clip" )
         {
           if ($opt == -1) { $opt = 4 ; }
           printf "#Clip above %d sigma\n", $opt;
           $totrms = &totrms($fname, $tbegin, $tend) ;
         }

         if ( "$command" eq "rclip" )
         {
           if ($opt == -1) { $opt = 100 ; }
           if ($opt1 == -1) { $opt1 = 4 ; }
           printf "#Running clip above %d sigma\n", $opt;
         }
   

         if ( "$command" eq "mean" )
         {
           if ($opt == -1 ) { $opt = 100 ; }
           printf "#Mean of %d points\n", $opt;
         }


      my @finalt ;
      my @finalv ;
      my @finalr ;

 my $nclipped=0;
 my $i=0 ;
 my $j=0 ;
 my $nr=0 ;
 while ( <fp> ) { 
  
 if ( $_ !~ /#/  )
 {
   $btime = 0 ;
   $etime = 0 ;
  my @vals=split /\s+/, $_;
   if ( $tbegin != -1 )
   {
     if ( $vals[0] >= $tbegin )
      { 
         $btime = 1; 
      }
   }
   else
   {
     $btime = 1 ;
   }
    if ( $tend != -1 )
    {
      if ($vals[0] <= $tend ) 
      { 
         $etime = 1; 
      }
      else
      {
         close(fp);
      }
    }
    else
    {
      $etime = 1 ;
    }

     if( ($btime == 1 ) && ( $etime == 1 ) )
     {
       push (@time,$vals[0]) ;
       push (@val,$vals[1] );

       $i++ ;
       $nr++ ;

        if ($nr >= $opt )
        {

          if ( "$command" eq "rmean" )
         {
           my @all ;
           push(@all, \@time, \@val);
           my $nmean = $opt ;
           my @tt = &getmeanrms($nmean, @all);
           push(@finalt, @{$tt[1]});
           push(@finalv, @{$tt[2]});
           push(@finalr, $tt[0]);
           splice (@time, 0, 1);
           splice (@val, 0, 1);
         }


         if ( "$command" eq "rclip" )
         {
           my @all ;
           push(@all, \@time, \@val);
           my $nmean = $opt ;
           my @tt = &rclip($nmean, @all);
           push(@finalt, @{$tt[1]});
           push(@finalv, @{$tt[2]});
           $nclipped += $tt[0] ;
           splice (@time, 0, 1);
           splice (@val, 0, 1);
         }


       
        }

        if ($i == $opt )
        {
          my @all ;
          push(@all, \@time, \@val);

         if ( "$command" eq "clip" )
         {
           my @tt =  &clip_above_nsigma($opt,@all);
           my $nt  = $tt[0] ;
           $main::nclipped += $nt ;
 
           push(@finalt, @{$tt[1]});
           push(@finalv, @{$tt[2]});
           push(@finalr, @{$tt[3]});
           @time = ();
           @val = ();
         }
   

         if ( "$command" eq "mean" )
         {
           my $nmean = $opt ;
           my @tt = &getmean($nmean, @all);
           push(@finalr, @{$tt[0]});
           push(@finalt, @{$tt[1]});
           push(@finalv, @{$tt[2]});
           @time = ();
           @val = ();
         }


         my $nt = scalar(@time);
         #print "Block read $j $hour $lst $vals[1] $nt \n";
         $j++;
         $i = 0; 
        }

     }
  }
  else
  {
   if ( $_ !~ /#Scan/  )
   {
        print $_ ;
   }
   if ( $_ =~ /#Filename/  )
   {
       if ( $_ =~ /\#Filename\s+\:\s+(\d+?)\-(\d+?)\-(\d+?)\-(\d\d)(\d\d)(\d\d).*$/ )
       {
        $main::year  =  $1;
        $main::month =  $2;
        $main::day   =  $3;
        $main::hour  =  $4;
        $main::min  =  $5;
        $main::sec  =  $6;

        my $hh ;
        $hh = $main::hour+$main::min/60.0 +$main::sec/60.0/60.0 ;
        my  $lst = &getlst($hh);
        my $lsth;
        my $lstm;
        my $lsts;
        $lstm = $lst - $lst%24 ;
        $lsth *= $lst%24;
        $lstm *= 60;
        $lsts = $lstm - $lstm%60 ;
        $lstm = $lstm%60;
        $lsts *= 60;

     print "#Date: $main::day $main::month $main::year ";
     print " Time: $main::hour $main::min $main::sec" ;
     print " LST: $lsth $lstm $lsts\n";
       }
   } #filename end
  } #coment lines else end
 } #while end

 return ($nclipped, \@finalt, \@finalv, \@finalr);
}


#main here

   my $filename ;
   my $tbegin = -1 ;
   my $tend = -1 ;
   my $command = "mean" ;
   my $opt = -1 ;
   my $opt1 = -1 ;
   my $i = 0 ;
   my $addarg= 0;
 foreach (@ARGV ) {
  if ($i == 0 ) { $filename = $_ ; }
  if ($i == 1 ) { $command = $_ ; }
  if ($i == 2 ) { $opt = $_ ; }
  if ( $command == "rclip" ) 
  { 
     $addarg=1;  
    if ($i == 3 ) { $opt1 = $_ ; }
  } 
  if ($i == 3+$addarg ) { $tbegin = $_ ; }
  if ($i == 4+$addarg ) { $tend = $_ ; }
  $i++ ;
 }

   my $nclipped = 0 ;
   my $year ;

   my $month ;
   my $day ;
   my $hour ;
  my @all = &main($filename,$command, $opt, $opt1, $tbegin,$tend);

  my $nc = $all[0] ;
  my $finalt = $all[1] ;
  my $finalv = $all[2] ;
  my $finalr = $all[3] ;

  my $mean ;
  my $rrms ;
   my $ndat = scalar(@$finalt);

  printf "#No. of data points %d\n", $ndat ;
     if ( "$command" eq "clip" )
     {
      printf "#No. of data points removed %d\n", $nc ;
     }
  for ($i=0; $i<$ndat; $i++ )
  {
   #print "$ftime[$i] $fval[$i]\n" ;
    my $hh = $$finalt[$i] ;
    my  $lst = &getlst($hh);

     if ( "$command" eq "rmean" )
     {
       if ($i==0 )
       {
        print "#Time Volt  Rmean Rrms \n" ;
       }
       print "$$finalt[$i] $$finalv[$i] $$finalr[$i] $lst\n" ;
     }
     else
     {
       if ($i==0 )
       {
        print "#Time Volt  Mean Rms  LST\n" ;
       }
       print "$$finalt[$i] $$finalv[$i] $$finalr[$i] $lst\n" ;
     }
  }
 
