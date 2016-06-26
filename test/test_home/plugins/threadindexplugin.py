import gi
gi.require_version ('Astroid', '0.1')
gi.require_version ('Gtk', '3.0')
from gi.repository import GObject, Gtk, Astroid

print ("threadindexplugin: plugin loading..")

class ThreadIndexPlugin (GObject.Object, Astroid.ThreadIndexActivatable):
  thread_index  = GObject.property (type = Gtk.Box)

  def do_activate (self):
    print ('tagformat: activate')
    self.label = Gtk.Label ("Hullu!")
    self.thread_index.pack_end (self.label, True, True, 5)


  def do_deactivate (self):
    print ('tagformat: deactivate')

  def do_format_tags (self, bg, tags, selected):

    from IPython import embed
    # embed ()

    print ("got tags: ", tags)

    return ", ".join (tags)

