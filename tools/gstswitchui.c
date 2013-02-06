/* GstSwitch
 * Copyright (C) 2012,2013 Duzy Chan <code@duzy.info>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include "gstswitchui.h"
#include "gstswitchcontroller.h"
#include "gstvideodisp.h"
#include "gstaudiovisual.h"
#include "gstcase.h"

#if 3 <= GTK_MAJOR_VERSION && GTK_MINOR_VERSION <= 4
#define CUSTOM_FRAME_DRAWING 1
#define gst_switch_ui_update(w) (gtk_widget_queue_draw (w))
#else
#define CUSTOM_FRAME_DRAWING 0
#define gst_switch_ui_update(w) ((void) FALSE)
#endif

#define GST_SWITCH_UI_LOCK_AUDIO(ui) (g_mutex_lock (&(ui)->audio_lock))
#define GST_SWITCH_UI_UNLOCK_AUDIO(ui) (g_mutex_unlock (&(ui)->audio_lock))
#define GST_SWITCH_UI_LOCK_COMPOSE(ui) (g_mutex_lock (&(ui)->compose_lock))
#define GST_SWITCH_UI_UNLOCK_COMPOSE(ui) (g_mutex_unlock (&(ui)->compose_lock))
#define GST_SWITCH_UI_LOCK_SELECT(ui) (g_mutex_lock (&(ui)->select_lock))
#define GST_SWITCH_UI_UNLOCK_SELECT(ui) (g_mutex_unlock (&(ui)->select_lock))

G_DEFINE_TYPE (GstSwitchUI, gst_switch_ui, GST_TYPE_SWITCH_CLIENT);

gboolean verbose;

static GOptionEntry entries[] = {
  {"verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose, "Be verbose", NULL},
  {NULL}
};

static const gchar * gst_switch_ui_css = 
  ".compose {\n"
  "  border-style: solid;\n"
  "  border-width: 5px;\n"
  "  border-radius: 5px;\n"
  "  border-color: rgba(0,0,0,0.2);\n"
  "  padding: 0px;\n"
  "}\n"
  ".preview_frame {\n"
  "  border-style: solid;\n"
  "  border-width: 5px;\n"
  "  border-radius: 5px;\n"
  "  border-color: rgba(0,0,0,0.2);\n"
  "  padding: 0px;\n"
  "}\n"
  ".preview_frame:selected {\n"
  "  border-color: rgba(25,25,200,0.75);\n"
  "}\n"
  ".active_audio_frame {\n"
  "  border-color: rgba(225,25,55,0.75);\n"
  "}\n"
  ".active_video_frame {\n"
  "  border-color: rgba(225,25,55,0.75);\n"
  "}\n"
  ;

static void
gst_switch_ui_parse_args (int *argc, char **argv[])
{
  GOptionContext *context;
  GError *error = NULL;
  context = g_option_context_new ("");
  g_option_context_add_main_entries (context, entries, "gst-switch");
  g_option_context_add_group (context, gst_init_get_option_group ());
  if (!g_option_context_parse (context, argc, argv, &error)) {
    g_print ("option parsing failed: %s\n", error->message);
    exit (1);
  }
  g_option_context_free (context);
}

static void
gst_switch_ui_quit (GstSwitchUI * ui)
{
  gtk_main_quit ();
}

static void
gst_switch_ui_window_closed (GtkWidget * widget, GdkEvent * event,
    gpointer data)
{
  GstSwitchUI * ui = GST_SWITCH_UI (data);
  gst_switch_ui_quit (ui);
}

/*
static gboolean
gst_switch_ui_compose_view_expose (GtkWidget * widget, GdkEventExpose * event,
    gpointer data)
{
  GstSwitchUI * ui = GST_SWITCH_UI (data);
  (void) ui;
  return FALSE;
}
*/

static gboolean
gst_switch_ui_compose_view_motion (GtkWidget * widget, GdkEventMotion * event,
    gpointer data)
{
  GstSwitchUI * ui = GST_SWITCH_UI (data);
  (void) ui;
  return FALSE;
}

static gboolean
gst_switch_ui_compose_view_press (GtkWidget * widget, GdkEventButton * event,
    gpointer data)
{
  GstSwitchUI * ui = GST_SWITCH_UI (data);
  (void) ui;
  return FALSE;
}

