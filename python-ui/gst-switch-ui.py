#!/usr/bin/env python

import os, sys, signal, logging, argparse
from inspect import getmembers

# Paths to look for the gst-switch python-api
paths = [
  os.path.abspath(os.path.join(__file__, '../../python-api')),
  '/usr/lib/gst-switch/python-api'
]

# Look for a gst-switch python-api and add it to the loading-path
for path in paths:
  if os.path.isdir(path):
    sys.path.insert(0, path)
    break

# load gst-switch modules
from gstswitch.controller import Controller
from gstswitch.exception import ConnectionError, ConnectionReturnError
from gstswitch.connection import Connection
from gi.repository import GLib, Gtk, Gdk, Gst, GObject, GdkX11, GstVideo

# init GObject before importing local classes
GObject.threads_init()
Gdk.init([])
Gtk.init([])
Gst.init([])

class UIVideoDisplay:
  """ Displays a GST-Switch Server-Video into a GtkWidget """
  log = logging.getLogger('UI.VideoDisplay')

  def __init__(self, port, widget):
    if not widget:
      self.log.error('No widget supplied. Is it present in your .ui-File and named correctly?')
      raise Exception('UIVideoDisplay initialized without a widget')

    pstr = 'tcpclientsrc port={} ! gdpdepay ! xvimagesink sync=false'.format(port)
    self.log.info('launching gstreamer-pipeline for widget %s: %s', widget.get_name(), pstr)

    self.pipeline = Gst.parse_launch(pstr)

    widget.realize()
    self.xid = widget.get_property('window').get_xid()

    bus = self.pipeline.get_bus()
    bus.add_signal_watch()
    bus.enable_sync_message_emission()

    bus.connect('message::error', self.on_error)
    bus.connect("sync-message::element", self.on_syncmsg)

  def run(self):
    self.pipeline.set_state(Gst.State.PLAYING)


  def on_syncmsg(self, bus, msg):
    if msg.get_structure().get_name() == "prepare-window-handle":
        msg.src.set_window_handle(self.xid)

  def on_error(self, bus, msg):
      self.log.error('on_error():', msg.parse_error())



class UIController:
  """ UI Controller Class """
  log = logging.getLogger('UI.Controller')

  # Construct the UI from a Glade-File
  def __init__(self, options):
    self.options = options

    # Instanciate GTK-Builder
    self.builder = Gtk.Builder()

    # Uf a UI-File was specified on the Command-Line, load it
    if self.options.ui_file:
      self.log.info('loading ui-file from file specified on command-line: %s', self.options.ui_file)
      self.builder.add_from_file(self.options.ui_file)

    else:
      # Paths to look for the gst-switch UI-File
      paths = [
        os.path.abspath(os.path.join(__file__, '../../ui/gst-switch.ui')),
        '/usr/lib/gst-switch/ui/gst-switch.ui'
      ]

      # Look for a gst-switch UI-File and load it
      for path in paths:
        self.log.debug('trying to load ui-file from file %s', path)

        if os.path.isfile(path):
          self.log.info('loading ui-file from file %s', path)
          self.builder.add_from_file(os.path.abspath(os.path.join(__file__, '../../ui/gst-switch.ui')))
          break


    # Aquire the Main-Window from the UI-File
    self.log.debug('Loading Main-Window "window"  from .ui-File')
    self.win = self.builder.get_object("window")

    if not self.win:
      self.log.error('Main-Window "window" found in .ui-File. Is it present and named correctly?')
      raise Exception('Main-Window not found in .ui-File')

    # Connect Close-Handler
    self.win.connect("delete-event", Gtk.main_quit)

    # If the Application was started non-quittable, disable the close-button
    if self.options.dissalow_quit:
      self.win.set_deletable(False)

    # Open a Control-Connection to the Server
    self.ctr = Controller()
    try:
      self.ctr.establish_connection()
    except ConnectionError as e:
      self.log.error('Could not connect to gst-switch-srv Server')
      raise e

    # Configure a GStreamer Pipeline showing the Composite-Video in the primary_video-Section of the GUI
    self.log.debug('Starting UIVideoDisplay for primary_video-Widget')
    self.primary_video_display = UIVideoDisplay(
      widget=self.builder.get_object('primary_video'),
      port=self.ctr.get_compose_port())

  # Run all GStreamer Pipelines and the GTK-Mainloop
  def run(self):
    self.win.show_all()
    self.primary_video_display.run()
    Gtk.main()



def parseArgs():
  """ Configure argparse and parse incoming Command-Line arguments """
  parser = argparse.ArgumentParser(description='gst-switch-ui Client')

  parser.add_argument('-v', '--verbose', action='store_true',
      help="Also print INFO and DEBUG messages.")
  parser.add_argument('-c', '--color', action='store', choices=['auto', 'always', 'never'], default='auto',
      help="Control the use of colors in the Log-Output")
  parser.add_argument('-u', '--ui-file', action='store',
      help="Load a custom Glade-File")
  parser.add_argument('-Q', '--dissalow-quit', action='store_true',
      help="Don't allow the user to quit the Application from the Gui")

  return parser.parse_args()



def main():
  """ Main Entry-Point """
  log = logging.getLogger('UI')

  # parse command-line args
  args = parseArgs()

  # configure logging
  docolor = (args.color == 'always') or (args.color == 'auto' and sys.stderr.isatty())
  logging.basicConfig(
    level=logging.DEBUG if args.verbose else logging.WARNING,
    format='\x1b[33m%(levelname)8s\x1b[0m \x1b[32m%(name)s\x1b[0m: %(message)s' if docolor else '%(levelname)8s %(name)s: %(message)s')

  # construct the UI from a Glade-File
  log.debug('constructing UI')
  ui = UIController(args)

  # Launch UI processed
  log.debug('running UI')
  ui.run()
  log.debug('done')



# Allow loading as a module
if __name__ == "__main__":
  # Trap Ctrl-C
  signal.signal(signal.SIGINT, signal.SIG_DFL)

  # Call main Entry-Point
  main()
