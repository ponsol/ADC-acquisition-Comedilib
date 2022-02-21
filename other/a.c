static void jchart_init_old (Jchart *ttt)
{
gint i,j;
GtkWidget *twid ;
GtkWidget *tab1 ;
GtkWidget *tab2 ;
GtkWidget *ebox ;
GtkAdjustment *tadj ;
GdkPixmap *pixmap ;
  
  gtk_table_resize (GTK_TABLE (ttt), 1, 1);
  gtk_table_set_homogeneous (GTK_TABLE (ttt), TRUE);

      tab1 =  gtk_table_new (2,1,TRUE);
      gtk_table_attach_defaults (GTK_TABLE (ttt), tab1, 0,1 , 0, 1);


      tab2 =  gtk_table_new (1,3,TRUE);
      gtk_table_attach_defaults (GTK_TABLE (tab1), tab2, 0,1 , 0, 1);

/* drawing window */
      ebox = gtk_event_box_new();
      twid =  gtk_drawing_area_new ();
      gtk_box_pack_start( ebox, twid, TRUE, TRUE, 0);

      gtk_table_attach_defaults (GTK_TABLE (tab2), ebox, 1,2 , 0, 1);
      gtk_widget_show (twid);
      ttt->WidPlot = twid ;

//      gtk_table_attach_defaults (GTK_TABLE (tab2), twid, 1,2 , 0, 1);

//      pixmap = gdk_pixmap_new(twid->window,
//			  twid->allocation.width,
//			  twid->allocation.height,
//			  -1);
//   gdk_draw_rectangle (pixmap,
//		      twid->style->white_gc,
//		      TRUE,
//		      0, 0,
//		      twid->allocation.width,
//		      twid->allocation.height);

/* adjustment  */
//      tadj =  gtk_adjustment_new(1.0,-1.0, 1.0, 0.1, 0.5, 0.5);
      twid  = gtk_vscale_new_with_range( -1.0, 1.0, 0.1);

 //     twid =  gtk_vscrollbar_new(NULL);
      gtk_widget_show (twid);
      ttt->WidPlot = twid ;
      gtk_table_attach_defaults (GTK_TABLE (tab2), twid, 2,3 , 0, 1);


      twid  = gtk_check_button_new_with_label("Chan0");
      ttt->WidCbutChan[0] = twid ;
      gtk_table_attach_defaults (GTK_TABLE (tab1), twid, 0,1 , 1, 2);

       g_signal_connect (G_OBJECT ( twid), "clicked", G_CALLBACK (jchart_chan_clicked), (gpointer) ttt);
       gtk_widget_set_size_request (twid, 20, 20);
       gtk_widget_show (tab1);
       gtk_widget_show (tab2);
       gtk_widget_show (twid);

//  for (i=0;i<3; i++)
//    for (j=0;j<3; j++)      {
//	ttt->buttons[i][j] = gtk_toggle_button_new ();
//	gtk_table_attach_defaults (GTK_TABLE (ttt), ttt->buttons[i][j], 
//				   i, i+1, j, j+1);
//	g_signal_connect (G_OBJECT (ttt->buttons[i][j]), "toggled",
//			  G_CALLBACK (jchart_toggle), (gpointer) ttt);
//	gtk_widget_set_size_request (ttt->buttons[i][j], 20, 20);
//	gtk_widget_show (ttt->buttons[i][j]);
 //     }

}

