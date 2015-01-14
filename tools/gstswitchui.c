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

/*! @file */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include "gstswitchui.h"
#include "gstswitchopts.c"
#include "gstswitchclient.h"
#include "logutils.h"


static void gst_switch_ui_parse_args (int *argc, char **argv[], GstSwitchUI * ui);
static void gst_switch_ui_free_args (GstSwitchUI * ui);
static void gst_switch_ui_class_init (GstSwitchUIClass * klass);
static void gst_switch_ui_init (GstSwitchUI * ui);
static void gst_switch_ui_setup (GstSwitchUI * ui);
static void gst_switch_ui_run (GstSwitchUI * ui);

G_DEFINE_TYPE (GstSwitchUI, gst_switch_ui, GST_TYPE_SWITCH_CLIENT);

gboolean verbose;

/**
 * @brief Main entry-point of gst-switch-ui.
 */
int
main (int argc, char *argv[])
{
  /* Pointer to the UI Object */
  GstSwitchUI *ui;

  /* Init GTK Runtime */
  gtk_init (&argc, &argv);

  /* Creat an Instance of the UI Object (calls gst_switch_ui_init) */
  ui = GST_SWITCH_UI (g_object_new (GST_TYPE_SWITCH_UI, NULL));

  /* Parse Command-Line Args into properties of the UI Object */
  gst_switch_ui_parse_args (&argc, &argv, ui);

  /* Setup the UI Elements from the Glade-File, honoring the Command-Line Args */
  gst_switch_ui_setup (ui);

  /* Run the UI Loop */
  gst_switch_ui_run (ui);

  /* Free reserved Args from the UI Object's properties */
  gst_switch_ui_free_args(ui);

  /* Delete the UI Object */
  g_object_unref (G_OBJECT (ui));

  /* Cya */
  return 0;
}


/**
 * @brief Parse command line arguments.
 * Stores them in their respective properties of GstSwitchUI * ui
 */
