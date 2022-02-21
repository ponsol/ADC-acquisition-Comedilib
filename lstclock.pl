#!/usr/bin/perl -w

use warnings;
use strict;

use Astro::Time;
use Glib qw/TRUE FALSE/;
use Gtk2;


 Gtk2->init ;

 Glib::Timeout->add (1000,\&update_time);

 my $window = Gtk2::Window->new('popup');
 #my $window = Gtk2::Window->new();

 my $vbox = Gtk2::VBox->new(FALSE,0);
   
   my $eventbox = Gtk2::EventBox->new();


   $window->signal_connect (destroy => sub { Gtk2->main_quit; });
   $window->signal_connect('button-press-event' =>  \&popup_menu  );

  #image contains nothing
  my $img = Gtk2::Image->new_from_pixmap (undef, undef);

    $eventbox->add($img);

   $vbox->pack_start($eventbox,FALSE,FALSE,0);
   $window->add($vbox);
   $window->set_position('mouse');
   $window->show_all();
    

     my $drawing_area = Gtk2::DrawingArea->new;
     $drawing_area->set_size_request (1600, 1600);
     $vbox->pack_start($drawing_area,FALSE,FALSE,0);
                                                                                
     $drawing_area->realize;

       my $pango_layout = $drawing_area->create_pango_layout("");
       my $colormap = Gtk2::Gdk::Colormap->get_system;
        #initial paint of screen
        &update_time;
        my  ($rx, $ry) = $window->get_position ;
        $ry=0;
        $rx=20;
        $window->move ($rx, $ry) ;
     Gtk2->main;


sub update_time {

   my ($ts, $ts1) = gettimes();

    $pango_layout->set_markup("<span background = '#BBBBBB' foreground= '#008800' size='50000' weight = 'ultralight'><b>$ts</b></span><span background = '#BBBBBB' foreground= '#FF0000' size='50000' weight = 'ultralight'><b>     $ts1</b></span>");
    my($pango_w,$pango_h)=$pango_layout->get_pixel_size;

     my $pixmap = Gtk2::Gdk::Pixmap->new ($drawing_area->window,
                                    $pango_w,
                                    $pango_h,
                                    -1);

    $pixmap->draw_rectangle ($drawing_area->style->black_gc,
                        TRUE,
                        0, 0,
                        $pango_w,
                        $pango_h);


    $pixmap->draw_layout($drawing_area->style->white_gc,0,0,$pango_layout);


  # my   $pixbuf = Gtk2::Gdk::Pixbuf->new ('rgb', TRUE, 8, $pango_w, $pango_h);


  #    $pixbuf->get_from_drawable ($pixmap, $colormap, 0, 0, 0,0, $pango_w, $pango_h);
  #    $pixbuf = $pixbuf->add_alpha (TRUE, 0, 0 , 0);


  #            my ($pm, $m) = $pixbuf->render_pixmap_and_mask (255);
    $window->resize(4,4);

#    $img->set_from_pixmap ($pm, $m);
    $img->set_from_pixmap ($pixmap, undef);

    #$window->shape_combine_mask ($m, 0, 0);

   #undef $pango_w, $pango_h ;
   #undef $pm, $m;
   #undef $ts, $ts1;   
   #undef $pixbuf ;
   #undef $pixmap ;

   return TRUE;
}



sub gettimes {

  my $lt = time();
 my($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst)= localtime($lt);
 my($gsec,$gmin,$ghour,$gmday,$gmon,$gyear,$gwday,$gyday,$gisdst)= gmtime($lt);

   ($sec =~ m/\d{2}/)||($sec = "0".$sec);
        ($min =~ m/\d{2}/)||($min = "0".$min);
        ($hour =~ m/\d{2}/)||($hour = "0".$hour);


     my $longitude =0 ;
     $longitude  = -(101.0+41.0/60.0) ;
  
    my $lturn = deg2turn($longitude);

     my $uth ;
     $uth = $ghour + $gmin/60.0 + $gsec/60.0/60.0 ;
     $uth /= 24.0 ;
     my $lst = cal2lst($gmday, $gmon+1, $gyear+1900, $uth, $lturn);

  
     $lst *= 24.0 ;
     my $lsth = $lst%24 ;
     my $lstm = ($lst - $lst%24)*60 ;
     my $lsts = ($lstm - $lstm%60)*60 ;
        $lstm = $lstm%60 ;
        $lsts = $lsts%60 ;


        ($gsec =~ m/\d{2}/)||($gsec = "0".$gsec);
        ($gmin =~ m/\d{2}/)||($gmin = "0".$gmin);
        ($ghour =~ m/\d{2}/)||($ghour = "0".$ghour);
#        $lsth = $ghour ;
#        $lstm = $gmin ;
#        $lsts = $gsec ;

 my $ts ;
 my $ts1 ;
 $ts = sprintf "CST: %02d:%02d:%02d",
              $hour, $min, $sec ;
 $ts1 = sprintf "LST: %02d:%02d:%02d ",
               $lsth, $lstm, $lsts ;

  #$ts =  sprintf "HH: %d %d %d", $gmday, $gmon+1, $gyear+1900;
 return ($ts, $ts1) ;
}

sub button_clicked {

	my $widget = shift;
	my $data   = shift;

   if ($data eq "forgetbutton" )
   {
     my $p = $widget->parent;
      $p = $p->parent;
      $p = $p->parent;
     $p->destroy;
   }
   if ($data eq "quitbutton" )
   {
     my $p = $widget->parent;
     $p->destroy;
     Gtk2->main_quit;
   }

}

sub popup_menu {

      my $child = Gtk2::Window->new('popup');
       my $frame = Gtk2::Frame->new (undef) ;
        $child->add($frame);

        my $hbox = Gtk2::HBox->new(FALSE,0);
        $frame->add($hbox);

         my $forgetbutton = Gtk2::Button->new("Forget");
           $forgetbutton->signal_connect("clicked" => \&button_clicked, "forgetbutton");
        $hbox->pack_start($forgetbutton,FALSE,FALSE,0);

        my $quitbutton = Gtk2::Button->new("Quit");
         $quitbutton->signal_connect("clicked" => \&button_clicked, "quitbutton");
        $hbox->pack_start($quitbutton,FALSE,FALSE,0);

        $child->show_all;
        $child->realize;

return FALSE ;
}

