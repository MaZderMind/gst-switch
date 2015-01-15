#!/usr/bin/env python

import os, sys, signal, logging, argparse

paths = [
  os.path.abspath(os.path.join(__file__, '../../python-api')),
  '/usr/lib/gst-switch/python-api'
]

for path in paths:
  if os.path.isdir(path):
    sys.path.insert(0, path)
    break

from gstswitch.controller import Controller
from gstswitch.exception import ConnectionReturnError
from gstswitch.connection import Connection
from gi.repository import GLib, Gtk


class GstSwichUI:
  log = logging.getLogger('GstSwichUI')

  def __init__(self, options):
    self.options = options

    builder = Gtk.Builder()
    if self.options.ui_file:
      self.log.info('loading ui-file from file specified on command-line: %s', self.options.ui_file)
      builder.add_from_file(self.options.ui_file)
    else:
      paths = [
        os.path.abspath(os.path.join(__file__, '../../ui/gst-switch.ui')),
        '/usr/lib/gst-switch/ui/gst-switch.ui'
      ]

      for path in paths:
        self.log.info('trying to load ui-file from file %s', path)
        if os.path.isfile(path):
          self.log.info('loading ui-file from file %s', path)
          builder.add_from_file(os.path.abspath(os.path.join(__file__, '../../ui/gst-switch.ui')))
          break

    self.win = builder.get_object("window")
    self.win.connect("delete-event", Gtk.main_quit)
    if self.options.dissalow_quit:
      self.win.set_deletable(False)

  def run(self):
    self.win.show_all()
    Gtk.main()



def parseArgs():
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
  log = logging.getLogger('main()')

  # parse command-line args
  args = parseArgs()

  # configure logging
  docolor = (args.color == 'always') or (args.color == 'auto' and sys.stderr.isatty())
  logging.basicConfig(
    level=logging.DEBUG if args.verbose else logging.WARNING,
    format='\x1b[33m%(levelname)8s\x1b[0m \x1b[32m%(name)s\x1b[0m: %(message)s' if docolor else '%(levelname)8s %(name)s: %(message)s')

  log.debug('constructing UI')
  ui = GstSwichUI(args)

  log.debug('running UI')
  ui.run()
  log.debug('done')



if __name__ == "__main__":
  signal.signal(signal.SIGINT, signal.SIG_DFL)
  main()