#if CUSTOM_FRAME_DRAWING
static gboolean
gst_switch_ui_draw_preview_frame (GtkWidget *widget, cairo_t *cr,
    GstSwitchUI *ui)
{
  GtkStyleContext *style;
  gpointer data= NULL;
  double x = 0.5, y = 0.5, width, height, radius, degrees;
  style = gtk_widget_get_style_context (widget);
  width = (double) gtk_widget_get_allocated_width (widget) - 0.5;
  height = (double) gtk_widget_get_allocated_height (widget) - 0.5;
  degrees = 3.14159265359 / 180.0;
  radius = 10.0;

  data = g_object_get_data (G_OBJECT (widget), "audio-visual");
  if (!data) 
    data = g_object_get_data (G_OBJECT (widget), "video-display");

#if 0
  gtk_style_context_save (style);
  gtk_style_context_add_class (style, "red");
  gtk_render_frame (style, cr, 0, 0, width, height);
  gtk_style_context_remove_class (style, "red");
  gtk_style_context_restore (style);
#else
  cairo_new_sub_path (cr);
  cairo_arc (cr, x + width - radius, y + radius, radius, -90 * degrees, 0 * degrees);
  cairo_arc (cr, x + width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees);
  cairo_arc (cr, x + radius, y + height - radius, radius, 90 * degrees, 180 * degrees);
  cairo_arc (cr, x + radius, y + radius, radius, 180 * degrees, 270 * degrees);
  cairo_close_path (cr);
  if (gtk_widget_get_state_flags (widget) & GTK_STATE_FLAG_SELECTED) {
    cairo_set_source_rgba (cr, 0.25, 0.25, 0.85, 0.85);
  } else if (data && GST_IS_VIDEO_DISP (data)) {
    if (gtk_style_context_has_class (style, "active_video_frame")) {
      cairo_set_source_rgba (cr, 0.85, 0.25, 0.25, 0.85);
    } else goto default_color;
  } else if (data && GST_IS_AUDIO_VISUAL (data)) {
    if (gtk_style_context_has_class (style, "active_audio_frame")) {
      cairo_set_source_rgba (cr, 0.85, 0.25, 0.25, 0.85);
    } else goto default_color;
  } else {
  default_color:
    cairo_set_source_rgba (cr, 0.25, 0.25, 0.25, 0.25);
  }
  cairo_set_line_width (cr, 10.0);
  cairo_stroke (cr);
#endif
  return TRUE;
}
#endif

static gboolean gst_switch_ui_key_event (GtkWidget *, GdkEvent *, GstSwitchUI *);
static gboolean gst_switch_ui_compose_key_event (GtkWidget *, GdkEvent *, GstSwitchUI *);

static void
gst_switch_ui_init (GstSwitchUI * ui)
{
  GtkWidget *main_box, *right_box;
  GtkWidget *scrollwin;
  GtkWidget *overlay;
  GtkStyleContext *style;
  GdkDisplay *display;
  GdkScreen *screen;
  GError *error = NULL;

  g_mutex_init (&ui->audio_lock);
  g_mutex_init (&ui->compose_lock);
  g_mutex_init (&ui->select_lock);

  display = gdk_display_get_default ();
  screen = gdk_display_get_default_screen (display);

  ui->css = gtk_css_provider_new ();
  gtk_style_context_add_provider_for_screen (screen,
      GTK_STYLE_PROVIDER (ui->css),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  ui->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW (ui->window), 640, 480);
  gtk_window_set_title (GTK_WINDOW (ui->window), "GstSwitch");

  g_signal_connect (G_OBJECT (ui->window), "key-press-event",
      G_CALLBACK (gst_switch_ui_key_event), ui);

  main_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
  right_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
  //gtk_widget_set_hexpand (right_box, TRUE);
  //gtk_widget_set_vexpand (right_box, TRUE);

  ui->preview_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
  gtk_widget_set_name (ui->preview_box, "previews");
  gtk_widget_set_size_request (ui->preview_box, 120, -1);
  gtk_widget_set_vexpand (ui->preview_box, TRUE);

  overlay = gtk_overlay_new ();
  gtk_widget_set_hexpand (overlay, TRUE);
  gtk_widget_set_vexpand (overlay, TRUE);

  ui->compose_view = gtk_drawing_area_new ();
  gtk_widget_set_name (ui->compose_view, "compose");
  gtk_widget_set_double_buffered (ui->compose_view, FALSE);
  gtk_widget_set_hexpand (ui->compose_view, TRUE);
  gtk_widget_set_vexpand (ui->compose_view, TRUE);
  gtk_widget_set_events (ui->compose_view, GDK_EXPOSURE_MASK
      | GDK_LEAVE_NOTIFY_MASK
      | GDK_BUTTON_PRESS_MASK
      | GDK_BUTTON_RELEASE_MASK
      | GDK_POINTER_MOTION_MASK
      | GDK_POINTER_MOTION_HINT_MASK
      );

  g_signal_connect (G_OBJECT (ui->compose_view), "key-press-event",
      G_CALLBACK (gst_switch_ui_compose_key_event), ui);

  ui->compose_overlay = gtk_fixed_new ();
  style = gtk_widget_get_style_context (ui->compose_overlay);
  gtk_style_context_add_class (style, "compose");
  gtk_widget_set_halign (ui->compose_overlay, GTK_ALIGN_START);
  gtk_widget_set_valign (ui->compose_overlay, GTK_ALIGN_START);
  /*
  gtk_widget_set_events (ui->compose_overlay, GDK_EXPOSURE_MASK
      | GDK_LEAVE_NOTIFY_MASK
      | GDK_BUTTON_PRESS_MASK
      | GDK_BUTTON_RELEASE_MASK
      | GDK_POINTER_MOTION_MASK
      | GDK_POINTER_MOTION_HINT_MASK
      );
  */

  /*
  GtkWidget *w = gtk_drawing_area_new ();
  //GtkWidget *w = gtk_event_box_new ();
  gtk_widget_set_size_request (w, 120, 80);
  gtk_fixed_put (GTK_FIXED (ui->compose_overlay), w, 10, 10);
  */

  ui->status = gtk_label_new (NULL);
  gtk_widget_set_hexpand (ui->status, TRUE);
  gtk_misc_set_alignment (GTK_MISC (ui->status), 1.0, 0);

  scrollwin = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollwin),
      GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_size_request (scrollwin, 130, -1);
  gtk_widget_set_vexpand (scrollwin, TRUE);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrollwin),
      ui->preview_box);

  gtk_container_add (GTK_CONTAINER (overlay), ui->compose_view);
  gtk_overlay_add_overlay (GTK_OVERLAY (overlay), ui->compose_overlay);

  gtk_box_pack_start (GTK_BOX (right_box), overlay, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (right_box), ui->status, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (main_box), scrollwin, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (main_box), right_box, TRUE, TRUE, 0);
  gtk_container_add (GTK_CONTAINER (ui->window), main_box);
  gtk_container_set_border_width (GTK_CONTAINER (ui->window), 5);

  g_signal_connect (G_OBJECT (ui->window), "delete-event",
      G_CALLBACK (gst_switch_ui_window_closed), ui);

  /*
  g_signal_connect (G_OBJECT (ui->compose_view), "expose-event",
      G_CALLBACK (gst_switch_ui_compose_view_expose), ui);
  */
  g_signal_connect (G_OBJECT (ui->compose_view), "motion-notify-event",
      G_CALLBACK (gst_switch_ui_compose_view_motion), ui);
  g_signal_connect (G_OBJECT (ui->compose_view), "button-press-event",
      G_CALLBACK (gst_switch_ui_compose_view_press), ui);

  gtk_css_provider_load_from_data (ui->css, gst_switch_ui_css, -1, &error);
  g_assert_no_error (error);
}

