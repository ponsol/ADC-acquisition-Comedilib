#! /usr/bin/perl -w
   my $tfh ;
   my $pid = $$ ;
   print " $pid \n";
   open $tfh, "+< /data/record.log.07Feb2006 " or die "can't start mxrecord";
   seek($tfh, 0,2);
   sleep(10);
   my @output = <$tfh> ;
   my $filename ;
   print "::\n @output :: \n";
   my $tag ;
   

