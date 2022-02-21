GTK+ 2.0 Tutorial
Prev <a2769.html>		Next <x2917.html>

------------------------------------------------------------------------


  Appendix C. Code Examples

Below are the code examples that are used in the above text which are
not included in complete form elsewhere.


  C.1. Tictactoe


    C.1.1. tictactoe.h

/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __TICTACTOE_H__
#define __TICTACTOE_H__


#include <glib.h>
#include <glib-object.h>
#include <gtk/gtktable.h>


G_BEGIN_DECLS

#define TICTACTOE_TYPE            (tictactoe_get_type ())
#define TICTACTOE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TICTACTOE_TYPE, Tictactoe))
#define TICTACTOE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TICTACTOE_TYPE, TictactoeClass))
#define IS_TICTACTOE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TICTACTOE_TYPE))
#define IS_TICTACTOE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TICTACTOE_TYPE))


typedef struct _Tictactoe       Tictactoe;
typedef struct _TictactoeClass  TictactoeClass;

struct _Tictactoe
{
  GtkTable table;
  
  GtkWidget *buttons[3][3];
};

struct _TictactoeClass
{
  GtkTableClass parent_class;

  void (* tictactoe) (Tictactoe *ttt);
};

GType          tictactoe_get_type        (void);
GtkWidget*     tictactoe_new             (void);
void	       tictactoe_clear           (Tictactoe *ttt);

G_END_DECLS

#endif /* __TICTACTOE_H__ */


    C.1.2. tictactoe.c


/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <gtk/gtksignal.h>
#include <gtk/gtktable.h>
#include <gtk/gtktogglebutton.h>
#include "tictactoe.h"

enum {
  TICTACTOE_SIGNAL,
  LAST_SIGNAL
};

static void tictactoe_class_init          (TictactoeClass *klass);
static void tictactoe_init                (Tictactoe      *ttt);
static void tictactoe_toggle              (GtkWidget *widget, Tictactoe *ttt);

static guint tictactoe_signals[LAST_SIGNAL] = { 0 };

GType
tictactoe_get_type (void)
{
  static GType ttt_type = 0;

  if (!ttt_type)
    {
      static const GTypeInfo ttt_info =
      {
	sizeof (TictactoeClass),
	NULL, /* base_init */
        NULL, /* base_finalize */
	(GClassInitFunc) tictactoe_class_init,
        NULL, /* class_finalize */
	NULL, /* class_data */
        sizeof (Tictactoe),
	0,
	(GInstanceInitFunc) tictactoe_init,
      };

      ttt_type = g_type_register_static (GTK_TYPE_TABLE, "Tictactoe", &ttt_info, 0);
    }

  return ttt_type;
}

static void
tictactoe_class_init (TictactoeClass *klass)
{
  
  tictactoe_signals[TICTACTOE_SIGNAL] = g_signal_new ("tictactoe",
					 G_TYPE_FROM_CLASS (klass),
	                                 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
	                                 G_STRUCT_OFFSET (TictactoeClass, tictactoe),
                                         NULL, 
                                         NULL,                
					 g_cclosure_marshal_VOID__VOID,
                                         G_TYPE_NONE, 0);


}

static void
tictactoe_init (Tictactoe *ttt)
{
  gint i,j;
  
  gtk_table_resize (GTK_TABLE (ttt), 3, 3);
  gtk_table_set_homogeneous (GTK_TABLE (ttt), TRUE);

  for (i=0;i<3; i++)
    for (j=0;j<3; j++)      {
	ttt->buttons[i][j] = gtk_toggle_button_new ();
	gtk_table_attach_defaults (GTK_TABLE (ttt), ttt->buttons[i][j], 
				   i, i+1, j, j+1);
	g_signal_connect (G_OBJECT (ttt->buttons[i][j]), "toggled",
			  G_CALLBACK (tictactoe_toggle), (gpointer) ttt);
	gtk_widget_set_size_request (ttt->buttons[i][j], 20, 20);
	gtk_widget_show (ttt->buttons[i][j]);
      }
}

GtkWidget*
tictactoe_new ()
{
  return GTK_WIDGET (g_object_new (tictactoe_get_type (), NULL));
}

void	       
tictactoe_clear (Tictactoe *ttt)
{
  int i,j;

  for (i = 0; i<3; i++)
    for (j = 0; j<3; j++)
      {
	g_signal_handlers_block_matched (G_OBJECT (ttt->buttons[i][j]), 
                                         G_SIGNAL_MATCH_DATA,
                                         0, 0, NULL, NULL, ttt);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ttt->buttons[i][j]),
				      FALSE);
	g_signal_handlers_unblock_matched (G_OBJECT (ttt->buttons[i][j]),
                                           G_SIGNAL_MATCH_DATA,
                                           0, 0, NULL, NULL, ttt);
      }
}

static void
tictactoe_toggle (GtkWidget *widget, Tictactoe *ttt)
{
  int i,k;

  static int rwins[8][3] = { { 0, 0, 0 }, { 1, 1, 1 }, { 2, 2, 2 },
			     { 0, 1, 2 }, { 0, 1, 2 }, { 0, 1, 2 },
			     { 0, 1, 2 }, { 0, 1, 2 } };
  static int cwins[8][3] = { { 0, 1, 2 }, { 0, 1, 2 }, { 0, 1, 2 },
			     { 0, 0, 0 }, { 1, 1, 1 }, { 2, 2, 2 },
			     { 0, 1, 2 }, { 2, 1, 0 } };

  int success, found;

  for (k = 0; k<8; k++)
    {
      success = TRUE;
      found = FALSE;

      for (i = 0; i<3; i++)
	{
	  success = success && 
	    GTK_TOGGLE_BUTTON (ttt->buttons[rwins[k][i]][cwins[k][i]])->active;
	  found = found ||
	    ttt->buttons[rwins[k][i]][cwins[k][i]] == widget;
	}
      
      if (success && found)
	{
	  g_signal_emit (G_OBJECT (ttt), 
	                 tictactoe_signals[TICTACTOE_SIGNAL], 0);
	  break;
	}
    }
}


    C.1.3. ttt_test.c


#include <stdlib.h>
#include <gtk/gtk.h>
#include "tictactoe.h"

void win( GtkWidget *widget,
          gpointer   data )
{
  g_print ("Yay!\n");
  tictactoe_clear (TICTACTOE (widget));
}

int main( int   argc,
          char *argv[] )
{
  GtkWidget *window;
  GtkWidget *ttt;
  
  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  
  gtk_window_set_title (GTK_WINDOW (window), "Aspect Frame");
  
  g_signal_connect (G_OBJECT (window), "destroy",
		    G_CALLBACK (exit), NULL);
  
  gtk_container_set_border_width (GTK_CONTAINER (window), 10);

  ttt = tictactoe_new ();
  
  gtk_container_add (GTK_CONTAINER (window), ttt);
  gtk_widget_show (ttt);

  /* And attach to its "tictactoe" signal */
  g_signal_connect (G_OBJECT (ttt), "tictactoe",
		    G_CALLBACK (win), NULL);

  gtk_widget_show (window);
  
  gtk_main ();
  
  return 0;
}

------------------------------------------------------------------------
Prev <a2769.html>	Home <index.html>	Next <x2917.html>
GDK Event Types	 	GtkDial

