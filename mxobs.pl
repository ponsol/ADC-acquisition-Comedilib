#!/usr/bin/perl -w
#
#
# Written by S. Jeyakumar 28-10-2006
#
# Master gui routine for all other observation routines 
#
#
#
#
#
use warnings;
use strict;


use FileHandle;
use Astro::Time;
use Glib qw/TRUE FALSE/;
use Gtk2;
use Gtk2::Helper;


use constant COL_LABEL   => 0;
use constant COL_VALUE   => 1;
use constant COL_EDITL   => 2;
use constant COL_EDITV   => 3;

      Gtk2->init ;


       &acqdefault;

       $main::plotpid = -1 ;
       $main::acqfh = -1;
       $main::recordfh =  -1 ;
       $main::recordrunning = 0 ;
       $main::recordloaded = 0 ;
       $main::acqloaded = 0 ;
       $main::acqrunning = 0 ;
       $main::recordlogfile = "" ;
       $main::acqlogfile = "" ;
       $main::acqinpfile = "/tmp/acq.inp" ;
       @main::recordingchan = ();

      @main::acqchan = ();
      @main::rbut = ();

      my $window = Gtk2::Window->new();
      $window->signal_connect (destroy => sub { Gtk2->main_quit; });
      $window->set_position('mouse');
      #$window->set_default_size(8,4);
      $window->set_size_request(900,450); 
   #   $window->set_size_request(7,7); 
      

      my $hbox = Gtk2::HBox->new(TRUE,0);
      my $vbox = Gtk2::VBox->new(FALSE,0);
      my $hboxright = Gtk2::HBox->new(TRUE,0);
      $hbox->pack_start ($vbox, TRUE, TRUE, 2);
      $hbox->pack_start ($hboxright, TRUE, TRUE, 2);
      $window->add($hbox);

    
      my $hbox1 = Gtk2::HBox->new(FALSE,0);
      my $hbox2 = Gtk2::HBox->new(FALSE,0);
      my $hboxe = Gtk2::HBox->new(FALSE,0);
      my $hboxradio = Gtk2::HBox->new(FALSE,0);
      my $hbox3 = Gtk2::HBox->new(FALSE,0);
      my $hbox11 = Gtk2::HBox->new(FALSE,0);
      $hbox1->pack_start ($hbox11, FALSE, TRUE, 2);
      my $hbox12 = Gtk2::HBox->new(TRUE,0);
      $hbox1->pack_start ($hbox12, TRUE, TRUE, 2);
       my $hbox31 = Gtk2::HBox->new(FALSE,0);
      $hbox3->pack_start ($hbox31, FALSE, TRUE, 2);
       my $hbox32 = Gtk2::HBox->new(FALSE,0);
      $hbox3->pack_start ($hbox32, TRUE, TRUE, 2);

       my $hbox21 = Gtk2::HBox->new(FALSE,0);
      $hbox2->pack_start ($hbox21, FALSE, TRUE, 2);
       my $hbox22 = Gtk2::HBox->new(FALSE,0);
      $hbox2->pack_start ($hbox22, TRUE, TRUE, 2);

       my $hboxradiol = Gtk2::HBox->new(FALSE,0);
       my $label = Gtk2::Label->new("Recording      ");
       $hboxradiol->pack_start ($label, FALSE, TRUE, 2);
      $hboxradio->pack_start ($hboxradiol, FALSE, TRUE, 2);
       my $hboxradior = Gtk2::HBox->new(FALSE,0);
      $hboxradio->pack_start ($hboxradior, TRUE, TRUE, 2);

      $vbox->pack_start ($hbox1, TRUE, TRUE, 2);
      $vbox->pack_start ($hbox2, FALSE, FALSE, 2);
      $vbox->pack_start ($hboxradio, FALSE, TRUE, 2);
      $vbox->pack_start ($hbox3, FALSE, TRUE, 2);
      $vbox->pack_start ($hboxe, TRUE, TRUE, 2);

