# include "astroid.hh"
# include "keybindings.hh"

# include "log.hh"

using namespace std;

namespace Astroid {
  Keybindings::Keybindings () {

  }

  void Keybindings::register_key (ustring spec,
                                  ustring name,
                                  ustring help,
                                  function<bool (Key)> t)
  {
    register_key (spec, vector<Key>(), name, help, t);
  }

  void Keybindings::register_key (ustring spec,
                                  vector<Key> aliases,
                                  ustring name,
                                  ustring help,
                                  function<bool (Key)> t)
  {

    register_key (Key::create (spec), aliases, name, help, t);

  }

  void Keybindings::register_key (ustring spec,
                                  vector<ustring> spec_aliases,
                                  ustring name,
                                  ustring help,
                                  function<bool (Key)> t)
  {

    vector<Key> aliases;
    for (auto &s : spec_aliases)
      aliases.push_back (Key::create (s));

    register_key (Key::create (spec), aliases, name, help, t);

  }

  void Keybindings::register_key (Key k,
                                  ustring name,
                                  ustring help,
                                  function<bool (Key)> t)
  {
    register_key (k, vector<Key> (), name, help, t);
  }

  void Keybindings::register_key (Key k,
                                  vector<Key> aliases,
                                  ustring name,
                                  ustring help,
                                  function<bool (Key)> t)
  {
    /* k     default key
     * name  name used for configurable keys
     */

    k.name = name;
    k.help = help;

    keys.insert (KeyBinding (k, t));

    k.hasaliases = !aliases.empty ();

    for (auto & ka : aliases) {
      ka.name = k.name;
      ka.help = k.help;
      ka.isalias = true;
      ka.master_key = &k;
    }
  }

  bool Keybindings::handle (GdkEventKey * event) {
    auto s = keys.find (Key(event));
    if (s != keys.end ()) {

      if (s->first.isalias) {
        auto m = keys.find (*(s->first.master_key));
        return m->second (s->first);
      } else {
        return s->second (s->first);
      }
    }

    return false;
  }

  Key::Key () { }

  Key::Key (bool _c, bool _m, char k, ustring _n, ustring _h) {
    ctrl = _c;
    meta = _m;
    key  = gdk_unicode_to_keyval(k);
    name = _n;
    help = _h;
  }

  Key::Key (bool _c, bool _m, guint k, ustring _n, ustring _h) {
    ctrl = _c;
    meta = _m;
    key  = k;
    name = _n;
    help = _h;
  }

  Key::Key (GdkEventKey *event, ustring _n, ustring _h) {
    ctrl = (event->state & GDK_CONTROL_MASK);
    meta = (event->state & GDK_MOD1_MASK);
    key  = event->keyval;
    name = _n;
    help = _h;
  }

  ustring Key::str () {
    ustring s;
    if (ctrl) s += "C-";
    if (meta) s += "M-";
    s += gdk_keyval_to_unicode (key);

    return s;
  }

  bool Key::operator== ( const Key & other ) const {
    return ((other.key == key) && (other.ctrl == ctrl) && (other.meta == meta));
  }

  bool Key::operator< ( const Key & other ) const {
    if (other.ctrl && !ctrl) return true;
    else if (ctrl && !other.ctrl) return false;

    if (other.meta && !meta) return true;
    else if (meta && !other.meta) return false;

    return key < other.key;
  }

  Key Key::create (string spec) { // {{{
    /* generate key from spec like:
     * k      : for key 'k'
     * C-k    : for Ctrl-k
     * M-k    : for Alt-k
     * C-M-K  : for Ctrl-Alt-Shift-K
     * K      : for Shift-k
     *
     * other keys supported:
     *
     * ESC
     * Up
     * Down
     * Backspace
     * Delete
     *
     */

    Key k;

    if (!((spec.size() == 1) || (spec.size () == 3) || (spec.size() == 5))) {
      log << error << "key spec invalid: " << spec << endl;
      return k;
    }

    if (spec.size () == 1) {
      k.ctrl = false;
      k.meta = false;

      k.key = gdk_unicode_to_keyval (spec[0]);
    }

    if (spec.size () >= 3) {
      /* one modifier */
      char M = spec[0];
      if (!((M == 'C') || (M == 'M'))) {
        log << error << "key spec invalid: " << spec << endl;
        return k;
      }

      if (M == 'C') k.ctrl = true;
      if (M == 'M') k.meta = true;

      if (spec[1] != '-') {
        log << error << "key spec invalid: " << spec << endl;
        return k;
      }

      k.key = gdk_unicode_to_keyval (spec[2]);
    }

    if (spec.size () == 5) {
      k.key = 0;
      char M = spec[2];

      if (!((M == 'C') || (M == 'M'))) {
        log << error << "key spec invalid: " << spec << endl;
        return k;
      }

      if (M == 'C') {
        if (k.ctrl) {
          log << error << "key spec invalid: " << spec << endl;
          return k;
        }
        k.ctrl = true;
      }
      if (M == 'M') {
        if (k.meta) {
          log << error << "key spec invalid: " << spec << endl;
          return k;
        }
        k.meta = true;
      }

      if (spec[3] != '-') {
        log << error << "key spec invalid: " << spec << endl;
        return k;
      }

      k.key = gdk_unicode_to_keyval (spec[4]);
    }

    return k;
  } // }}}
}

