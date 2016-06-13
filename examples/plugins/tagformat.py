import gi
gi.require_version ('Astroid', '0.1')
from gi.repository import GObject, Gtk, Astroid

print ("tagformat: plugin loading..")

class TagFormatPlugin (GObject.Object, Astroid.ThreadIndexActivatable):
  object = GObject.property (type=GObject.Object)
  # listview = GObject.property (type = Gtk.TreeView)

  def do_activate (self):
    print ('tagformat: activate')

  def do_deactivate (self):
    print ('tagformat: deactivate')

  def do_format_tags (self, tags):

    from IPython import embed
    # embed ()

    print ("got tags: ", tags)

    return ", ".join (tags)

