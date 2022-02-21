#include <stdlib.h>
#include <gtk/gtk.h>
#include "jchart.h"


int main( int   argc,
          char *argv[] )
{
  GtkWidget *window;
  GtkWidget *wid;
  
  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  
  gtk_window_set_title (GTK_WINDOW (window), "Aspect Frame");
  
  g_signal_connect (G_OBJECT (window), "destroy",
		    G_CALLBACK (exit), NULL);
  
  gtk_container_set_border_width (GTK_CONTAINER (window), 10);

  wid = jchart_new ();
  
  gtk_container_add (GTK_CONTAINER (window), wid);
  gtk_widget_show (wid);

  /* And attach to its "jchart" signal */
 // g_signal_connect (G_OBJECT (wid), "jchart",
//		    G_CALLBACK (win), NULL);

  gtk_widget_show (window);
  
  gtk_main ();
  
  return 0;
}