#error mesg window.
      my $sw  ;
      my $ferror = Gtk2::Frame->new("Message");
       $sw = Gtk2::ScrolledWindow->new;
       $sw->set_policy ('automatic', 'automatic');
      $ferror->add ($sw);
      my $textmesg = Gtk2::TextView->new();
      $main::textmesg = $textmesg ;
      $textmesg->set_editable(FALSE);
      $sw->add ($textmesg);
      $hboxe->pack_start ($ferror, TRUE, TRUE, 2);


#command window
      my $entry = Gtk2::Entry->new_with_max_length(100 );
#       $entry->set_size_request(300,30); 
      my $elabel = Gtk2::Label->new("  Command:->");
      my $blue = Gtk2::Gdk::Color->new (0,0,0xFFFF);
      $elabel->modify_fg( 'normal',$blue);
      $hbox31->pack_start ($elabel, FALSE, FALSE, 4);
      $hbox32->pack_start ($entry, FALSE, FALSE, 4);
      $entry->signal_connect( activate => \&set_command, 10 );

#comments window setup
       $vbox = Gtk2::VBox->new(FALSE,0);
      $hbox21->pack_start ($vbox, FALSE, FALSE, 4);
      my $clabel = Gtk2::Label->new("Comments    ");
      $vbox->pack_start ($clabel, FALSE, FALSE, 4);
      my $fcomment = Gtk2::Frame->new("");
      $hbox22->pack_start ($fcomment, TRUE, TRUE, 4);
      my $textcomment = Gtk2::TextView->new();
      $textcomment->set_editable(TRUE);
       $sw = Gtk2::ScrolledWindow->new;
       $sw->set_policy ('automatic', 'automatic');
      $fcomment->add ($sw);
      $sw->add ($textcomment);


#buttons  setup
     my $vboxbuttons = Gtk2::VBox->new(FALSE,0);
     my $fpars = Gtk2::Frame->new("ACQ Input Parameters");
     my $facqlog = Gtk2::Frame->new("Acq Log");
     my $frecordlog = Gtk2::Frame->new("Record Log");
      $hbox11->pack_start ($vboxbuttons, FALSE, FALSE, 2);
      $hbox12->pack_start ($fpars, TRUE, TRUE, 4);
      #$hbox12->set_size_request(100,40);
      $hboxright->pack_start ($frecordlog, TRUE, TRUE, 4);
      $hboxright->pack_start ($facqlog, TRUE, TRUE, 4);

     my $bquit = Gtk2::Button->new_from_stock ('Quit');
     my $bacqstart = Gtk2::Button->new_from_stock ('Start ACQ');
     my $brecordstart = Gtk2::Button->new_from_stock ('Start Record');
     my $bshowlog = Gtk2::Button->new_from_stock ('Hide Log');
     my $bplot = Gtk2::Button->new_from_stock ('Start Plot');

     $bquit->signal_connect (clicked => \&quit_program, 10);
     $bacqstart->signal_connect (clicked => \&call_acq, 10);
     $brecordstart->signal_connect (clicked => \&call_record, 10);
     $bshowlog->signal_connect (clicked => \&call_showlog, 10);
     $bplot->signal_connect (clicked => \&call_plot, 10);

     $vboxbuttons->pack_start ($bquit, FALSE, TRUE, 4);
     $vboxbuttons->pack_start ($bacqstart, FALSE, TRUE, 4);
     $vboxbuttons->pack_start ($brecordstart, FALSE, TRUE, 4);
     $vboxbuttons->pack_start ($bshowlog, FALSE, TRUE, 4);
     $vboxbuttons->pack_start ($bplot, FALSE, TRUE, 8);

     my $textacqlog = Gtk2::TextView->new();
     $textacqlog->set_editable(FALSE);
     $sw = Gtk2::ScrolledWindow->new;
     $sw->set_policy ('automatic', 'automatic');
     $facqlog->add ($sw);
     $sw->add ($textacqlog);

     my $textrecordlog = Gtk2::TextView->new();
     $textrecordlog->set_editable(FALSE);
     $sw = Gtk2::ScrolledWindow->new;
     $sw->set_policy ('automatic', 'automatic');
     $sw->add ($textrecordlog);
     $frecordlog->add ($sw);

      $main::acqbuffer  = $textacqlog->get_buffer();
      $main::recordbuffer = $textrecordlog->get_buffer();
      $main::mesgbuffer = $textmesg->get_buffer();
      $main::commentbuffer = $textcomment->get_buffer();

      &prostatus();
     my $parmodel = create_parmodel();
     my $textpars = Gtk2::TreeView->new ($parmodel);
      $textpars->set_rules_hint (TRUE);
      $textpars->get_selection->set_mode ('single');
     $sw = Gtk2::ScrolledWindow->new;
     $sw->set_policy ('automatic', 'automatic');
     $sw->add ($textpars);
     $fpars->add ($sw);
    
     &render_textpars ($textpars);

      $window->show_all();
      &update_pars();
      &setstatus_acq($bacqstart);
      &setstatus_record($brecordstart);
      &loadlogs();
      $main::tviewacq = $textacqlog;
      $main::tviewrecord = $textrecordlog;
      &scroll_toend();

     #start with hide
     #$hboxright->hide ;
     Glib::Timeout->add (5000, \&update_logs, $bshowlog );
     Gtk2->main;



