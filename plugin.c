#include <libgimp/gimp.h>

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

static void
  radial_distortion(GimpDrawable *drawable)
{
  gint         i, j, k, channels;
  gint         x1, y1, x2, y2, center_x, center_y;
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

  center_x = (x1 + x2) / 2;
  center_y = (y1 + y2) / 2;

  float a = 2, b = 2;
  
  for (i = x1; i < x2; i++)
  {
    for (j = y1; j < y2; j++)
    {
      float dx = i - center_x;
      float dy = j - center_y;
      float radius = sqrtf(dx*dx + dy*dy);
      float distortion_radius = 1 + a*radius*radius + b*radius*radius*radius*radius;
      float normalized_dx = dx / radius;
      float normalized_dy = dy / radius;
      float distortion_dx = normalized_dx * distortion_radius;
      float distortion_dy = normalized_dy * distortion_radius;
      float distortion_x = center_x + distortion_dx;
      float distortion_y = center_y + distortion_dy;
      
      gint left_top_ngb_x = (gint)distortion_x;
      gint left_top_ngb_y = (gint)distortion_y;

      float x_offset = distortion_x - (float)left_top_ngb_x;
      float y_offset = distortion_y - (float)left_top_ngb_y;

      // get pixels around new radius
      guchar pixel[4][4];

      gimp_pixel_rgn_get_pixel(&rgn_in,
	pixel[0],
	MAX(x1, MIN(left_top_ngb_x, x2)),
	MAX(y1, MIN(left_top_ngb_y, y2)));

      gimp_pixel_rgn_get_pixel(&rgn_in,
	pixel[1],
	MAX(x1, MIN(left_top_ngb_x + 1, x2)),
	MAX(y1, MIN(left_top_ngb_y, y2)));

      gimp_pixel_rgn_get_pixel(&rgn_in,
	pixel[2],
	MAX(x1, MIN(left_top_ngb_x, x2)),
	MAX(y1, MIN(left_top_ngb_y + 1, y2)));

      gimp_pixel_rgn_get_pixel(&rgn_in,
	pixel[3],
	MAX(x1, MIN(left_top_ngb_x + 1, x2)),
	MAX(y1, MIN(left_top_ngb_y + 1, y2)));

      for (k = 0; k < channels; k++)
      {
	float horizontal_interpolation_top = pixel[0][k] * x_offset + pixel[1][k] * (1.0f - x_offset);
	float horizontal_interpolation_bottom = pixel[2][k] * x_offset + pixel[3][k] * (1.0f - x_offset);
	float vertical_interpolation = horizontal_interpolation_top * y_offset + horizontal_interpolation_bottom * (1.0f - y_offset);
	output[k] = (guchar)vertical_interpolation;
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

  radial_distortion(drawable);

  /*   g_print ("blur() took %g seconds.\n", g_timer_elapsed (timer));
   *   g_timer_destroy (timer);
   */

  gimp_displays_flush ();
  gimp_drawable_detach (drawable);
}

