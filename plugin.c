#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

static void query (void);
static void run   (const gchar      *name,
                   gint              nparams,
                   const GimpParam  *param,
                   gint             *nreturn_vals,
                   GimpParam       **return_vals);

GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,
  NULL,
  query,
  run
};

MAIN()

static void
query (void)
{
  static GimpParamDef args[] =
  {
    {
      GIMP_PDB_INT32,
      "run-mode",
      "Run mode"
    },
    {
      GIMP_PDB_IMAGE,
      "image",
      "Input image"
    },
    {
      GIMP_PDB_DRAWABLE,
      "drawable",
      "Input drawable"
    }
  };

  gimp_install_procedure (
    "radial-distortion-ip",
    "Radial distortion.",
    "Corrects radial distortion.",
    "MP",
    "Copyright MP",
    "2013",
    "_Radial Distortion...",
    "RGB*, GRAY*",
    GIMP_PLUGIN,
    G_N_ELEMENTS (args), 0,
    args, NULL);

  gimp_plugin_menu_register ("radial-distortion-ip",
                             "<Image>/Filters/Misc");
}

float a = 0, b = 0, scale = 1000;
  

gboolean dialog (GimpDrawable *drawable)
{
   GtkWidget *dialog;
   GtkWidget *main_vbox;
   GtkWidget *main_hbox;
   GtkWidget *a_label;
   GtkWidget *a_d;
   GtkObject *a_adj_d;
   GtkWidget *b_label;
   GtkWidget *b_d;
   GtkObject *b_adj_d;
   gboolean   run;

   gimp_ui_init("Radial Distortion Dialog", FALSE);
   dialog = gimp_dialog_new("Radial Distortion Dialog", "Radial Distortion Dialog",
                            NULL, (GtkDialogFlags) 0,
                            gimp_standard_help_func, "radial-distortion-ip",
                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_OK,     GTK_RESPONSE_OK,
                            NULL);
   main_vbox = gtk_vbox_new(FALSE, 6);
   gtk_container_add(GTK_CONTAINER(GTK_DIALOG (dialog)->vbox), main_vbox);
   gtk_widget_show(main_vbox);

   {
     a_label = gtk_label_new_with_mnemonic("_a:");
     gtk_widget_show(a_label);
     gtk_box_pack_start(GTK_BOX (main_vbox), a_label, FALSE, FALSE, 6);
     gtk_label_set_justify(GTK_LABEL (a_label), GTK_JUSTIFY_RIGHT);
    
     a_d = gimp_spin_button_new(&a_adj_d, 0,
       -10, 10, 0.1, 0.1, 0, 13, 1);
     gtk_box_pack_start(GTK_BOX(main_vbox), a_d, FALSE, FALSE, 0);
     gtk_widget_show(a_d);
     g_signal_connect(a_adj_d, "value_changed",
       G_CALLBACK(gimp_float_adjustment_update),
       &a);
   }

   {
     b_label = gtk_label_new_with_mnemonic("_b:");
     gtk_widget_show(b_label);
     gtk_box_pack_start(GTK_BOX (main_vbox), b_label, FALSE, FALSE, 6);
     gtk_label_set_justify(GTK_LABEL (b_label), GTK_JUSTIFY_RIGHT);
    
     b_d = gimp_spin_button_new(&b_adj_d, 0,
       -4, 4, 0.01, 0.01, 0, 13, 2);
     gtk_box_pack_start(GTK_BOX(main_vbox), b_d, FALSE, FALSE, 0);
     gtk_widget_show(b_d);
     g_signal_connect(b_adj_d, "value_changed",
       G_CALLBACK(gimp_float_adjustment_update),
       &b);
   }
   
   {
     b_label = gtk_label_new_with_mnemonic("_scale:");
     gtk_widget_show(b_label);
     gtk_box_pack_start(GTK_BOX (main_vbox), b_label, FALSE, FALSE, 6);
     gtk_label_set_justify(GTK_LABEL (b_label), GTK_JUSTIFY_RIGHT);
    
     b_d = gimp_spin_button_new(&b_adj_d, 1000,
       -1000, 1000, 100, 100, 0, 13, 0);
     gtk_box_pack_start(GTK_BOX(main_vbox), b_d, FALSE, FALSE, 0);
     gtk_widget_show(b_d);
     g_signal_connect(b_adj_d, "value_changed",
       G_CALLBACK(gimp_float_adjustment_update),
       &scale);
   }
   gtk_widget_show(dialog);

   run = (gimp_dialog_run(GIMP_DIALOG(dialog)) == GTK_RESPONSE_OK);

   gtk_widget_destroy(dialog);
   return run;
}

