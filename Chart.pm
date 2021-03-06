package Chart;

use warnings;
use strict;
use Glib qw/TRUE FALSE/;
use Gtk2;
use Gtk2::Gdk::Keysyms;
use Gtk2::Helper;
use FileHandle;

use constant MIN_CHART_WIDTH  => 456;
use constant MIN_CHART_HEIGHT => 150;

use constant MAX_DATA_LENGTH => 320000;
#use constant BUFFER_SIZE => 589824;
use constant BUFFER_SIZE =>  MAX_DATA_LENGTH/4 ;

my %drag_info;
use constant DRAG_PAD => 4;
use constant MAX_CHAN => 64 ;

#my  @datamax ;
#my  @datamin ;

# contains the channel-number of the
#channels to be plotted 
my  @plotchannels ;
my  @channelsfh ;
my  @channelstag ;
my  $savedx ;
my  $savedxyes=0 ;
my  @sbuffer ;


sub min {
        my $min = shift(@_);
        foreach my $foo (@_) {
            $min = $foo if $min > $foo;
        }
       return $min;
}



sub max {
        my $max = shift(@_);
        foreach my $foo (@_) {
            $max = $foo if $max < $foo;
        }
       return $max;
}

sub datatoscreen {
	my ($plot, $dx, $dy) = @_;

  my $cx = $dx*$plot->{slopex} + $plot->{constx} ;
  my $cy = $dy*$plot->{slopey} + $plot->{consty} ;

 return ($cx, $cy);
}

sub screentodata {

  my ($plot, $cx, $cy) = @_;

  my $dx = ($cx - $plot->{constx})/ $plot->{slopex}  ;
  my $dy = ($cy - $plot->{consty})/ $plot->{slopey}  ;
 return ($dx, $dy);
}

use Glib::Object::Subclass
	'Gtk2::DrawingArea',
	signals => {
		offsetzero_changed => {
			method      => 'do_offsetzero_changed',
			flags       => [qw/run-first/],
			return_type => undef, 
			param_types => [], 
		},

		size_request =>         __PACKAGE__.'::size_request',
		expose_event =>         __PACKAGE__.'::expose_event',
		configure_event =>      __PACKAGE__.'::configure_event',
		motion_notify_event =>  __PACKAGE__.'::motion_notify_event',
		button_press_event =>   __PACKAGE__.'::button_press_event',
		button_release_event => __PACKAGE__.'::button_release_event',
		key_press_event =>      __PACKAGE__.'::key_press_event',
		focus_in_event =>       __PACKAGE__.'::focus_events',
		focus_out_event =>      __PACKAGE__.'::focus_events',
	},
	properties => [
		Glib::ParamSpec->uint ('nchan',
		                         'Nchan',
		                         'Nchan',
		                          0, MAX_CHAN, 0,
		                         [qw/readable writable/]),
		Glib::ParamSpec->double ('offsetzero',
		                         'Offset Zero',
		                         'Offset Zero',
		                          -1.E308, 1.E308, 0.0,
		                         [qw/readable writable/]),
		Glib::ParamSpec->double ('zoom',
		                         'Zoom Scale',
		                         'Zoom Scale',
		                          -100, 100, 0,
		                         [qw/readable writable/]),
		Glib::ParamSpec->double ('minscale',
		                         'Min Scale',
		                         'Min Scale',
		                          0, 0, 0,
		                         [qw/readable writable/]),
		Glib::ParamSpec->double ('maxscale',
		                         'Max Scale',
		                         'Max Scale',
		                          0, 0, 0,
		                         [qw/readable writable/]),
		Glib::ParamSpec->boxed ('data',
		                        'plot Data',
		                        'Array reference containing plot data',
		                        'Glib::Scalar',
		                        [qw/readable writable/]),
	],
;