static void
gst_switch_ui_finalize (GstSwitchUI * ui)
{
  gtk_widget_destroy (GTK_WIDGET (ui->window));
  ui->window = NULL;

  g_object_unref (ui->css);

  g_mutex_clear (&ui->audio_lock);
  g_mutex_clear (&ui->compose_lock);
  g_mutex_clear (&ui->select_lock);

  if (G_OBJECT_CLASS (gst_switch_ui_parent_class)->finalize)
    (*G_OBJECT_CLASS (gst_switch_ui_parent_class)->finalize) (G_OBJECT (ui));
}

static void
gst_switch_ui_on_controller_closed (GstSwitchUI * ui, GError *error)
{
#if 0
  GtkWidget * msg = gtk_message_dialog_new (GTK_WINDOW (ui->window),
      GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
      "Switch server closed.\n"
      "%s", error->message);
  gtk_widget_show_now (msg);
#endif
  gtk_main_quit ();
}

static void gst_switch_ui_set_audio_port (GstSwitchUI *, gint);
static void gst_switch_ui_set_compose_port (GstSwitchUI *, gint);
static void gst_switch_ui_add_preview_port (GstSwitchUI *, gint, gint, gint);

static void
gst_switch_ui_prepare_videos (GstSwitchUI * ui)
{
  GVariant *preview_ports;
  gsize n, num_previews = 0;
  gint port;

  port = gst_switch_client_get_compose_port (GST_SWITCH_CLIENT (ui));
  gst_switch_ui_set_compose_port (ui, port);

  port = gst_switch_client_get_audio_port (GST_SWITCH_CLIENT (ui));
  gst_switch_ui_set_audio_port (ui, port);

  port = gst_switch_client_get_encode_port (GST_SWITCH_CLIENT (ui));
  INFO ("Encoded output port: %d", port);

  preview_ports = gst_switch_client_get_preview_ports (GST_SWITCH_CLIENT (ui));
  if (preview_ports) {
    GVariant *ports = NULL;
    GError *error = NULL;
    gchar *s = NULL;
    gint serve, type;

    g_variant_get (preview_ports, "(&s)", &s);
    ports = g_variant_parse (G_VARIANT_TYPE ("a(iii)"), s, NULL, NULL, &error);

    num_previews = g_variant_n_children (ports);
    for (n = 0; n < num_previews; ++n) {
      g_variant_get_child (ports, n, "(iii)", &port, &serve, &type);
      gst_switch_ui_add_preview_port (ui, port, serve, type);
      //INFO ("preview: %d, %d, %d", port, serve, type);
    }
  }
}

