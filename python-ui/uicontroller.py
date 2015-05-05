import os, logging

# load gst-switch modules
from gstswitch.controller import Controller
from gstswitch.exception import ConnectionError
from gstswitch.connection import Connection

# load Gtk-Module
from gi.repository import Gtk

# load UI Components
from uivideodisplay import UIVideoDisplay


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
    self.win = self.get_check_widget("window")

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

    # Aquire current Composition-Mode
    mode = self.ctr.get_composite_mode()
    self.log.info("current composition-mode is: %u", mode)

    # Select the correct Button and set it Active
    composite_buttons = self.get_check_widget('composite_buttons')
    buttons = composite_buttons.get_children()
    buttons[mode-1].set_active(True)

    # Connect Callback-Events with UI-Controller function
    self.builder.connect_signals({
      "composite_button_clicked": self.composite_button_clicked,
    })

    # Configure a GStreamer Pipeline showing the Composite-Video in the primary_video-Section of the GUI
    self.log.debug('Starting UIVideoDisplay for primary_video-Widget')
    self.primary_video_display = UIVideoDisplay(
      widget=self.get_check_widget('primary_video'),
      port=self.ctr.get_compose_port())


  def get_check_widget(self, widget_id):
    widget = self.builder.get_object(widget_id)
    if not widget:
      self.log.error('could not load required widget "%s" from the .ui-File', widget_id)
      raise Exception('Widget not found in .ui-File')

    return widget


  # Run all GStreamer Pipelines and the GTK-Mainloop
  def run(self):
    self.win.show_all()
    self.primary_video_display.run()
    Gtk.main()


  # Click on one of the Composition-Mode selection Buttons
  def composite_button_clicked(self, btn):
    # Ignore the event on the now de-selected Button
    if not btn.get_active():
      return

    composite_buttons = self.get_check_widget('composite_buttons')
    composite_buttons = self.get_check_widget('composite_buttons')
    buttons = composite_buttons.get_children()
    idx = buttons.index(btn)

    self.log.info('switching to new composition-mode: %u', idx)
    self.ctr.set_composite_mode(idx)
