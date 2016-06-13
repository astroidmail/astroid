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

  def do_ask (self, n):
    print (repr(n))
    print ("got ask:", n)

    from IPython import embed
    # embed ()


    n = n.replace ("hello", "asdf")
    print (n)

    return n

  def do_get_avatar_uri (self, email, tpe, size):
    print ("getting avatar uri", email, tpe, size)
    return ""