sub set_acqchannels
{
      @main::acqchan = @_;
      foreach my $c (@main::acqchan)
      {
       my $label = "Chan$c" ;
       my $rbut = Gtk2::CheckButton->new($label);
       $hboxradior->pack_start ($rbut, TRUE, TRUE, 2);
       $rbut->show();
       push(@main::rbut, $rbut);
      } 
   $hboxradior->show();
}


sub set_recordchannels
{
      my ($w) = @_;

       my $i=0 ; 
     foreach my $b (@main::rbut)
     {
       if ($b == $w )
       {
          my $st =  $w->get_active();
         if ( $st == TRUE )
         {
           &run_command("mxscan begin $i 2>&1 |");
         } 
         if ( $st == FALSE )
         {
           &run_command("mxscan end $i 2>&1 |");
         } 
       } 
      $i++ ;
     } 

}

sub update_pars
{
   my %m ;
  foreach my $a (@main::parlist)
  {
    $m{$a->{label}} = $a->{value} ;
  }

  $main::commentbuffer->set_text($m{comment});

}


sub get_comment
{
  my $tt = $main::commentbuffer->get_text( $main::commentbuffer->get_start_iter,
                                           $main::commentbuffer->get_end_iter, TRUE);
  foreach my $a (@main::parlist)
  {
    if ( $a->{label} =~ /comment/ )
    {
     my @all = split( '\n', $tt) ;
     $tt = join ' ', @all ;
     
      $a->{value} = $tt ;
    }
  }
}


sub quit_program 
{
my ($button, $data) = @_ ;

    if ($main::plotpid > 1 )
    {
     my $fh ;
     open ($fh, "kill -9 $main::plotpid 2>&1 | ");
     my @cont = <$fh> ;
     &printmesg(@cont);
    }
 
        Gtk2->main_quit;
        1;
}

sub create_parmodel
{
my $parmodel = Gtk2::ListStore->new (qw/Glib::String Glib::String Glib::Boolean Glib::Boolean/);

   foreach my $a (@main::parlist) 
   {

      my $t = $a->{label} ;
     if ( $t !~ /comment/ )
     {
      my $iter = $parmodel->append;
      $parmodel->set ($iter,
                   COL_LABEL,  $a->{label},
                   COL_VALUE, $a->{value},
                   COL_EDITL, $a->{editl}, COL_EDITV, $a->{editv} );
     }
  }


 return $parmodel ;
}