static void
  radial_distortion(GimpDrawable *drawable)
{
  gint         i, j, k, channels;
  gint         x1, y1, x2, y2;
  GimpPixelRgn rgn_in, rgn_out;
  guchar       output[4];

  /* Gets upper left and lower right coordinates,
   * and layers number in the image */
  gimp_drawable_mask_bounds (drawable->drawable_id,
    &x1, &y1,
    &x2, &y2);
  channels = gimp_drawable_bpp (drawable->drawable_id);

  /* Initialises two PixelRgns, one to read original data,
   * and the other to write output data. That second one will
   * be merged at the end by the call to
   * gimp_drawable_merge_shadow() */
  gimp_pixel_rgn_init (&rgn_in,
    drawable,
    x1, y1,
    x2 - x1, y2 - y1, 
    FALSE, FALSE);
  gimp_pixel_rgn_init (&rgn_out,
    drawable,
    x1, y1,
    x2 - x1, y2 - y1, 
    TRUE, TRUE);

  float center_x = (x1 + x2) / 2;
  float center_y = (y1 + y2) / 2;

  printf("x1 = %d, x2 = %d, y1 = %d, y2 = %d\n", x1, x2, y1, y2);
  printf("a = %f, b = %f\n", a, b);
  const int bilinear = 1;

  for (i = x1; i < x2; i++)
  {
    for (j = y1; j < y2; j++)
    {
      float dx = (float)i - center_x;
      float dy = (float)j - center_y;
      float radius = sqrtf(dx*dx + dy*dy)/scale;
      float z = 1 + a*radius*radius + b*radius*radius*radius*radius;
      float distortion_x = center_x + dx*z;
      float distortion_y = center_y + dy*z;
      
      gint left_top_ngb_x = (gint)floor(distortion_x);
      gint left_top_ngb_y = (gint)floor(distortion_y);

      float x_offset = distortion_x - (float)left_top_ngb_x;
      float y_offset = distortion_y - (float)left_top_ngb_y;

      // get pixels around new radius
      guchar pixel[4][4];

      gimp_pixel_rgn_get_pixel(&rgn_in,
	pixel[0],
	MAX(x1, MIN(left_top_ngb_x, x2 - 1)),
	MAX(y1, MIN(left_top_ngb_y, y2 - 1)));

      gimp_pixel_rgn_get_pixel(&rgn_in,
	pixel[1],
	MAX(x1, MIN(left_top_ngb_x + 1, x2 - 1)),
	MAX(y1, MIN(left_top_ngb_y, y2 - 1)));

      gimp_pixel_rgn_get_pixel(&rgn_in,
	pixel[2],
	MAX(x1, MIN(left_top_ngb_x, x2 - 1)),
	MAX(y1, MIN(left_top_ngb_y + 1, y2 - 1)));

      gimp_pixel_rgn_get_pixel(&rgn_in,
	pixel[3],
	MAX(x1, MIN(left_top_ngb_x + 1, x2 - 1)),
	MAX(y1, MIN(left_top_ngb_y + 1, y2 - 1)));

      for (k = 0; k < channels; k++)
      {
	if(bilinear)
	{
	  float horizontal_interpolation_bottom = pixel[0][k] * (1.0f - x_offset) + pixel[1][k] * x_offset;
	  float horizontal_interpolation_top = pixel[2][k] * (1.0f - x_offset) + pixel[3][k] * x_offset;
	  float vertical_interpolation = horizontal_interpolation_top * y_offset + horizontal_interpolation_bottom * (1.0f - y_offset);
	  output[k] = (guchar)vertical_interpolation;
	}
	else
	{
	  output[k] = pixel[0][k];
	}
      }

      gimp_pixel_rgn_set_pixel (&rgn_out,
	output,
	i, j);
    }

    if (i % 10 == 0)
      gimp_progress_update ((gdouble) (i - x1) / (gdouble) (x2 - x1));
  }

  /*  Update the modified region */
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
  gimp_drawable_update (drawable->drawable_id,
    x1, y1,
    x2 - x1, y2 - y1);
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam  values[1];
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  GimpRunMode       run_mode;
  GimpDrawable     *drawable;


  /* Setting mandatory output values */
  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  /* Getting run_mode - we won't display a dialog if 
   * we are in NONINTERACTIVE mode */
  run_mode = param[0].data.d_int32;

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  gimp_progress_init ("Creating radial distortion...");

  /* Let's time blur
   *
   *   GTimer timer = g_timer_new time ();
   */

  if (run_mode == GIMP_RUN_INTERACTIVE) {
    dialog(drawable);
  }
  
  radial_distortion(drawable);

  /*   g_print ("blur() took %g seconds.\n", g_timer_elapsed (timer));
   *   g_timer_destroy (timer);
   */

  gimp_displays_flush ();
  gimp_drawable_detach (drawable);
}