sub INIT_INSTANCE {
	my $plot = shift;
	#warn "INIT_INSTANCE $plot";
        my @tc ;
        my @fill ;

	$plot->can_focus (TRUE);

	$plot->{minscale}       = -1;
	$plot->{maxscale}       = 1;
	$plot->{xmin}       = 0;
	$plot->{xmax}       = 1;
	$plot->{plotfrac}       = 0;
	$plot->{slopex}       = 1;
	$plot->{slopey}       = 1;
	$plot->{constx}       = 0;
	$plot->{consty}       = 0;
	$plot->{zoom}          = 0;
	$plot->{setoffset}     = 0;
	@tc   = map { 0 } (0..MAX_CHAN)  ;
	@fill  = map { 0 } (0..MAX_CHAN)  ;
	$plot->{channels}       = \@tc ;
	$plot->{channelfill}    = \@fill ;
	$plot->{gc}             = undef ;
	$plot->{pixmap}         = undef;
	$plot->{offsetline}     = undef;
	$plot->{dragging}       = FALSE;
	$plot->{continuous}     = 0;
	$plot->{nchan}          = 0;
	$plot->{datalength}     = MAX_DATA_LENGTH/2;
	$plot->{rms_layout}   = $plot->create_pango_layout ("RMS");
#	$plot->{maxscale_layout} = $plot->create_pango_layout ("255");
#	$plot->{minscale_layout} = $plot->create_pango_layout ("0");

	$plot->{cheight}      = 0;
	$plot->{cbottom}      = 0;
	$plot->{ctop}         = 0;
	$plot->{cleft}        = 0;
	$plot->{cright}       = 0;
	$plot->{cwidth}       = 0;
        $plot->{colormap}     = Gtk2::Gdk::Colormap->get_system;

	$plot->set_events ([qw/exposure-mask
			       leave-notify-mask
			       button-press-mask
			       button-release-mask
			       pointer-motion-mask
			       pointer-motion-hint-mask/]);

        #open the MAX_CHAN pipes 
        my @dptrarray ;
        @dptrarray =  map { 0 } (0..2*MAX_CHAN-1) ;
        my $umaskold = umask ;
        umask 0000 ;
        for (my $i = 0 ; $i < MAX_CHAN  ; $i++ )
        {
             my $tbuf = "" ;
             $sbuffer[$i] = \$tbuf ;
             $plotchannels[$i] = 0 ;
             my $tfh;
             my $pipename = sprintf "/tmp/_chan%02d", $i ; 
             #create pipes if they are not existing
             if ( not -p $pipename ) {
	       system("mkfifo -m 666 $pipename") && 
                         die "Can't mkfifo $pipename: $!";
             }
             open ($tfh, "+<$pipename") or die "can't open pipe";
             $tfh->autoflush(1);
             push @channelsfh, $tfh ;
        }
	$plot->{data}   = \@dptrarray ;
        $plot->{nchan} = 0 ;
        umask $umaskold;

        #add watcher to all the  pipes 
        #if a pipe is active call the  callback with
        #the corresponding filehandle
        for (my $i = 0 ; $i < MAX_CHAN  ; $i++ )
        {
         my $tag;
         my $tfh = $channelsfh[$i] ;
         $tag = Gtk2::Helper->add_watch ( $tfh->fileno, 'in', 
                       sub { pipewatch_callback($plot, $tfh, $tag);});
         push @channelstag, $tag ;
        }
}

sub pipewatch_callback {
      my ($plot, $curfh, $curtag) = @_;
my $buffer ;

      my @tchannels = @{$plot->{channels}} ;
      my $tnchan  =  $plot->{nchan} ;
      my $fill = $plot->{channelfill} ;
      my $ret = 0 ;
     for (my $i = 0 ; $i < MAX_CHAN  ; $i++ )
     {
       my $tfh = $channelsfh[$i] ;
       if ($tfh == $curfh )
       {
          $buffer = $sbuffer[$i] ;
           my $tbuf ;
           $| = 1 ;
           $ret = 0;
            $ret =  sysread($curfh, $tbuf, BUFFER_SIZE) ; 
           #my $ret1 =  syswrite($curfh, 0,0) ; 
           
#      print "$buffer\n";
           if ( not $ret ) 
           {
              #remove this  channel from plotting
              if ( $tchannels[$i] == 1 )
              {
                 my $dptr = $plot->{data} ;
               $tchannels[$i] = 0  ;
               $tnchan-- ;
               undef @{${$dptr}[2*$i+1]}  ;
               undef @{${$dptr}[2*$i]}  ;
               ${$dptr}[2*$i+1] = 0 ;
               ${$dptr}[2*$i] = 0 ;

              }
           }
           else
           {
              #append the data
              if ($tbuf =~ /(.*?)END(.*?)$/ )
              {   
                 #new channel opened 
                 if ( $tchannels[$i] == 0 )
                 {
                     $tchannels[$i] = 1  ;
                     $tnchan++ ;
                    my $dptr = $plot->{data} ;
                    my $xx = ${$dptr}[2*$i] ;
                    my $yy = ${$dptr}[2*$i+1] ;
 
                  if ( $yy == 0 )
                  {
                   my   @ydata =  map { 0 } (0..$plot->{datalength}-1) ;
                   my   @xdata =  map { 0 } (0..$plot->{datalength}-1) ;
                     ${$dptr}[2*$i+1] =  \@ydata ;
                     ${$dptr}[2*$i] =  \@xdata ;
                  }
                 }

                $$buffer = $$buffer . $1 ;
               my @all = split( '\s+', $$buffer) ;
               my @xd ;
               my @yd ;
               my $nu  = scalar(@all) ;


               for (my $k = 0 ; $k < $nu ; $k=$k+2 )
               {

                  push (@xd, $all[$k]);
                  push (@yd, $all[$k+1]);
           #       <STDIN>
               }
                $$buffer = $2 ;

               my @dptr = @{$plot->{data}} ;
               my $xx = $dptr[2*$i] ;
               my $yy = $dptr[2*$i+1] ;
               my $nd = scalar(@$xx) ;
               my $nnew = scalar(@yd) ;

               print "Received $i numbers  $nu samples $nnew \n";
               
               my $tfill = $$fill[$i]  ;
              $$fill[$i] =  $tfill+$nnew > $plot->{datalength} ? $plot->{datalength} : $tfill+$nnew ;
                splice(@$xx, 0, $nd-$nnew, @$xx[$nnew..$nd-1]);
                splice(@$yy, 0, $nd-$nnew, @$yy[$nnew..$nd-1]);
                splice(@$xx, $nd-$nnew,$nnew, @xd);
                splice(@$yy, $nd-$nnew,$nnew, @yd);
              }   
              else
              {   
               $$buffer = $$buffer . $tbuf ;
              }   
           }
       }
     }

      if ( $plot->{channels} ) {
           delete $plot->{channels} ; 
           $plot->{channels} = \@tchannels ;
           $plot->{nchan} = $tnchan ;
           print "NCHAN $plot->{nchan} \n";
      }
     #call the plotter
     $plot->setdataready();
     $plot->plotdata ;
     $plot->queue_draw;


 return 1;
}



