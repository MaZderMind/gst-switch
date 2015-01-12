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
#include "logutils.h"


gboolean verbose, disallow_quit;
gchar *uifile = NULL;

static GOptionEntry entries[] = {
  {"verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose, "Be verbose.", NULL},
  {"dissalow-quit", 'Q', 0, G_OPTION_ARG_NONE, &disallow_quit, "Don't allow the user to quit the Application from the GUI.", NULL},
  {"ui-file", 'u', 0, G_OPTION_ARG_FILENAME, &uifile, "Supply a custom .glade-File to build the GUI from.", NULL},
  {NULL}
};

/**
 * @brief Parse command line arguments.
 */
static void
gst_switch_ui_parse_args (int *argc, char **argv[])
{
  GOptionContext *context;
  GError *error = NULL;
  context = g_option_context_new ("");

  g_option_context_add_main_entries (context, entries, "gst-switch-ui");
  g_option_context_add_group (context, gst_init_get_option_group ());

  if (!g_option_context_parse (context, argc, argv, &error)) {
    ERROR("option parsing failed: %s\n", error->message);
    exit (1);
  }

  g_option_context_free (context);
}

/**
 * @brief The entry of gst-switch-ui.
 */
int
main (int argc, char *argv[])
{
  GtkBuilder *builder;
  UiData      data;

  gtk_init (&argc, &argv);
  gst_switch_ui_parse_args (&argc, &argv);

  /* Create builder and load interface */
  if ( uifile )
  {
    builder = gtk_builder_new_from_file( uifile );
  }
  else
  {
    builder = gtk_builder_new_from_file( "ui/gstswitchui.glade" );
  }

  /* Obtain widgets that we need */
  INFO("loading mainwindow from builder-file");
  data.mainwindow = GTK_WIDGET( gtk_builder_get_object( builder, "mainwindow" ) );

  /* if the application is set non-quittable, try to disable the quit-button */
  if (disallow_quit)
  {
    gtk_window_set_deletable( GTK_WINDOW( data.mainwindow ), FALSE );
  }

  INFO("loading quitdialog from builder-file");
  data.quitdialog = GTK_WIDGET( gtk_builder_get_object( builder, "quitdialog" ) );

  /* Connect callbacks */
  gtk_builder_connect_signals( builder, &data );

  /* Destroy builder */
  g_object_unref( G_OBJECT( builder ) );

  /* Show main window and start main loop */
  gtk_widget_show( data.mainwindow );
  gtk_main();

  return 0;
}

G_MODULE_EXPORT gboolean
cb_delete_event( GtkWidget *window,
                 GdkEvent  *event,
                 UiData    *data )
{
  /* if the application is set non-quittable, always return TRUE */
  if (disallow_quit)
  {
    INFO("the application was started none-quittable'from the gui");
    return TRUE;
  }

  gint response = 1;

  /* Run dialog */
  INFO("showing quitdialog");
  response = gtk_dialog_run( GTK_DIALOG( data->quitdialog ) );
  gtk_widget_hide( data->quitdialog );

  return ( 1 != response );
}