static gboolean
gst_switch_ui_tick (GstSwitchUI * ui)
{
  if (ui->audio) {
    gboolean stucked = FALSE;
    gboolean silent = FALSE;
    GstClockTime endtime, diff;
    gdouble value = 0;
    GST_SWITCH_UI_LOCK_AUDIO (ui);
    endtime = gst_audio_visual_get_endtime (ui->audio);
    diff = endtime - ui->audio_endtime;
    value = gst_audio_visual_get_value (ui->audio);
    stucked = ((GST_MSECOND * 700) <= diff);
    if (ui->audio_value == value) ui->audio_stuck_count += 1;
    else ui->audio_stuck_count = 0;
    if (!stucked && 3 < ui->audio_stuck_count) stucked = TRUE;
    if (!stucked) silent = (value <= 0.01);
    ui->audio_endtime = endtime;
    ui->audio_value = value;

    GST_SWITCH_UI_UNLOCK_AUDIO (ui);

    if (!stucked && !silent) {
      const gchar *s = g_strdup_printf ("audio: %f", ui->audio_value);
      gtk_label_set_text (GTK_LABEL (ui->status), s);
      g_free ((gpointer) s);
    }

    if (stucked) {
      const gchar *s = g_strdup_printf ("audio stucked at %ld (%f)",
	  endtime/GST_MSECOND, ui->audio_value);
      gtk_label_set_text (GTK_LABEL (ui->status), s);
      g_free ((gpointer) s);
    }

    if (silent) {
      const gchar *s = g_strdup_printf ("audio silent at %ld (%f)",
	  endtime/GST_MSECOND, ui->audio_value);
      gtk_label_set_text (GTK_LABEL (ui->status), s);
      g_free ((gpointer) s);
    }

    //INFO ("audio: %f", ui->audio_value);
  }
  return TRUE;
}

static void
gst_switch_ui_run (GstSwitchUI * ui)
{
  gst_switch_client_connect (GST_SWITCH_CLIENT (ui));

  ui->timer = g_timeout_add (200, (GSourceFunc) gst_switch_ui_tick, ui);

  gtk_widget_show_all (ui->window);
  gtk_widget_realize (ui->window);
  gst_switch_ui_prepare_videos (ui);
  gtk_main ();
}

static GstVideoDisp *
gst_switch_ui_new_video_disp (GstSwitchUI *ui, GtkWidget *view, gint port)
{
  gchar *name = g_strdup_printf ("video-%d", port);
  GdkWindow *xview = gtk_widget_get_window (view);
  GstVideoDisp *disp = GST_VIDEO_DISP (g_object_new (GST_TYPE_VIDEO_DISP,
	  "name", name, "port", port,
	  "handle", (gulong) GDK_WINDOW_XID (xview),
	  NULL));
  g_free (name);
  g_object_set_data (G_OBJECT (view), "video-display", disp);
  if (!gst_worker_start (GST_WORKER (disp)))
    ERROR ("failed to start video display");
  return disp;
}

static GstAudioVisual *
gst_switch_ui_new_audio_visual_unsafe (GstSwitchUI *ui, GtkWidget *view,
    gulong handle, gint port)
{
  gchar *name = g_strdup_printf ("visual-%d", port);
  GstAudioVisual *visual;

  if (view && handle == 0) {
    GdkWindow *xview = gtk_widget_get_window (view);
    handle = GDK_WINDOW_XID (xview);
  }

  visual = GST_AUDIO_VISUAL (
      g_object_new (GST_TYPE_AUDIO_VISUAL, "name", name, "port", port,
	  "handle", handle, "active", (ui->audio_port == port), NULL));
  g_free (name);
  if (!gst_worker_start (GST_WORKER (visual)))
    ERROR ("failed to start audio visual");
  return visual;
}

static GstAudioVisual *
gst_switch_ui_new_audio_visual (GstSwitchUI *ui, GtkWidget *view, gint port)
{
  GstAudioVisual *visual = NULL;
  GST_SWITCH_UI_LOCK_AUDIO (ui);
  visual = gst_switch_ui_new_audio_visual_unsafe (ui, view, 0, port);
  GST_SWITCH_UI_UNLOCK_AUDIO (ui);
  return visual;
}

static void
gst_switch_ui_remove_preview (GstSwitchUI *ui, GstWorker *worker,
    const gchar *name)
{
  GList *v = NULL;
  GST_SWITCH_UI_LOCK_SELECT (ui);
  v = gtk_container_get_children (GTK_CONTAINER (ui->preview_box));
  for (; v; v = g_list_next (v)) {
    GtkWidget *frame = GTK_WIDGET (v->data);
    gpointer data = g_object_get_data (G_OBJECT (frame), name);
    if (data && GST_IS_WORKER (data) && GST_WORKER (data) == worker) {
      if (ui->selected == frame) {
	GList *nxt = g_list_next (v);
	if (nxt == NULL) nxt = g_list_previous (v);
	ui->selected = nxt ? GTK_WIDGET (nxt->data) : NULL;
      }
      gtk_widget_destroy (frame);
      g_object_unref (worker);
    }
  }
  GST_SWITCH_UI_UNLOCK_SELECT (ui);
}