sub GET_PROPERTY {
	my ($plot, $pspec) = @_;
	if ($pspec->get_name eq 'offsetzero') {
		return $plot->{offsetzero};
	} elsif ($pspec->get_name eq 'data') {
		return $plot->{data};
	} elsif ($pspec->get_name eq 'minscale') {
		return $plot->{minscale};
	} elsif ($pspec->get_name eq 'maxscale') {
		return $plot->{maxscale};
	} elsif ($pspec->get_name eq 'zoom') {
		return $plot->{zoom};
	}
}


sub SET_PROPERTY {
	my ($plot, $pspec, $newval) = @_;
	if ($pspec->get_name eq 'offsetzero') {
		$plot->set_plot_offsetzero ($newval);
	} elsif ($pspec->get_name eq 'data') {
		$plot->set_plot_data ($newval);
	} elsif ($pspec->get_name eq 'minscale') {
		$plot->{minscale} = $newval;
	} elsif ($pspec->get_name eq 'channels') {
		$plot->set_channels ($newval);
	} elsif ($pspec->get_name eq 'zoom') {
		$plot->set_zoom ($newval);
	}
}





sub getdims {

        my $plot = shift;
                                                                                
        my $context  = $plot->{rms_layout}->get_context;
        my $fontdesc = $context->get_font_description;
        my $metrics  = $context->get_metrics ($fontdesc, undef);
                                                                                
        $plot->{textwidth} = 5 * $metrics->get_approximate_digit_width
                           / Gtk2::Pango->scale; #PANGO_SCALE;
        $plot->{textheight} = ($metrics->get_descent + $metrics->get_ascent)
                            / Gtk2::Pango->scale; #PANGO_SCALE;
                                                                                
        $plot->{cleft} =  2*$plot->{textwidth};
        $plot->{cright} = $plot->allocation->width -   1*$plot->{textwidth};
        $plot->{cwidth} = $plot->{cright} - $plot->{cleft};
        $plot->{ctop} =  4*$plot->{textheight} ;
        $plot->{cbottom} = $plot->allocation->height -  3*$plot->{textheight};
        $plot->{cheight}  = $plot->{cbottom} - $plot->{ctop};
}


sub draw_border {
   my ($plot, $slopex, $constx, $slopey, $consty) = @_;

    my $gc = $plot->{gc} ;
 
    my $black = Gtk2::Gdk::Color->parse('black');
       $gc->set_rgb_fg_color($black);

     my @cpoints ;
     my $x ; my $y ;
     $x = $plot->{cleft};
     $y = $plot->{cbottom};
     push @cpoints, $x ;
     push @cpoints, $y ;
     $x = $plot->{cright};
     $y = $plot->{cbottom};
     push @cpoints, $x ;
     push @cpoints, $y ;
    $plot->{pixmap}->draw_lines ($gc,  @cpoints);

     @cpoints = () ;
     $x = $plot->{cleft};
     $y = $plot->{ctop};
     push @cpoints, $x ;
     push @cpoints, $y ;
     $x = $plot->{cright};
     $y = $plot->{ctop};
     push @cpoints, $x ;
     push @cpoints, $y ;
    $plot->{pixmap}->draw_lines ($gc,  @cpoints);

     @cpoints = () ;
     $x = $plot->{cleft};
     $y = $plot->{ctop};
     push @cpoints, $x ;
     push @cpoints, $y ;
     $x = $plot->{cleft};
     $y = $plot->{cbottom};
     push @cpoints, $x ;
     push @cpoints, $y ;
    $plot->{pixmap}->draw_lines ($gc,  @cpoints);


     @cpoints = () ;
     $x = $plot->{cright};
     $y = $plot->{ctop};
     push @cpoints, $x ;
     push @cpoints, $y ;
     $x = $plot->{cright};
     $y = $plot->{cbottom};
     push @cpoints, $x ;
     push @cpoints, $y ;
    $plot->{pixmap}->draw_lines ($gc,  @cpoints);

}

