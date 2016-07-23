import gi
gi.require_version ('Astroid', '0.1')
from gi.repository import GObject, Astroid

print ("basic: plugin loading..")

class BasicPlugin (GObject.Object, Astroid.Activatable):
  object = GObject.property (type=GObject.Object)

  def do_activate (self):
    print ('basic: activate')

  def do_deactivate (self):
    print ('basic: deactivate')

  def do_get_avatar_uri (self, email, tpe, size):
    print ("getting avatar uri", email, tpe, size)
    return "https://assets-cdn.github.com/images/modules/site/infinity-ill-small.png"

  def do_get_allowed_uris (self):
    return ["asdf", "https://assets-cdn.github.com/images/modules/site/"]