sub editpar {
 my ($cell, $path_string, $new_text, $model) = @_;


 my $path = Gtk2::TreePath->new_from_string ($path_string);
 my $column = $cell->get_data ("column");
 my $iter = $model->get_iter ($path);

  if ($column == COL_LABEL) 
  {
        my $i = ($path->get_indices)[0];
        $main::parlist[$i]{label} = $new_text;

        $model->set ($iter, $column, $main::parlist[$i]{label});

  } 
  elsif ($column == COL_VALUE) 
  {
        my $i = ($path->get_indices)[0];
        $main::parlist[$i]{value} = $new_text;

        $model->set ($iter, $column, $main::parlist[$i]{value});
  }

}

sub render_textpars {
  my ($w) = @_;
  my $model = $w->get_model;

  # number column
  my $render = Gtk2::CellRendererText->new;
  $render->signal_connect (edited => \&editpar, $model);
  $render->set_data (column => COL_LABEL);

  $w->insert_column_with_attributes (-1, "Parameter", $render,
                                            text => COL_LABEL,
                                            editable => COL_EDITL);

  # product column
  $render = Gtk2::CellRendererText->new;
  $render->signal_connect (edited => \&editpar, $model);
  $render->set_data (column => COL_VALUE);

  $w->insert_column_with_attributes (-1, "Value", $render,
                                            text => COL_VALUE,
                                            editable => COL_EDITV);
}



sub getstatus 
{
  my ($command) = @_ ;
   my $filename = "" ;
   my $tfh ;
   my $ret = open ($tfh, "$command");
   if ( not $ret )
   {  &printmesg ( "cannot execute $command" ); return 0; }

   my  @output = <$tfh> ;
   &printmesg(@output);
   my $flag = 0 ;
   foreach my $t (@output )
   {
     if ( $t  =~ /Logfilename\s+([\/\w\W].*?)\s+.*?/  )
     {
        $filename = $1 ;
         if ( $filename !~ /^$/ )
         {
          $flag = 1 ;
         }
     }

     if ( $t  =~ /Exit/  )
     {
       &printmesg("Got exit");
        $filename = "" ;
        $flag = 0 ;
     }

     if ( $t  =~ /Comment:\s+([\/\w\W].*?)$/  )
     {
         my $tt = $1 ;

         foreach my $ap (@main::parlist)
         {
           if ( $ap->{label}  =~ /comment/  )
           {
               $ap->{value} = $tt;
           }
         }

     }

     if ( $t  =~ /Observer\s+([\/\w\W].*?)$/  )
     {
         my $tt = $1 ;

         foreach my $ap (@main::parlist)
         {
           if ( $ap->{label}  =~ /observer/  )
           {
               $ap->{value} = $tt;
           }
         }

     }

     if ( $t  =~ /Sampling\s+([\d].*?)$/  )
     {
         my $tt = $1 ;

         foreach my $ap (@main::parlist)
         {
           if ( $ap->{label}  =~ /samplingrate/  )
           {
               $ap->{value} = $tt;
           }
         }

     }

     if ( $t  =~ /Recording\s+([\/\w\W].*?)$/  )
     {
         my $tt = $1 ;

         my @alc = split ' ', $tt ;
           my $i=0 ;

         foreach my $c (@alc)
         {
           if  ($main::acqchan[$i] == $c )
           { 
             $main::rbut[$i]->set_active (TRUE);
           } 
           $i++;
         }

         foreach my $b (@main::rbut)
         {
          $b->signal_connect (toggled => \&set_recordchannels, undef);
         }
     }

     if ( $t  =~ /Acqchannels\s+([\/\w\W].*?)$/  )
     {
         my $tt = $1 ;

         my @alc = split ' ', $tt ;
         
          $tt = join ',', @alc ;
         foreach my $ap (@main::parlist)
         {
           if ( $ap->{label}  =~ /channels/  )
           {
               $ap->{value} = $tt;
           }
         }

         &set_acqchannels(@alc);
     }



   }


  close($tfh);

return ($flag, $filename) ;
}