sub set_channels {
        my ($plot, $chanptr) = @_;
    
           if ( $plot->{channels} ) 
           { delete $plot->{channles} ; }
        $plot->{channels} = $chanptr ;
        my @tchan =  @{$plot->{channels}} ;
        my $nchan  = 0 ;
        for (my $i=0 ; $i < MAX_CHAN ; $i++ )
        {
          if ( $tchan[$i] == 1 )
          {
             $plotchannels[$i] = $i ;
             $nchan++ ;
          }
        }
        $plot->{nchan} = $nchan ;

        $plot->getdims();
}

sub set_zoom {
        my ($plot, $zoom) = @_;
    
        $plot->{zoom} = $zoom ;
        $plot->getdims();
        $plot->plotdata();
        $plot->queue_draw;
}



sub set_plot_offsetzero {
        my ($plot, $offsetzero) = @_;
        $plot->{offsetzero} = $offsetzero ;

        $plot->getdims();

	if ($plot->{pixmap}) {
                $plot->plotdata;
		$plot->queue_draw;
	}
}


sub set_plot_data {
        my ($plot, $dptr) = @_;
                                                                                
        $plot->{data} = $dptr ;

	if ($plot->{pixmap}) {
                $plot->plotdata;
		$plot->queue_draw;
	}
}


#called during initialisation
sub size_request {
        my ($plot, $requisition) = @_;
        #warn "in class override for $_[0]\::size_request";
                                                                                
        $requisition->width ($plot->{textwidth} + 2 + MIN_CHART_WIDTH);
        $requisition->height ($plot->{textheight} + MIN_CHART_HEIGHT);
                                                                                
        # chain up to the parent class.
        shift->signal_chain_from_overridden (@_);
}


sub expose_event {
        my ($plot, $event) = @_;


        $plot->window->draw_drawable ($plot->style->fg_gc($plot->state),
                                      $plot->{pixmap},
                                      $event->area->x, $event->area->y,
                                      $event->area->x, $event->area->y,
                                      $event->area->width, $event->area->height);
        return FALSE;
}


sub getxminmax {
       my ($plot) = @_;

      my @tchannels = @{$plot->{channels}} ;
      my @dptrarray = @{$plot->{data}} ;
      my $xmax ;
      my $xmin ;

      my $first = 0 ;
      my $j = 0 ;
      for (my $i=1 ; $i < $plot->{nchan}+1 ; $i++ )
      {

          if ( $tchannels[$i-1] == 1 )
          {
             my $xx = $dptrarray[2*($i-1)] ;
             my $ndat = scalar(@$xx) ;
             my $fill = ${$plot->{channelfill}}[$i-1];
             my $txmax = max(@$xx[$ndat-$fill..$ndat-1]) ;
             my $txmin = min(@$xx[$ndat-$fill..$ndat-1]) ;
             if ($fill != 0 )
             {
               if ($first == 0 )
               {
                $xmin = $txmin ;
                $xmax = $txmax ;
                  $first = 1 ;
               }
                if ($txmin < $xmin ) {$xmin = $txmin; } 
                if ($txmax > $xmax ) {$xmax = $txmax; }  
             }
          }
      }
   $plot->{xmin} = $xmin;
   $plot->{xmax} = $xmax;
}

sub getyminmax {
       my ($plot) = @_;

      my @tchannels = @{$plot->{channels}} ;
      my @dptrarray = @{$plot->{data}} ;
      my $ymax ;
      my $ymin ;

      my $j = 0 ;
      for (my $i=1 ; $i < $plot->{nchan}+1 ; $i++ )
      {


          if ( $tchannels[$i-1] == 1 )
          {
             my $fill = ${$plot->{channelfill}}[$i-1];
             if ($fill != 0 )
             {
                my $yy = $dptrarray[2*($i-1)+1] ;
                my $ndat = scalar(@$yy) ;
                my $tmax = max(@$yy[$ndat-$fill..$ndat-1]) ;
                my $tmin = min(@$yy[$ndat-$fill..$ndat-1]) ;

                  if ($j == 0 )
                  {
                    $ymax = $tmax ;
                    $ymin = $tmin ;
                  }
                if ( $tmax > $ymax ) { $ymax = $tmax; }
                if ( $tmin < $ymin ) { $ymin = $tmin; }
               $j++ ;
              }
          }
      }

   $plot->{minscale} = $ymin;
   $plot->{maxscale} = $ymax;

}

sub setdataready {
       my ($plot) = @_;
       $plot->getyminmax();
       $plot->getxminmax();
}

