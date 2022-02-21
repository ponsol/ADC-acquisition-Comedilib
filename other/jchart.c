#include <math.h>
#include <gtk/gtksignal.h>
#include <gtk/gtktable.h>
#include <gtk/gtkcheckbutton.h>
#include <gtk/gtkdrawingarea.h>
#include <gtk/gtkadjustment.h>
#include <gtk/gtkvscale.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkeventbox.h>
#include "jchart.h"


#include "jchart.h"

#define  JCHART_DEFAULT_WIDTH  700
#define  JCHART_DEFAULT_HEIGHT 200


static GtkWidgetClass *parent_class = NULL;


enum {
  JCHART_SIGNAL,
  LAST_SIGNAL
};

static void jchart_class_init          (JchartClass *mclass);
static void jchart_init                (Jchart      *jc);
static void jchart_toggle              (GtkWidget *widget, Jchart *jc);

static guint jchart_signals[LAST_SIGNAL] = { 0 };

GType jchart_get_type (void)
{
  static GType jc_type = 0;

  if (!jc_type)
    {
      static const GTypeInfo jc_info =
      {
	sizeof (JchartClass),
	NULL, /* base_init */
        NULL, /* base_finalize */
	(GClassInitFunc) jchart_class_init,
        NULL, /* class_finalize */
	NULL, /* class_data */
        sizeof (Jchart),
	0,
	(GInstanceInitFunc) jchart_init,
      };

      jc_type = g_type_register_static (GTK_TYPE_TABLE, "Jchart", &jc_info, 0);
    }

  return jc_type;
}


void jchart_clear (Jchart *jc)
{
  int i,j;

}

static void jchart_chan_clicked (GtkWidget *widget, Jchart *jc)
{

}

static void gtk_jchart_size_request (GtkWidget *wid,
                           GtkRequisition *requisition)
{
  requisition->width = JCHART_DEFAULT_WIDTH;
  requisition->height = JCHART_DEFAULT_HEIGHT;
}


static void  gtk_jchart_configure (GtkWidget *widget, GdkEventConfigure *event, gpointer data )
{
GdkPixmap  *pixmap ;
Jchart *jc ;

        jc  = (Jchart *) data ;
        pixmap = jc->PlotPixmap ;

   fprintf(stderr,"\nconfigure event called\n");
   getchar();
  if (pixmap == NULL )
   {
      g_object_unref(pixmap);
   }

   pixmap = gdk_pixmap_new(widget->window, widget->allocation.width, 
                               widget->allocation.height,-1);
   gdk_draw_rectangle (pixmap,
                      widget->style->white_gc,
                      TRUE,
                      0, 0,
                      widget->allocation.width,
                      widget->allocation.height);

//  return TRUE;
}


static void gtk_jchart_realize (GtkWidget *widget)
{
  Jchart *jc;
  GdkWindowAttr attributes;
  gint attributes_mask;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (IS_JCHART (widget));

  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
  jc = JCHART (widget);

  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.event_mask = gtk_widget_get_events (widget) |
    GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK |
    GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK |
    GDK_POINTER_MOTION_HINT_MASK;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);

  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
  widget->window = gdk_window_new (widget->parent->window, &attributes, attributes_mask);

  widget->style = gtk_style_attach (widget->style, widget->window);

  gdk_window_set_user_data (widget->window, widget);

  gtk_style_set_background (widget->style, widget->window, GTK_STATE_ACTIVE);

}

