#!/usr/bin/env python

import os, sys, signal, logging, argparse

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

# load gi modules
from gi.repository import Gtk, Gdk, Gst, GObject, GdkX11, GstVideo

# init GObject & Co. before importing local classes
GObject.threads_init()
Gdk.init([])
Gtk.init([])
Gst.init([])

from uicontroller import UIController

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