sub getscales {
       my ($plot,$ichan) = @_;
       my @dptrarray = @{$plot->{data}} ;

       my $ymax ;
       my $ymin ;
       my $xx = $dptrarray[2*$ichan] ;
       my $ndat = scalar(@$xx) ;
       my $fill = ${$plot->{channelfill}}[$ichan];
#       my $slopex = ($fill/$ndat)*($plot->{cright}-$plot->{cleft})  / ($$xx[$ndat-1]-$$xx[$ndat-$fill]) ;
       my $frac = $fill/$plot->{datalength} ;
       print "FRAC: $frac\n";
       my $slopex = $frac*($plot->{cright}-$plot->{cleft}) / 
             ($plot->{xmax} - $plot->{xmin})  ;

#      my $ttx = ($$xx[$ndat-1]-$$xx[$ndat-$fill]) ;

      my $constx = $plot->{cright} - $slopex*$plot->{xmax} ;

      my @tchannels = @{$plot->{channels}} ;




   
      my $dwidth ;
        $ymax = $plot->{maxscale} ;
        $ymin = $plot->{minscale} ;
        $dwidth = $ymax - $ymin ;
      if ( $plot->{setoffset} == 0 )
      {
          $plot->{setoffset} = 1 ;
          $plot->{offsetzero} = ($ymax + $ymin)/2.0 ;
      }
      $plot->{maxscale}  =  $plot->{offsetzero} + $dwidth/2.0 - 
                              $plot->{zoom} *($dwidth/2.0)/100.0 ;
      $plot->{minscale}  =  $plot->{offsetzero} - $dwidth/2.0 + 
                              $plot->{zoom} *($dwidth/2.0)/100.0 ;
#           print "YMIN YMAX: $plot->{minscale}   $plot->{maxscale} \n";
     $ymax = $plot->{maxscale} ;
     $ymin = $plot->{minscale} ;
     my $slopey = ($plot->{ctop}-$plot->{cbottom})/ ($ymax-$ymin) ;
     my $consty = $plot->{ctop} - $slopey*$ymax ;

     $plot->{slopex} = $slopex ;
     $plot->{slopey} = $slopey ;
     $plot->{constx} = $constx ;
     $plot->{consty} = $consty ;
 return ($slopex, $constx, $slopey, $consty) ;
}

sub setlabeltransparent {

my ($plot, $layout, $ttx, $tty) = @_ ;

    my($pango_w,$pango_h)=$layout->get_pixel_size;
    my $pixbuf = Gtk2::Gdk::Pixbuf->new ('rgb', TRUE, 8, $pango_w, $pango_h);
    $pixbuf->get_from_drawable ($plot->{pixmap}, $plot->{colormap}, 0, 0, 0,0, $pango_w, $pango_h);
    $pixbuf = $pixbuf->add_alpha (TRUE, 0, 0 , 0);
    my ($pm, $m) = $pixbuf->render_pixmap_and_mask (255);

    my $src_x = 0 ;
    my $src_y = 0 ;
    my $dest_x = $ttx ;
    my $dest_y = $tty ;
    my $w   = $pango_w ;
    my $h   = $pango_h ;
    my $dither = 'none' ;
    my $x_dither = 0 ;
    my $y_dither  = 0;
    my $gc= $plot->{gc} ;
    $plot->{pixmap}->draw_pixbuf ($gc, $pixbuf,
    $src_x, $src_y, $dest_x, $dest_y, $w, $h, $dither,
    $x_dither, $y_dither) ;
    $plot->shape_combine_mask ($m, 0, 0);
}

sub draw_offset_line {

 my ($plot, $pm, $yes) = @_;


        if (!$plot->{ogc}) {
                $plot->{ogc} = Gtk2::Gdk::GC->new ($plot->{pixmap});
                $plot->{ogc}->copy ($plot->style->fg_gc ($plot->state));
                $plot->{ogc}->set_function ('invert');
        }

        my ($sx, $sy) = $plot->datatoscreen($plot->{xmin}, $plot->{offsetzero});
        $pm->draw_line ($plot->{ogc},
                       $plot->{cleft}, $sy,
                       $plot->{cright}, $sy);

    my  ($textwidth, $textheight) = $plot->{rms_layout}->get_pixel_size;

}

sub draw_xlabel {
  my ($plot, $xlabel) = @_ ;

      my $gc = $plot->{gc} ;
    my $black = Gtk2::Gdk::Color->parse('black');
      $gc->set_rgb_fg_color($black);

   my  $labellayout = $plot->create_pango_layout ("");
    $labellayout->set_markup("<span background = '#FFFFFF' foreground= '#008800' size='10000' weight = 'ultralight'><b>$xlabel</b></span>");

      my $s =  Gtk2::Pango->scale ;
      my ($lw, $lh) = $labellayout->get_size ;
       $lw /= $s ;
       $lh /= $s ;
     my $tty = $plot->{cbottom}+2*$plot->{textheight} ;
     my $ttx = $plot->{cleft} + ($plot->{cright} - $plot->{cleft} ) / 2.0;
     $plot->{pixmap}->draw_layout ($gc, $ttx-$lw/2.0, $tty-$lh/2.0, $labellayout);

}