sub loadlogfile 
{
     my ($pfname, $pbuffer)  = @_ ;

   my $tag ;
   my $fh ;
   my @content ;
   ($tag, $fh, @content) = &getfilecontent($$pfname);

    my $cc = join ' ', @content ;
    ${$pbuffer}->set_text($cc);
   my $iter = ${$pbuffer}->get_end_iter();
   my $mark = ${$pbuffer}->create_mark (undef, $iter, TRUE); 
      ${$pbuffer}->delete ($iter, $iter);

 return ($fh);
}

sub clearlogfile 
{
     my ($pfh, $pfname, $pbuffer)  = @_ ;

   my $fh = $$pfh ;
   close($fh);
   my $cc = "";
   ${$pbuffer}->set_text($cc);
   ${$pbuffer}->set_modified(TRUE);
}



sub acqstatus 
{

   ($main::acqrunning, $main::acqlogfile) = 
                            &getstatus("mxacq status  2>&1 |");
}
sub recordstatus 
{

   ($main::recordrunning, $main::recordlogfile, @main::recordingchan) = 
                            &getstatus("mxrecord status 2>&1 |");
}
sub prostatus 
{
    &acqstatus();
    &recordstatus();
}




sub setstatus_acq
{
   my ($b) = @_ ;
      my $red = Gtk2::Gdk::Color->new (0xFFFF,0xAAAA,0xAAAA);
      my $green = Gtk2::Gdk::Color->new (0xAAAA,0xFFFF,0xAAAA);

   my $la = $b->get_label ;
   if ($main::acqrunning == 1 )
   {
       $b->modify_bg( 'normal',$green);
       $b->set_label("Stop ACQ") ;
   }
   else
   {
       $b->modify_bg( 'normal',$red);
       $b->set_label("Start ACQ") ;
   }
}


sub setstatus_record
{
   my ($b) = @_ ;
      my $red = Gtk2::Gdk::Color->new (0xFFFF,0xAAAA,0xAAAA);
      my $green = Gtk2::Gdk::Color->new (0xAAAA,0xFFFF,0xAAAA);
    my  $la = $b->get_label ;
   if ($main::recordrunning == 1 )
   {
       $b->modify_bg( 'normal',$green);
       $b->set_label("Stop Record") ;
   }
   else
   {
       $b->modify_bg( 'normal',$red);
       $b->set_label("Start Record") ;
   }

}


sub call_acq
{
   my ($b) = @_ ;
   my $la = $b->get_label ;
  
  my $stat = $main::acqrunning ;
  
   if ($stat == 1 )
   {
      if ( $main::recordrunning == 1 )
      {
        &call_recordstop ;
        &unset_recordchannels ;
      
        $main::recordrunning = 0 ;
        &setstatus_record($brecordstart) ;
      }
     &call_acqstop;
     &unset_acqchannels();
     $main::acqrunning = 0 ;
   }
   else
   {
     $main::acqrunning =  &call_acqstart($b);
   } 

     &setstatus_acq($b) ;

}


sub call_record
{
   my ($b) = @_ ;
   my $la = $b->get_label ;
  
  my $stat = $main::recordrunning ;
  
   if ($stat == 0 )
   {
     &get_comment();
     my $ret = &acqinpfile();
     if ( $ret == 1 ) { return 1; }
     $main::recordrunning  = &call_recordstart($b);
   }

   if ($stat == 1 )
   {
      &call_recordstop($b);
      &unset_recordchannels ;
      $main::recordrunning  = 0;
   }
     &setstatus_record($b) ;

}


sub printmesg 
{
   my (@mesg) = @_ ;
  foreach my $a (@mesg )
  {
   $main::textmesg->scroll_to_iter( $main::mesgbuffer->get_end_iter, 0, FALSE, 0, 0 );
   $main::mesgbuffer->insert ($main::mesgbuffer->get_end_iter, $a);
  }
   $main::textmesg->scroll_to_iter( $main::mesgbuffer->get_end_iter, 0, FALSE, 0, 0 );
}