static void
gst_switch_ui_end_video_disp (GstWorker *worker, GstSwitchUI *ui)
{
  GstVideoDisp *disp = GST_VIDEO_DISP (worker);
  INFO ("video ended: %s, %d", worker->name, disp->port);
  gst_switch_ui_remove_preview (ui, worker, "video-display");
}

static void
gst_switch_ui_end_audio_visual (GstWorker *worker, GstSwitchUI *ui)
{
  GstAudioVisual *visual = GST_AUDIO_VISUAL (worker);
  INFO ("audio ended: %s, %d", worker->name, visual->port);
  if (visual->renewing) {
    g_object_unref (GST_WORKER (visual));
  } else {
    gst_switch_ui_remove_preview (ui, worker, "audio-visual");
  }
}

static void
gst_switch_ui_set_compose_port (GstSwitchUI *ui, gint port)
{
  GST_SWITCH_UI_LOCK_COMPOSE (ui);
  if (ui->compose) {
    gst_worker_stop (GST_WORKER (ui->compose));
    g_object_unref (ui->compose);
  }

  ui->compose = gst_switch_ui_new_video_disp (ui, ui->compose_view, port);
  GST_SWITCH_UI_UNLOCK_COMPOSE (ui);
}

static GstAudioVisual *
gst_switch_ui_renew_audio_visual (GstSwitchUI *ui, GtkWidget *frame,
    GstAudioVisual *visual)
{
  gulong handle = visual->handle;
  gint port = visual->port;

  visual->renewing = TRUE;
  gst_worker_stop (GST_WORKER (visual));

  visual = gst_switch_ui_new_audio_visual_unsafe (ui, NULL, handle, port);

  g_object_set_data (G_OBJECT (frame), "audio-visual", visual);
  return visual;
}

static void
gst_switch_ui_set_audio_port (GstSwitchUI *ui, gint port)
{
  GList *v = NULL;

  GST_SWITCH_UI_LOCK_AUDIO (ui);
  ui->audio_port = port;

  v = gtk_container_get_children (GTK_CONTAINER (ui->preview_box));
  for (; v; v = g_list_next (v)) {
    GtkWidget *frame = GTK_WIDGET (v->data);
    GtkStyleContext *style = gtk_widget_get_style_context (frame);
    gpointer data = g_object_get_data (G_OBJECT (frame), "audio-visual");
    if (data) {
      GstAudioVisual *visual = GST_AUDIO_VISUAL (data);
      if ((ui->audio_port == visual->port && !visual->active) ||
	  (/*ui->audio_port != visual->port &&*/  visual->active)) {
	gtk_style_context_remove_class (style, "active_audio_frame");
	visual = gst_switch_ui_renew_audio_visual (ui, frame, visual);
	if (visual->active) {
	  gtk_style_context_add_class (style, "active_audio_frame");
	  gst_switch_ui_update (frame);
	  ui->audio = visual;
	}
      }
    }
  }
  GST_SWITCH_UI_UNLOCK_AUDIO (ui);
}

static void
gst_switch_ui_mark_active_video (GstSwitchUI *ui, gint port, gint type)
{
  GList *item = gtk_container_get_children (
      GTK_CONTAINER (ui->preview_box));
  for (; item; item = g_list_next (item)) {
    GtkWidget *frame = GTK_WIDGET (item->data);
    gpointer data = g_object_get_data (G_OBJECT (frame), "video-display");
    if (data) {
      GtkStyleContext *style = gtk_widget_get_style_context (frame);
      GstVideoDisp *disp = GST_VIDEO_DISP (data);
      if (disp->port == port) {
	gtk_style_context_add_class (style, "active_video_frame");
      } else if (disp->type == type) {
	gtk_style_context_remove_class (style, "active_video_frame");
      }
      gst_switch_ui_update (frame);
    }
  }
}

static gboolean gst_switch_ui_switch_unsafe (GstSwitchUI *, gint);
static gboolean gst_switch_ui_switch (GstSwitchUI *, gint);

