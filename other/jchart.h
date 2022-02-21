#ifndef __JCHART_H__
#define __JCHART_H__


#include <glib.h>
#include <glib-object.h>
#include <gtk/gtktable.h>


G_BEGIN_DECLS

#define JCHART_TYPE            (jchart_get_type ())
#define JCHART(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), JCHART_TYPE, Jchart))
#define JCHART_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), JCHART_TYPE, JchartClass))
#define IS_JCHART(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), JCHART_TYPE))
#define IS_JCHART_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), JCHART_TYPE))


typedef struct _Jchart       Jchart;
typedef struct _JchartClass  JchartClass;

#define MAXCHAN  64

struct _Jchart
{
  gint nchan  ;
  GtkWidget       *CbutChan[MAXCHAN];
  GtkWidget       *PlotArea;
  GtkWidget       *PlotBox;
  GtkWidget       *PlotZoom;
  GdkPixmap       *PlotPixmap ;
};

struct _JchartClass
{
  GtkContainerClass   parent_class;
//  GtkTableClass parent_class;

  void (* jchart) (Jchart *jc);
};

GType          jchart_get_type        (void);
GtkWidget*     jchart_new             (void);
void	       jchart_clear           (Jchart *jc);

G_END_DECLS

#endif /* __JCHART_H__ */