sub draw_title {
  my ($plot, $title) = @_ ;

      my $gc = $plot->{gc} ;
    my $black = Gtk2::Gdk::Color->parse('black');
      $gc->set_rgb_fg_color($black);

   my  $titlelayout = $plot->create_pango_layout ("");
    $titlelayout->set_markup("<span background = '#FFFFFF' foreground= '#FF0000' size='12000' weight = 'ultralight'><b>$title</b></span>");

      my $s =  Gtk2::Pango->scale ;
      my ($lw, $lh) = $titlelayout->get_size ;
       $lw /= $s ;
       $lh /= $s ;
     my $tty = $plot->{ctop}-2.0*$plot->{textheight} ;
     my $ttx = $plot->{cleft} + ($plot->{cright} - $plot->{cleft} ) / 2.0;
     $plot->{pixmap}->draw_layout ($gc, $ttx-$lw/2.0, $tty-$lh/2.0, $titlelayout);

}


sub draw_yticks {
  my ($plot, $ybottom, $ytop, $nticks) = @_ ;

    my $gc = $plot->{gc} ;
    my $black = Gtk2::Gdk::Color->parse('black');
      $gc->set_rgb_fg_color($black);

  my $tlength = (1.0/4)*$plot->{textwidth} ;
  my  $dy = ( $plot->{ctop} - $plot->{cbottom} ) / ($nticks-1) ;
  my  $dyy = ( $ytop - $ybottom) / ($nticks-1) ;
  for ( my $i=0 ; $i < $nticks ; $i++ )
  {
     my @cpoints ;
     my $tty = $plot->{cbottom} + $i*$dy ;
     my $ttx = $plot->{cleft}+$tlength ;
     push @cpoints, $ttx, $tty ;
      $ttx = $plot->{cleft} ;
     push @cpoints, $ttx, $tty ;
     $plot->{pixmap}->draw_lines ($gc,  @cpoints);

     my $ticklabel ;
     my $y = $ybottom+$i*$dyy ;
     $ticklabel = sprintf "%11.3g", $y ;
      my  $layout = $plot->create_pango_layout ($ticklabel);
      my $context = $layout->get_context;
      my $fontdesc = $context->get_font_description;
      $layout->set_font_description ($fontdesc);
      my $s =  Gtk2::Pango->scale ;
      my ($lw, $lh) = $layout->get_size ;
      $lw = $lw / $s ;
      $lh = $lh / $s ;
       $ttx = $plot->{cleft}-$tlength ;
     $plot->{pixmap}->draw_layout ($gc, $ttx-$lw, $tty-$lh/2, $layout);

  }
}


sub draw_xticks {
  my ($plot, $xleft, $xright, $nticks) = @_ ;

    my $gc = $plot->{gc} ;
    my $black = Gtk2::Gdk::Color->parse('black');
      $gc->set_rgb_fg_color($black);

  my $tlength = (1.0/4)*$plot->{textwidth} ;
  my  $dx = ( $plot->{cright} - $plot->{cleft} ) / ($nticks-1) ;
  my  $dxx = ( $xright - $xleft) / ($nticks-1) ;
  for ( my $i=0 ; $i < $nticks ; $i++ )
  {
     my @cpoints ;
     my $ttx = $plot->{cleft} + $i*$dx ;
     my $tty = $plot->{cbottom}-$tlength ;
     push @cpoints, $ttx, $tty ;
       $tty = $plot->{cbottom} ;
     push @cpoints, $ttx, $tty ;
       $tty = $plot->{cbottom}+$tlength ;
     $plot->{pixmap}->draw_lines ($gc,  @cpoints);

     my $ticklabel ;
     my $x = $xleft+$i*$dxx ;
     my $xh ;
     if ($x < 0 ) { $x = 24.0+$x; }
      $xh=$x%24 ;
     my $xm = ($x - $x%24)*60 ;
     my $xs = ($xm - $xm%60)*60 ;
        $xm = $xm%60 ;
        $xs = $xs%60 ;
     $ticklabel = sprintf "%02d:%02d:%02d", $xh,$xm,$xs ;
      my  $layout = $plot->create_pango_layout ($ticklabel);
      my $context = $layout->get_context;
      my $fontdesc = $context->get_font_description;
      $layout->set_font_description ($fontdesc);
      my $s =  Gtk2::Pango->scale ;
      my ($lw, $lh) = $layout->get_size ;
      $lw = $lw / $s ;
     $plot->{pixmap}->draw_layout ($gc, $ttx-$lw/2.0, $tty, $layout);
  }

}