sub call_acqstart
{

   my ($window) = @_ ;
 
   my $ret = &acqinpfile();

   if ($ret == 1 ) { return 1; }

   ($main::acqrunning, $main::acqlogfile) = 
                            &getstatus("mxacq start  $main::acqinpfile  2>&1 |");

   if ($main::acqloaded == 0)
   {
     if ($main::acqrunning == 1)
     {
        ($main::acqfh) 
           = &loadlogfile(\$main::acqlogfile, \$main::acqbuffer);
       $main::acqloaded = 1 ;
     }
   }

      &acqstatus();
   #$window->queue_draw;
return $main::acqrunning ;
}

sub unset_acqchannels
{

       foreach my $b (@main::rbut)
       {
          $hboxradior->remove($b);
          undef($b);
       }
         @main::rbut = ();
         @main::acqchan = ();
}


sub call_acqstop 
{

   ($main::acqrunning, $main::acqlogfile) = 
                            &getstatus("mxacq stop  2>&1 |");

   if ($main::acqloaded == 1)
   {
     if ($main::acqrunning == 0)
     {
        &clearlogfile(\$main::acqfh, \$main::acqlogfile, \$main::acqbuffer);
       $main::acqloaded = 0 ;
     }
   }

}




sub call_recordstart 
{

   ($main::recordrunning, $main::recordlogfile) = 
                            &getstatus("mxrecord start $main::acqinpfile 2>&1 |");

   if ($main::recordloaded == 0)
   {
     if ($main::recordrunning == 1)
     {
        ($main::recordfh) 
           = &loadlogfile(\$main::recordlogfile, \$main::recordbuffer);
       $main::recordloaded = 1 ;
     }
   }

      &recordstatus();

 return $main::recordrunning ;
}


sub call_recordstop 
{

   ($main::recordrunning, $main::recordlogfile) = 
                            &getstatus("mxrecord stop 2>&1 |");

   if ($main::recordloaded == 1)
   {
     if ($main::recordrunning == 0)
     {
        &clearlogfile(\$main::recordfh, \$main::recordlogfile, \$main::recordbuffer);
       $main::recordloaded = 0 ;
     }
   }

}

sub unset_recordchannels
{

   foreach my $b (@main::rbut)
   {
       $b->set_active(FALSE);
   }

}


sub loadlogs 
{

  if ($main::recordloaded != 1)
  {
   if ($main::recordrunning == 1)
   {
      ($main::recordfh) 
         = &loadlogfile(\$main::recordlogfile, \$main::recordbuffer);
     $main::recordloaded = 1 ;
   }
  }


  if ($main::acqloaded != 1)
  {
   if ($main::acqrunning == 1)
   {
      ($main::acqfh) 
         = &loadlogfile(\$main::acqlogfile, \$main::acqbuffer);
     $main::acqloaded = 1 ;
   }
  }
}


sub getfilecontent
{
  my ($filename) = @_ ;

   my $tfh ;
   #open ($tfh, "<$filename") or die "can't open file $filename";
   open ($tfh, "tail -n 100  $filename | ") or die "can't open file $filename";
   my @content = <$tfh> ;
   my $tag ;
   #$tag = Gtk2::Helper->add_watch ( $tfh->fileno, 'in',
   #                   sub { pipewatch_callback($tfh, $tag);});
   close($tfh);
 return ($tag, $tfh, @content);
}


sub update_logs
{

  my ($b) = @_ ;
 
    my $la = $b->get_label ;

   if ($la =~ /Show Log/ )
   {
    return 1 ;
   }

 if ($main::acqrunning == 1)
 {
 ($main::acqfh)
           = &loadlogfile(\$main::acqlogfile, \$main::acqbuffer);
 #  my $aiter = $main::acqbuffer->get_end_iter();
 #     $main::acqbuffer->delete ($aiter, $aiter)

 }
 if ($main::recordrunning == 1)
 {
 ($main::recordfh)
           = &loadlogfile(\$main::recordlogfile, \$main::recordbuffer);
 #  my $riter = $main::recordbuffer->get_end_iter();
 #     $main::recordbuffer->delete ($riter, $riter)
 }

   &scroll_toend();

 return 1 ;
}