static gboolean
gst_switch_ui_preview_click (GtkWidget *w, GdkEvent *event, GstSwitchUI *ui)
{
  GstVideoDisp *disp = NULL;
  GstAudioVisual *visual = NULL;
  GtkWidget *previous = NULL;
  gint port = 0;

  switch (event->type) {
  case GDK_BUTTON_PRESS: {
    while (w) {
      disp = g_object_get_data (G_OBJECT (w), "video-display");
      visual = g_object_get_data (G_OBJECT (w), "audio-visual");
      if (disp) {
	port = disp->port;
	break;
      } else if (visual) {
	port = visual->port;
	break;
      } else {
	w = gtk_widget_get_parent (w);
      }
    }
  } break;
  default: break;
  }

  (void) port;

  if ((disp || visual) && w) {
    GdkEventButton *ev = (GdkEventButton *) event;
    GtkWidget *prevframe = NULL;
    GstVideoDisp *prevdisp = NULL;
    gint newvideotype = GST_CASE_UNKNOWN;

    GST_SWITCH_UI_LOCK_SELECT (ui);
    previous = ui->selected;
    ui->selected = w;

    switch (ev->button) {
    case 1: // left button
      newvideotype = GST_CASE_BRANCH_A;
      break;
    case 3: // right button
      if (disp) newvideotype = GST_CASE_BRANCH_B;
      break;
    }

    if (newvideotype != GST_CASE_UNKNOWN) {
      gpointer data;
      GList *item = gtk_container_get_children (
	  GTK_CONTAINER (ui->preview_box));
      for (; item; item = g_list_next (item)) {
	GtkWidget *frame = GTK_WIDGET (item->data);
	data = g_object_get_data (G_OBJECT (frame), "video-display");
	if (frame == ui->selected || data == disp) continue;
	if (!GST_IS_VIDEO_DISP (data)) continue;
	if (GST_VIDEO_DISP (data)->type == newvideotype) {
	  prevdisp = GST_VIDEO_DISP (data);
	  prevframe = frame;
	  break;
	}
      }
    }
    
    switch (ev->button) {
    case 1: // left button
      if (gst_switch_ui_switch_unsafe (ui, GDK_KEY_A)) {
	goto swap_video_type;
      }
      break;
    case 3: // right button
      if (disp && gst_switch_ui_switch_unsafe (ui, GDK_KEY_B)) {
	goto swap_video_type;
      }
      break;
    swap_video_type:
      if (disp) {
	gint t = disp->type;
	disp->type = newvideotype;
	if (prevframe && prevdisp) {
	  GtkStyleContext *style = gtk_widget_get_style_context (prevframe);
	  switch (t) {
	  case GST_CASE_BRANCH_A:
	  case GST_CASE_BRANCH_B:
	    gtk_style_context_add_class (style, "active_video_frame");
	    break;
	  case GST_CASE_BRANCH_p:
	    gtk_style_context_remove_class (style, "active_video_frame");
	    break;
	  }
	  gst_switch_ui_update (prevframe);
	  prevdisp->type = t;
	}
      }
    }
    ui->selected = previous;
    GST_SWITCH_UI_UNLOCK_SELECT (ui);
  }
  return TRUE;
}

static void
gst_switch_ui_add_preview_port (GstSwitchUI *ui, gint port, gint serve,
    gint type)
{
  GstVideoDisp *disp = NULL;
  GstAudioVisual *visual = NULL;
  GtkStyleContext *style = NULL;
  GtkWidget *frame = gtk_frame_new (NULL);
  GtkWidget *preview = gtk_drawing_area_new ();
  gtk_widget_set_double_buffered (preview, FALSE);
  gtk_widget_set_size_request (preview, -1, 80);
  gtk_widget_set_events (preview, GDK_BUTTON_RELEASE_MASK|
      GDK_BUTTON_PRESS_MASK);
  gtk_container_add (GTK_CONTAINER (frame), preview);
  gtk_container_add (GTK_CONTAINER (ui->preview_box), frame);

  style = gtk_widget_get_style_context (frame);
  gtk_style_context_add_class (style, "preview_frame");

  style = gtk_widget_get_style_context (preview);
  gtk_style_context_add_class (style, "preview_drawing_area");

  gtk_widget_show_all (frame);

#if CUSTOM_FRAME_DRAWING
  g_signal_connect (frame, "draw",
      G_CALLBACK (gst_switch_ui_draw_preview_frame), ui);
#endif
  g_signal_connect (preview, "button-press-event",
      G_CALLBACK (gst_switch_ui_preview_click), ui);

  /*
  GST_CASE_BRANCH_A,
  GST_CASE_BRANCH_B,
  GST_CASE_BRANCH_a,
  GST_CASE_BRANCH_p,
  */

  switch (serve) {
  case GST_SERVE_VIDEO_STREAM:
    disp = gst_switch_ui_new_video_disp (ui, preview, port);
    disp->type = type;
    g_object_set_data (G_OBJECT (frame), "video-display", disp);
    g_signal_connect (G_OBJECT(disp), "end-worker",
	G_CALLBACK (gst_switch_ui_end_video_disp), ui);
    switch (type) {
    case GST_CASE_BRANCH_A:
    case GST_CASE_BRANCH_B:
      style = gtk_widget_get_style_context (frame);
      gtk_style_context_add_class (style, "active_video_frame");
      gst_switch_ui_mark_active_video (ui, port, type);
      break;
    }
    gst_switch_ui_update (frame);
    break;
  case GST_SERVE_AUDIO_STREAM:
    visual = gst_switch_ui_new_audio_visual (ui, preview, port);
    g_object_set_data (G_OBJECT (frame), "audio-visual", visual);
    g_signal_connect (G_OBJECT(visual), "end-worker",
	G_CALLBACK (gst_switch_ui_end_audio_visual), ui);
    if (visual->active) {
      style = gtk_widget_get_style_context (frame);
      gtk_style_context_add_class (style, "active_audio_frame");
    }
    gst_switch_ui_update (frame);
    break;
  default:
    gtk_widget_destroy (preview);
    gtk_widget_destroy (frame);
    break;
  }
}