sub plotdata {
        my $plot = shift;
#       my $gc = $plot->style->fg_gc ($plot->state);
   

        my @colors ;
        my $red = Gtk2::Gdk::Color->parse('red');
        push @colors, $red ;
        my $green = Gtk2::Gdk::Color->parse('green');
        push @colors, $green ;
        my $blue  =  Gtk2::Gdk::Color->parse('blue');
        push @colors, $blue ;

        if (!$plot->{gc}) {
                $plot->{gc} = Gtk2::Gdk::GC->new ($plot->{pixmap}, undef);
                $plot->{gc}->copy ($plot->style->fg_gc ($plot->state));
#                $plot->{gc}->set_function ('invert');
        }
        my $gc = $plot->{gc} ;

        # erase the area
        my $white = Gtk2::Gdk::Color->parse('white');
        my $gc1 = Gtk2::Gdk::GC->new ($plot->{pixmap}, undef);
           $gc1->set_rgb_fg_color($white);
#                    ($plot->style->bg_gc ($plot->state),
        $plot->{pixmap}->draw_rectangle 
                    ($gc1,
                     TRUE, 0, 0,
                     $plot->allocation->width,
                     $plot->allocation->height);



        my @tchan =  @{$plot->{channels}} ;
        my $nchan  = 0 ;
        for (my $i=0 ; $i < MAX_CHAN ; $i++ )
        {
          if ( $tchan[$i] == 1 )
          {
             $plotchannels[$i] = $i ;
             $nchan++ ;
          }
        }
        $plot->{nchan} = $nchan ;


       my ($slopex, $constx, $slopey, $consty)  ;

      my @dptrarray = @{$plot->{data}} ;
      my $fill = \@{$plot->{channelfill}} ;
  for (my $i=1 ; $i < $plot->{nchan}+1 ; $i++ )
  {
       my $achan = $plotchannels[$i] ;
        #print "Chan $achan \n" ;
       my @xdata = @{$dptrarray[2*$achan]} ;
       my @ydata = @{$dptrarray[2*$achan+1]} ;
       my $ndat = scalar(@xdata) ;
       my $ndat1 = scalar(@ydata) ;
       if ($ndat != $ndat1 )
       { print "serious error $ndat $ndat1\n"; }
       if ( $ndat ) {


              for (my $j=$ndat-$$fill[$i-1] ; $j < $ndat-1 ; $j++ )
              {
                if ( $xdata[$j+1] <  $xdata[$j] ) 
                {
                 # print "serious error $xdata[$j+1] $xdata[$j]\n";
                 # $xdata[$j+1] += 24.0 ;
                }
              }

               my @cpoints = ();
              ($slopex, $constx, $slopey, $consty) = $plot->getscales($i-1);

              for (my $j=$ndat-$$fill[$i-1] ; $j < $ndat ; $j++ )
              {
                 #convert to chart coordinates 
                                
              my $cx = $xdata[$j]*$slopex + $constx ;
              my $cy = $ydata[$j]*$slopey + $consty ;
              #$cy = $cy - $plot->{ctop} ;
              push @cpoints,$cx, $cy ;
                                
              }

          #$plot->{pixmap}->draw_polygon ($gc, TRUE, @cpoints,
          #      $plot->allocation->width, $plot->{cbottom} + 1,
          #      $plot->{cleft}, $plot->{cbottom} + 1);
          

          $gc->set_rgb_fg_color($colors[$i-1]);
          $plot->{pixmap}->draw_lines ($gc,  @cpoints);

          my ($w, $h );

          $w = $plot->allocation->width ;
          $h =  $plot->{ctop};
          $plot->{pixmap}->draw_rectangle 
                    ($gc1,
                     TRUE, 0, 0, $w, $h);

          $h = $plot->allocation->height - $plot->{cbottom};
          $plot->{pixmap}->draw_rectangle 
                    ($gc1,
                     TRUE, 0, $plot->{cbottom}, $w, $h);
          $plot->draw_border($slopex, $constx, $slopey, $consty);

        }
  }

   if ( $plot->{nchan} > 0 )
   {
    my $achan = $plotchannels[$plot->{nchan}-1] ;
    my @xdata = @{$dptrarray[2*$achan]} ;
    my $ndat = scalar(@xdata) ;
    my $xleft = ($plot->{cleft} - $constx )/ $slopex ;
    my $xright = $xdata[$ndat-1] ;
    my $nticks = 5 ;
    draw_xticks($plot, $xleft, $xright, $nticks);
    my $ybottom = $plot->{minscale};
    my $ytop = $plot->{maxscale};
    $nticks = 4 ;
    draw_yticks($plot, $ybottom, $ytop, $nticks);
    draw_xlabel($plot, "CST");
    draw_title($plot, "MEXART  DATA");
    $plot->draw_offset_line ($plot->{pixmap}, TRUE )  ;
   }

}