static void
gst_switch_ui_parse_args (int *argc, char **argv[], GstSwitchUI * ui)
{
  INFO("gst_switch_ui_parse_args");

  GOptionContext *context;
  GError *error = NULL;
  context = g_option_context_new ("");

  GOptionEntry entries[] = {
    {"verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose, "Be verbose.", NULL},
    {"dissalow-quit", 'Q', 0, G_OPTION_ARG_NONE, &ui->disallow_quit, "Don't allow the user to quit the Application from the GUI.", NULL},
    {"ui-file", 'u', 0, G_OPTION_ARG_FILENAME, &ui->uifile, "Supply a custom .glade-File to build the GUI from.", NULL},
    {NULL}
  };

  g_option_context_add_main_entries (context, entries, "gst-switch-ui");
  g_option_context_add_group (context, gst_init_get_option_group ());

  if (!g_option_context_parse (context, argc, argv, &error)) {
    ERROR("option parsing failed: %s\n", error->message);
    exit (1);
  }

  g_option_context_free (context);
}

/**
 * @brief Frees properties read from argv.
 * Frees properties of GstSwitchUI * ui that have been read rom argv and need to be freed.
 */
static void
gst_switch_ui_free_args (GstSwitchUI * ui)
{
  g_free(ui->uifile);
}



/**
 * @brief GstSwitchUI initialisation function
 * Called by the GLib Class-System
 * @param ui The GstSwitchUI instance.
 */
static void
gst_switch_ui_init (GstSwitchUI * ui)
{
  INFO("gst_switch_ui_init");
}

/**
 * @brief Setup the UI Elements from the Glade-File, honoring the Command-Line Args
 */
static void
gst_switch_ui_setup (GstSwitchUI * ui)
{
  /* Create builder and load interface */
  if ( ui->uifile )
  {
    /* Load Builder from UI-File specified on Command-Line */
    ui->builder = gtk_builder_new_from_file( ui->uifile );
  }
  else
  {
    /* Load Builder UI-File bundled on Compile-Time */
    ui->builder = gtk_builder_new_from_resource( "/us/timvideos/gst-switch-ui/gstswitchui.ui" );
  }

  /* Obtain Main-Window Widget */
  INFO("loading mainwindow from builder-file");
  ui->mainwindow = GTK_WIDGET( gtk_builder_get_object( ui->builder, "mainwindow" ) );

  /* if the application is set non-quittable, try to disable the quit-button */
  if (ui->disallow_quit)
  {
    gtk_window_set_deletable( GTK_WINDOW( ui->mainwindow ), FALSE );
  }

  /* Obtain Quit-Dialog Widget */
  INFO("loading quitdialog from builder-file");
  ui->quitdialog = GTK_WIDGET( gtk_builder_get_object( ui->builder, "quitdialog" ) );

  /* Connect callbacks */
  gtk_builder_connect_signals( ui->builder, ui );
}

/**
 * @brief Destroy Widgets created in gst_switch_ui_init/_setup
 */
static void
gst_switch_ui_finalize (GstSwitchUI * ui)
{
  INFO("gst_switch_ui_finalize");
  gtk_widget_destroy (GTK_WIDGET (ui->mainwindow));
  ui->mainwindow = NULL;

  gtk_widget_destroy (GTK_WIDGET (ui->quitdialog));
  ui->quitdialog = NULL;

  /* Destroy builder */
  g_object_unref( G_OBJECT( ui->builder ) );
}

/**
 * @brief Run the UI Loop
 * @param ui The GstSwitchUI instance.
 * @memberof GstSwitchUI
 */
static void
gst_switch_ui_run (GstSwitchUI * ui)
{
  INFO("gst_switch_ui_run");
  /* Try to connect to the controller */
  if (!gst_switch_client_connect (GST_SWITCH_CLIENT( ui ), CLIENT_ROLE_UI)) {
    ERROR ("failed to connect to controller");
    return;
  }

  /* Prepare Audio- & Video-Feed Previews */
  //gst_switch_ui_prepare_previews( data.mainwindow );

  /* Show main window and start main loop */
  gtk_widget_show( ui->mainwindow );
  gtk_main();

  /* Mainloop ended */
}



/**** GstSwitchUI class methods ****/



static void
gst_switch_ui_on_controller_closed (GstSwitchUI * ui, GError * error)
{
  gtk_main_quit ();
}

static void
gst_switch_ui_set_compose_port (GstSwitchUI * ui, gint port)
{
  //
}

static void
gst_switch_ui_set_audio_port (GstSwitchUI * ui, gint port)
{
  //
}

static void
gst_switch_ui_add_preview_port (GstSwitchUI * ui, gint port, gint serve,
    gint type)
{
  //
}

static void
gst_switch_ui_show_face_marker (GstSwitchUI * ui, GVariant * faces)
{
  //
}

static void
gst_switch_ui_show_track_marker (GstSwitchUI * ui, GVariant * tracking)
{
  //
}



/**
 * @brief Connect class methods to GstSwitchUI Class
 * @param klass
 * @memberof GstSwitchUIClass
 */
static void
gst_switch_ui_class_init (GstSwitchUIClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GstSwitchClientClass *client_class = GST_SWITCH_CLIENT_CLASS (klass);

  object_class->finalize = (GObjectFinalizeFunc) gst_switch_ui_finalize;

  client_class->connection_closed = (GstSwitchClientConnectionClosedFunc) gst_switch_ui_on_controller_closed;
  client_class->set_compose_port  = (GstSwitchClientSetComposePortFunc)   gst_switch_ui_set_compose_port;
  client_class->set_audio_port    = (GstSwitchClientSetAudioPortFunc)     gst_switch_ui_set_audio_port;
  client_class->add_preview_port  = (GstSwitchClientAddPreviewPortFunc)   gst_switch_ui_add_preview_port;
  client_class->show_face_marker  = (GstSwitchClientShowFaceMarkerFunc)   gst_switch_ui_show_face_marker;
  client_class->show_track_marker = (GstSwitchClientShowFaceMarkerFunc)   gst_switch_ui_show_track_marker;
}

/**
 * @brief Called when the Main-Window is about to get deleted.
 * Potentially asks the user is this is ok.
 * @return TRUE when the window should be closedm FALSE otherwise
 */
G_MODULE_EXPORT gboolean
cb_delete_event( GtkWidget   *window,
                 GdkEvent    *event,
                 GstSwitchUI *ui )
{
  /* if the application is set non-quittable, always return TRUE */
  if (ui->disallow_quit)
  {
    INFO("the application was started none-quittable from the gui");
    return TRUE;
  }

  gint response = 1;

  /* Run dialog */
  INFO("showing quitdialog");
  response = gtk_dialog_run( GTK_DIALOG( ui->quitdialog ) );
  gtk_widget_hide( ui->quitdialog );

  return ( 1 != response );
}
