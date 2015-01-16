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
from gstswitch.exception import ConnectionReturnError
from gstswitch.connection import Connection
from gi.repository import GLib, Gtk, Gdk, Gst, GObject, GdkX11, GstVideo

# init GObject before importing local classes
GObject.threads_init()
Gdk.init([])
Gtk.init([])
Gst.init([])

class GstSwichUI:
  """ UI Controller Class """
  log = logging.getLogger('GstSwichUI')

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
        self.log.info('trying to load ui-file from file %s', path)

        if os.path.isfile(path):
          self.log.info('loading ui-file from file %s', path)
          self.builder.add_from_file(os.path.abspath(os.path.join(__file__, '../../ui/gst-switch.ui')))
          break


    # Aquire the Main-Window from the UI-File
    self.win = self.builder.get_object("window")

    # Connect Close-Handler
    self.win.connect("delete-event", Gtk.main_quit)

    # If the Application was started non-quittable, disable the close-button
    if self.options.dissalow_quit:
      self.win.set_deletable(False)

    self.ctr = Controller()
    self.ctr.establish_connection()

    self.connect_primary_video()

  def connect_primary_video(self):
    self.primary_video_pipeline = Gst.parse_launch('tcpclientsrc port={port} ! gdpdepay ! xvimagesink'.format(
      port=self.ctr.get_compose_port()
    ))

    primary_video_widget = self.builder.get_object('primary_video')
    primary_video_widget.realize()
    self.primary_video_widget_id = primary_video_widget.get_property('window').get_xid()

    bus = self.primary_video_pipeline.get_bus() 
    bus.add_signal_watch() 
    bus.enable_sync_message_emission() 
    bus.connect("sync-message::element", self.primary_video_syncmsg)

    self.primary_video_pipeline.set_state(Gst.State.PLAYING)

  def primary_video_syncmsg(self, bus, message):
    struct = message.get_structure()
    print(struct.get_name())
    if struct is None:
        return False

    if struct.get_name() == "prepare-window-handle":
        Gdk.threads_enter()
        Gdk.Display.get_default().sync()

        imagesink = message.src
        imagesink.set_property("force-aspect-ratio", True)
        imagesink.set_window_handle(self.primary_video_widget_id)

        Gdk.threads_leave()


  def run(self):
    self.win.show_all()
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
  log = logging.getLogger('main()')

  # parse command-line args
  args = parseArgs()

  # configure logging
  docolor = (args.color == 'always') or (args.color == 'auto' and sys.stderr.isatty())
  logging.basicConfig(
    level=logging.DEBUG if args.verbose else logging.WARNING,
    format='\x1b[33m%(levelname)8s\x1b[0m \x1b[32m%(name)s\x1b[0m: %(message)s' if docolor else '%(levelname)8s %(name)s: %(message)s')

  # construct the UI from a Glade-File
  log.debug('constructing UI')
  ui = GstSwichUI(args)

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