sub draw_zoom_slider  {
        my ($plot) = @_;


}

sub configure_event {
        my ($plot, $event) = @_;


        $plot->{pixmap} = Gtk2::Gdk::Pixmap->new(
                     $plot->window,
                     $plot->allocation->width,
                     $plot->allocation->height,
                     -1); 

        draw_zoom_slider();
        $plot->getdims;

        $plot->setdataready();
        $plot->plotdata;
        $plot->realize ;
        return TRUE;
}

sub marker_hit {
        my ($plot, $screen_x, $screen_y) = @_;

        my ($dummy, $offy) = $plot->datatoscreen (0,$plot->{offsetzero});
          if ($screen_y > $offy - DRAG_PAD &&
              $screen_y < $offy + DRAG_PAD) {
                      return $offy;
            }
        return undef;
}

my $sizer ;
sub motion_notify_event {
        my ($plot, $event) = @_;

        my ($x, $y, $state);

        if ($event->is_hint) {
                (undef, $x, $y, $state) = $event->window->get_pointer;
        } else {
                $x = $event->x;
                $y = $event->y;
                $state = $event->state;
        }
        if ($plot->{dragging}) {
                return FALSE
                        unless $state >= 'button1-mask'
                            and defined $plot->{pixmap};

            #this is to erase
            $plot->draw_offset_line ($plot->window, FALSE);

            $y += $drag_info{offset_y};

                # confine to valid region
                $y = $plot->{ctop} if $y < $plot->{ctop};
                $y = $plot->{cbottom} if $y > $plot->{cbottom};

                (undef, $plot->{offsetzero}) = $plot->screentodata (0,$y);
                $plot->draw_offset_line ($plot->window, TRUE);

                $plot->signal_emit ("offsetzero-changed")
                        if $plot->{continuous};

        } else {
                my $c = undef;
                my $sy = $plot->marker_hit ($x, $y);
                if (defined $sy) {
                        $sizer = Gtk2::Gdk::Cursor->new ('GDK_SB_V_DOUBLE_ARROW')
                                if not defined $sizer;
                        $c = $sizer;
                }
                $plot->window->set_cursor ($c);
             
        }

        return TRUE;

}

sub button_press_event {

        my ($plot, $event) = @_;

        $plot->grab_focus if $plot->can_focus and not $plot->has_focus;

        return FALSE
                if ($event->button != 1 || not defined $plot->{pixmap});

        my $sy = $plot->marker_hit ($event->x, $event->y);
        return FALSE
                unless defined $sy;

       my ($osx, $osy) = $plot->datatoscreen (0, $plot->{offsetzero} );
       my  ($textwidth, $textheight) = $plot->{rms_layout}->get_pixel_size;
#        $plot->{offsetzero} = $plot->{offsetzero};
        $plot->draw_offset_line ($plot->{pixmap}, FALSE);
        $plot->window->draw_drawable ($plot->style->fg_gc($plot->state),
                                      $plot->{pixmap},
                        $plot->{cleft}, $osy - $textwidth, 
                        $plot->{cleft}, $osy - $textwidth,
                        $plot->{cright} -$plot->{cleft}, $textheight+1 );

#        $plot->window->draw_drawable ($plot->style->fg_gc ($plot->state),
#                                      $plot->{pixmap},
#                                      0, 0, 0, 0,
#                                      $plot->allocation->width,
#                                      $plot->allocation->height);
        # and draw the new one on the window.
        $plot->draw_offset_line ($plot->window, TRUE);
        $plot->{dragging} = TRUE;

        $drag_info{offset_y} = $osy - $event->y;

        return TRUE;

}

sub button_release_event {

        my ($plot, $event) = @_;

        return FALSE
                if ($event->button != 1
                    || !$plot->{dragging}
                    || not defined $plot->{pixmap});

        # erase the previous threshold line from the window...
        $plot->draw_offset_line ($plot->window, FALSE);
        my ($dx, $dy) =  $plot->screentodata (0, $event->y + $drag_info{offset_y});
        $plot->{offsetzero} = $dy ;
        # and draw the new one on the pixmap.
        $plot->draw_offset_line ($plot->{pixmap}, TRUE);

        $plot->window->draw_drawable ($plot->style->fg_gc ($plot->state),
                                      $plot->{pixmap},
                                      0, 0, 0, 0,
                                      $plot->allocation->width,
                                      $plot->allocation->height);


        $plot->{dragging} = FALSE;

        $plot->signal_emit ("offsetzero-changed");

        return TRUE;

}

sub do_offsetzero_changed {

   my ($plot ) = @_;
    
   $plot->plotdata ;
   $plot->queue_draw;
}


sub focus_events {
        my $plot = shift;
        my $ret = $plot->signal_chain_from_overridden (@_);

     #   $plot->draw_offset_line ($plot->{pixmap}, TRUE);

   return $ret;
}

1;