static gboolean
gtk_jchart_expose( GtkWidget      *widget,
		 GdkEventExpose *event )
{
Jchart   *jc ;  

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (IS_JCHART (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  if (event->count > 0)
    return FALSE;
  
  jc = JCHART (widget);

//    plotarea_expose_event (jc->PlotBox, event, (gpointer) jc );

return FALSE ;
}

static void
gtk_jchart_destroy (GtkObject *object)
{
  Jchart *jc;

  g_return_if_fail (object != NULL);
  g_return_if_fail (IS_JCHART (object));

  jc = JCHART (object);

  if (jc->PlotPixmap)
    gtk_object_unref (GTK_OBJECT (jc->PlotPixmap));




  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);

}

static void jchart_class_init (JchartClass *mclass)
{

  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkWidgetClass *parent_class ;

  object_class = (GtkObjectClass*) mclass;
  widget_class = (GtkWidgetClass*) mclass;
  parent_class = gtk_type_class (gtk_widget_get_type ());
  object_class->destroy = gtk_jchart_destroy;


//  widget_class->expose_event = gtk_jchart_expose;
//  widget_class->realize = gtk_jchart_realize;
  widget_class->size_request = gtk_jchart_size_request;

  //widget_class->configure_event = gtk_jchart_configure;

//  jchart_signals[JCHART_SIGNAL] = g_signal_new ("jchart",
//					 G_TYPE_FROM_CLASS (mclass),
//	                                 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
//	                                 G_STRUCT_OFFSET (JchartClass, jchart),
 //                                        NULL, 
  //                                       NULL,                
//					 g_cclosure_marshal_VOID__VOID,
 //                                        G_TYPE_NONE, 0);


}

gboolean  plotarea_configure_event (GtkWidget *widget, GdkEventConfigure *event, gpointer data )
{
GdkPixmap  *pixmap ;
Jchart *jc ;

        jc  = (Jchart *) data ;
        pixmap = jc->PlotPixmap ;

   fprintf(stderr,"\nconfigure event called\n");
   getchar();
  if (pixmap)
   {
      free(pixmap) ;
   }
   pixmap = gdk_pixmap_new(widget->window, widget->allocation.width, 
                               widget->allocation.height,-1);
   gdk_draw_rectangle (pixmap,
                      widget->style->white_gc,
                      TRUE,
                      0, 0,
                      widget->allocation.width,
                      widget->allocation.height);

  return TRUE;
}

gboolean plotarea_expose_event (GtkWidget *widget, GdkEventExpose *event,
                                gpointer data)
{
GdkPixmap  *pixmap ;  
GdkPixmap  *pm ;  
GdkBitmap  *mask ;  
GtkImage   *image ;  
Jchart   *jc ;  
GdkGC    *gc ; 
GdkColor color ;

        jc = (Jchart  *) data ;
        image = (GtkImage *) jc->PlotArea ;
        pixmap = jc->PlotPixmap ;

   if (pixmap)
   {
      g_object_unref(pixmap);
   }
   pixmap = gdk_pixmap_new(widget->window, widget->allocation.width, 
                               widget->allocation.height,-1);
           
   fprintf(stderr,"\nexpose event called \n");


         gc =  gdk_gc_new (pixmap);
         color.red=0xFF; color.green=0x0; color.blue=0x0 ;
         gdk_gc_set_foreground (gc, &color);
         color.red=0x00; color.green=0xFF; color.blue=0x0 ;
         gdk_gc_set_background (gc, &color);

//         gc = widget->style->fg_gc[GTK_WIDGET_STATE (widget)]
         gdk_draw_arc (pixmap,gc ,
           TRUE, 0, 0, widget->allocation.width, widget->allocation.height,
           0, 64 * 360);
         gdk_draw_line(pixmap,gc,10,0,800,0);

//         gtk_image_get_pixmap(image, &pm, &mask);

         gtk_image_set_from_pixmap (image, pixmap, NULL);
         gdk_draw_drawable(widget->window,
	    widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
            pixmap, 
                    event->area.x, event->area.y,
                    event->area.x, event->area.y,
                    event->area.width, event->area.height);
  return TRUE;
}



static void jchart_init (Jchart *jc)
{
gint i,j;
GtkWidget *twid ;
GtkWidget  *image ;
GtkWidget *tab1 ;
GtkWidget *tab2 ;
GtkWidget *ebox ;
GtkWidget *vbox ;
GtkWidget *hbox ;
GtkAdjustment *tadj ;
GdkPixmap *pixmap ;
  
        jc->PlotPixmap = NULL ;

//     gtk_table_resize (GTK_TABLE (jc), 1, 1);
//     gtk_table_set_homogeneous (GTK_TABLE (jc), FALSE);
       gtk_box_set_homogeneous (GTK_BIN(jc), FALSE);

      /* tab1  vertical 2 box */
//      tab1 =  gtk_table_new (2,1,FALSE);
       fprintf(stderr,"here\n");
        vbox =  gtk_vbox_new (FALSE,0);
        gtk_widget_show (vbox);
//      gtk_table_attach_defaults (GTK_TABLE (jc), tab1, 0,1 , 0, 1);
//      gtk_table_set_homogeneous (GTK_TABLE (tab1), FALSE);
       gtk_container_add(GTK_CONTAINER(jc), GTK_WIDGET(vbox));
//      gtk_box_pack_start_defaults(tab1, twid);


      /* tab2  horizontal 3 box */
//      tab2 =  gtk_table_new (1,3,FALSE);
//      gtk_table_attach_defaults (GTK_TABLE (tab1), tab2, 0,1 , 0, 1);
//      gtk_table_set_homogeneous (GTK_TABLE (tab2), FALSE);

        hbox =  gtk_hbox_new (FALSE,0);
        gtk_box_pack_start  ( GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
        gtk_widget_show (hbox);

/*left window */
        twid  = gtk_check_button_new_with_label("Chan1");
        gtk_widget_show (twid);
        gtk_box_pack_start  ( GTK_BOX(hbox), twid, TRUE, TRUE, 0);
      //gtk_table_attach_defaults (GTK_TABLE (tab2), twid, 0,1 , 0, 1);

/* drawing window */
       ebox = gtk_event_box_new();
       gtk_widget_show (ebox);
       jc->PlotBox = ebox ;
       image = gtk_image_new ();
       jc->PlotArea = image ;

          gtk_container_add (GTK_CONTAINER (ebox), image);

          g_signal_connect (G_OBJECT (ebox), 
                      "expose_event",
                      G_CALLBACK (plotarea_expose_event), (gpointer) jc);

          g_signal_connect (G_OBJECT(ebox),"configure_event",
                      G_CALLBACK (plotarea_configure_event), (gpointer)jc);

          gtk_box_pack_start  ( GTK_BOX(hbox), ebox, TRUE, TRUE, 0);

//       gtk_widget_set_size_request (twid, 100, 100);

       //   gtk_table_attach_defaults (GTK_TABLE (tab2), ebox, 1,2 , 0, 1);
       // gtk_table_attach (GTK_TABLE (tab2), ebox, 1,2 , 0, 1, GTK_FILL, GTK_EXPAND | GTK_FILL,0,0);




/* adjustment  */
//      tadj =  gtk_adjustment_new(1.0,-1.0, 1.0, 0.1, 0.5, 0.5);
       twid  = gtk_vscale_new_with_range( -1.0, 1.0, 0.1);
       gtk_widget_show (twid);
       jc->PlotZoom = twid ;
       gtk_box_pack_end  ( GTK_BOX(hbox), twid, TRUE, TRUE, 0);
       //gtk_table_attach_defaults (GTK_TABLE (tab2), twid, 2,3 , 0, 1);
//      gtk_table_attach(GTK_TABLE (tab1),twid,2,3,0,1, GTK_EXPAND, GTK_FILL,1,1);


/* channels */
/* bottom window */
      twid  = gtk_check_button_new_with_label("Chan0");
      gtk_widget_show (twid);
      jc->CbutChan[0] = twid ;
      //gtk_table_attach_defaults (GTK_TABLE (tab1), twid, 0,1 , 1, 2);
//      gtk_table_attach(GTK_TABLE (tab1),twid,0,1,1,2, GTK_SHRINK, GTK_SHRINK,0,0);
      g_signal_connect (G_OBJECT ( twid), "clicked", G_CALLBACK (jchart_chan_clicked), (gpointer) jc);
//       gtk_widget_set_size_request (twid, 600, 200);
       gtk_box_pack_end  ( GTK_BOX(vbox), twid, TRUE, TRUE, 0);

       //gtk_widget_show (tab1);
       //gtk_widget_show (tab2);
//       gtk_widget_draw(GTK_WIDGET(jc),NULL);
}




GtkWidget* jchart_new ()
{
  return GTK_WIDGET (g_object_new (jchart_get_type (), NULL));
}