static void
gst_switch_ui_select_preview (GstSwitchUI *ui, guint key)
{
  GList *view = NULL, *selected = NULL;
  GtkWidget *previous = NULL;
  GST_SWITCH_UI_LOCK_SELECT (ui);
  view = gtk_container_get_children (GTK_CONTAINER (ui->preview_box));
  if (ui->selected == NULL) {
    if (view) switch (key) {
    case GDK_KEY_Up:
      selected = g_list_last (view);
      break;
    case GDK_KEY_Down:
      selected = view;
      break;
    }
  } else {
    for (; view; view = g_list_next (view)) {
      if (GTK_WIDGET (view->data) == ui->selected) {
	selected = view;
	break;
      }
    }
    if (selected) {
      previous = GTK_WIDGET (selected->data);
      switch (key) {
      case GDK_KEY_Up:
	selected = g_list_previous (selected);
	break;
      case GDK_KEY_Down:
	selected = g_list_next (selected);
	break;
      }
    }
  }

  if (selected) {
    ui->selected = GTK_WIDGET (selected->data);
  }

  if (ui->selected) {
    if (previous) {
      gtk_widget_unset_state_flags (previous, GTK_STATE_FLAG_SELECTED);
    }
    gtk_widget_set_state_flags (ui->selected, GTK_STATE_FLAG_SELECTED, TRUE);
    //INFO ("select: %p, %p", previous, ui->selected);
  }
  GST_SWITCH_UI_UNLOCK_SELECT (ui);
}

static gboolean
gst_switch_ui_switch_unsafe (GstSwitchUI *ui, gint key)
{
  gint port, type;
  gpointer data;
  gboolean ok = FALSE;
  //GST_SWITCH_UI_LOCK_SELECT (ui);
  if (!ui->selected) {
    goto end;
  }

  data = g_object_get_data (G_OBJECT (ui->selected), "video-display");
  if (data && GST_IS_VIDEO_DISP (data)) {
    port = GST_VIDEO_DISP (data)->port;
    switch (key) {
    case GDK_KEY_A:
    case GDK_KEY_a:
      type = GST_CASE_BRANCH_A;
      ok = gst_switch_client_switch (GST_SWITCH_CLIENT (ui), 'A', port);
      INFO ("switch-a: %d, %d", port, ok);
      break;
    case GDK_KEY_B:
    case GDK_KEY_b:
      type = GST_CASE_BRANCH_B;
      ok = gst_switch_client_switch (GST_SWITCH_CLIENT (ui), 'B', port);
      INFO ("switch-b: %d, %d", port, ok);
      break;
    }

    if (ok) {
      gst_switch_ui_mark_active_video (ui, port, type);
    }
    
    goto end;
  }

  data = g_object_get_data (G_OBJECT (ui->selected), "audio-visual");
  if (data && GST_IS_AUDIO_VISUAL (data)) {
    port = GST_AUDIO_VISUAL (data)->port;
    switch (key) {
    case GDK_KEY_A:
    case GDK_KEY_a:
      ok = gst_switch_client_switch (GST_SWITCH_CLIENT (ui), 'a', port);
      INFO ("switch-audio: %d, %d", port, ok);
      break;
    }
    goto end;
  }

 end:
  //GST_SWITCH_UI_UNLOCK_SELECT (ui);
  return ok;
}

static gboolean
gst_switch_ui_switch (GstSwitchUI *ui, gint key)
{
  gboolean ok = FALSE;
  GST_SWITCH_UI_LOCK_SELECT (ui);
  ok = gst_switch_ui_switch_unsafe (ui, key);
  GST_SWITCH_UI_UNLOCK_SELECT (ui);
  return ok;
}

static void
gst_switch_ui_next_compose_mode (GstSwitchUI *ui)
{
  gboolean ok = FALSE;
  if (3 < ui->compose_mode) ui->compose_mode = 0;
  ok = gst_switch_client_set_composite_mode (GST_SWITCH_CLIENT (ui),
      ui->compose_mode);
  INFO ("set composite mode: %d (%d)", ui->compose_mode, ok);
  ui->compose_mode += 1;
}

