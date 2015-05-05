import logging
from gi.repository import Gst

class UIVideoDisplay:
  """ Displays a GST-Switch Server-Video into a GtkWidget """
  log = logging.getLogger('UI.VideoDisplay')

  def __init__(self, port, widget):
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
