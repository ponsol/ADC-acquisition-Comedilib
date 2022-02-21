#!/usr/bin/perl -w

use Glib qw/TRUE FALSE/;
use Gtk2 ; 

#require "$ENV{HOME}/bin/Chart.pm" ;
use Chart ;

   Gtk2->init ; 
   my $window = Gtk2::Window->new;
   $window->signal_connect (delete_event => 
                      sub { Gtk2->main_quit; FALSE });

   my $hbox = Gtk2::HBox->new;
   $window->add ($hbox);
   $window->set_border_width (6);

   my @ydat =  map { sin($_/125*3.1415) } (0..255) ;
   my @ydat1 =  map { sin($_/12*3.1415) } (0..255) ;
   my @xdat =  map { $_ } (0..255) ;

   my @adata ;
       push @adata, \@xdat, \@ydat, \@ydat1 ;
   my $dptr = \@adata;
#
# a nicely framed histogram plot with some cheesy data
#
my $plot = Chart->new (
        nchan => 0,
	offsetzero => 64 
);

my $frame = Gtk2::Frame->new;
$hbox->pack_start ($frame, TRUE, TRUE, 0);
$frame->add ($plot);
$frame->set_shadow_type ('in');

  my $zoom =  Gtk2::Adjustment->new(0, -100, 100, 1, 2, undef);
 
  $zoom->signal_connect(value_changed => \&set_zoom, $zoom);
  my $vscale = Gtk2::VScale->new($zoom);
  $hbox->pack_start ($vscale, FALSE, FALSE, 0);

  $plot->signal_connect (offsetzero_changed => sub {
	;
	});

  $window->show_all;
  Gtk2->main;

undef $plot;
undef $window;


sub set_zoom {
 my ($adj) = @_ ;
 my $zz = $adj->value ;
 $plot->set_zoom($zz) ;
}