sub scroll_toend
{
 $main::tviewacq->scroll_to_iter( $main::acqbuffer->get_end_iter, 0, FALSE, 0, 0 );
 $main::tviewrecord->scroll_to_iter( $main::recordbuffer->get_end_iter, 0, FALSE, 0, 0 );
}


sub run_command
{
   my ($str)= @_;
  my $tfh ;
  my $ret =  open($tfh, "$str");
  &printmesg ( "$str\n" ); 
   my @cont ;
  if ($tfh)
  {
   @cont = <$tfh> ;
  }
  &printmesg(@cont);
  close($tfh);
}

sub set_command
{
  my ($entry) = @_ ;
  my $str = $entry->get_text ;
  $entry->set_text("") ;
   if ( $str !~ /^$/ )
   {
      &run_command("$str 2>&1 & |");
   }
}


sub call_showlog
{
    my ($b) = @_ ;
 
    my $la = $b->get_label ;

 if ($la =~ /Hide Log/ )
 {
     $hboxright->hide ;
     $b->set_label("Show Log");
 }
 else
 {
     $hboxright->show ;
     $b->set_label("Hide Log");
 }

}

sub call_plot
{
    my ($b) = @_ ;
 
    my $la = $b->get_label ;

 if ($la =~ /Start Plot/ )
 {
     my $fh ;
    $main::plotpid = open ($fh, "| mxgplot  2>&1  & ");
    my @cont = <$fh> ;
    &printmesg(@cont);
    $main::plotpid = $main::plotpid+1 ;
     $b->set_label("Stop Plot");
 }
 else
 {
     my $fh ;
     open ($fh, "kill -9 $main::plotpid 2>&1 | ");
    my @cont = <$fh> ;
    &printmesg(@cont);
     $b->set_label("Start Plot");
 }

}

sub acqdefault
{
 push @main::parlist,
   {editl=> FALSE, editv=> TRUE, label => "samplingrate",  value =>  "10.0",         comment => "ms"  },
   {editl=> FALSE, editv=> TRUE, label => "acqlogfile",    value => "acq.log",     comment => ""    },
   {editl=> FALSE, editv=> TRUE, label => "channels",      value =>  "0,1",        comment => ""    },
   {editl=> FALSE, editv=> TRUE, label => "recorddir",     value =>  "/data",      comment => ""    },
   {editl=> FALSE, editv=> TRUE, label => "recordlogfile", value =>  "record.log", comment => ""    },
   {editl=> FALSE, editv=> TRUE, label => "observer",      value =>  "Jeyakumar",  comment => ""    }, 
   {editl=> FALSE, editv=> TRUE, label => "comment",      value =>  "Test Observations", comment => ""} 
   ;
}

sub acqinpfile 
{
   my %m ;
  foreach my $a (@main::parlist)
  {
    $m{$a->{label}} = $a->{value} ;
  }

  my $umaskold = umask ;
  umask 0000 ;

   my $tfh ;
  open ($tfh, "> $main::acqinpfile");
  if (not $tfh )
  {
     &printmesg("cannot write $main::acqinpfile\n");
     return 1 ;
  }


print $tfh "# Setup file for  acq                 #\n",
      "# line starting with '#' are ignored  #\n",
      "#\n",
      "# sampling rate in milliseconds\n",
      "samplingrate = $m{samplingrate}\n",
      "acqlogfile         = $m{acqlogfile}\n",
      "# the channel numbers for acquisition to be comma separated such as \n",
      "# channels =  0,1,5 \n",
      "channels     = $m{channels}\n",
      "#\n",
      "#\n",
      "# For record\n",
      "recorddir          = $m{recorddir}\n",
      "recordlogfile      =$m{recordlogfile}\n",
      "observer           =$m{observer}\n",
      "comment            =$m{comment}\n",
      "#\n",
      "#\n";
# $tfh->autoflush(1);
 select((select($tfh), $| = 1)[0]);
 close($tfh);
 umask  $umaskold ;
 #sleep(5);

 return 0 ;
}