static void
gst_switch_ui_next_compose_off (GstSwitchUI *ui)
{
  gboolean ok = gst_switch_client_set_composite_mode (GST_SWITCH_CLIENT (ui),
      0);
  INFO ("set composite off (%d)", ok);
  if (ok && ui->compose_mode == 0) ui->compose_mode = 1;
}

static void
gst_switch_ui_new_record (GstSwitchUI *ui)
{
  gboolean ok = gst_switch_client_new_record (GST_SWITCH_CLIENT (ui));
  INFO ("new record: %d", ok);
}

static void
gst_switch_ui_adjust_pip (GstSwitchUI *ui, gboolean resize, gint key)
{
  const gint step = 1;
  gint dx = 0, dy = 0, dw = 0, dh = 0;
  guint result;

  if (resize) {
    switch (key) {
    case GDK_KEY_Left:  dw -= step; break;
    case GDK_KEY_Right: dw += step; break;
    case GDK_KEY_Up:    dh -= step; break;
    case GDK_KEY_Down:  dh += step; break;
    }
  } else {
    switch (key) {
    case GDK_KEY_Left:  dx -= step; break;
    case GDK_KEY_Right: dx += step; break;
    case GDK_KEY_Up:    dy -= step; break;
    case GDK_KEY_Down:  dy += step; break;
    }
  }

  result = gst_switch_client_adjust_pip (GST_SWITCH_CLIENT (ui),
      dx, dy, dw, dh);
  INFO ("adjust-pip: (%d) %d", resize, result);
}

static gboolean
gst_switch_ui_key_event (GtkWidget *w, GdkEvent *event, GstSwitchUI *ui)
{
  switch (event->type) {
  case GDK_KEY_PRESS:
  case GDK_KEY_RELEASE: {
    GdkEventKey *ke = (GdkEventKey *) event;
    gboolean mod = ke->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK);
    switch (ke->keyval) {
    case GDK_KEY_Up:
    case GDK_KEY_Down:
      if (mod) goto adjust_pip;
      gst_switch_ui_select_preview (ui, ke->keyval);
      break;
    case GDK_KEY_Left:
    case GDK_KEY_Right:
      if (mod) {
	gboolean resize;
      adjust_pip:
	resize = (ke->state & GDK_SHIFT_MASK) ? TRUE : FALSE;
	gst_switch_ui_adjust_pip (ui, resize, ke->keyval);
	return TRUE;
      }
      break;
    case GDK_KEY_A:
    case GDK_KEY_a:
    case GDK_KEY_B:
    case GDK_KEY_b:
      gst_switch_ui_switch (ui, ke->keyval);
      break;
    case GDK_KEY_Tab: {
      if (200 <= ke->time - ui->tabtime) {
	ui->tabtime = ke->time;
	gst_switch_ui_next_compose_mode (ui);
      }
    } break;
    case GDK_KEY_Escape:
      gst_switch_ui_next_compose_off (ui);
      break;
    case GDK_KEY_R:
    case GDK_KEY_r:
      gst_switch_ui_new_record (ui);
      break;
    }
  } break;
  default: break;
  }
  return FALSE;
}

static gboolean
gst_switch_ui_compose_key_event (GtkWidget *w, GdkEvent *event, GstSwitchUI *ui)
{
  switch (event->type) {
  case GDK_KEY_PRESS:
  case GDK_KEY_RELEASE: {
    GdkEventKey *ke = (GdkEventKey *) event;
    INFO ("compose-key: %d", ke->keyval);
  }
  default: break;
  }
  return FALSE;
}

static void
gst_switch_ui_class_init (GstSwitchUIClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GstSwitchClientClass * client_class = GST_SWITCH_CLIENT_CLASS (klass);

  object_class->finalize = (GObjectFinalizeFunc) gst_switch_ui_finalize;

  client_class->connection_closed = (GstSwitchClientConnectionClosedFunc)
    gst_switch_ui_on_controller_closed;
  client_class->set_compose_port = (GstSwitchClientSetComposePortFunc)
    gst_switch_ui_set_compose_port;
  client_class->set_audio_port = (GstSwitchClientSetAudioPortFunc)
    gst_switch_ui_set_audio_port;
  client_class->add_preview_port = (GstSwitchClientAddPreviewPortFunc)
    gst_switch_ui_add_preview_port;
}

int
main (int argc, char *argv[])
{
  GstSwitchUI *ui;

  gst_switch_ui_parse_args (&argc, &argv);
  gtk_init (&argc, &argv);

  ui = GST_SWITCH_UI (g_object_new (GST_TYPE_SWITCH_UI, NULL));

  gst_switch_ui_run (ui);

  g_object_unref (G_OBJECT (ui));
  return 0;
}
