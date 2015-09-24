# include "astroid.hh"
# include "keybindings.hh"

# include "log.hh"

using namespace std;

namespace Astroid {
  Keybindings::Keybindings () {

  }

  void Keybindings::register_key (Key k, function<void(Key)> t) {
    keys.insert (KeyBinding (k, t));  
  }

  Key::Key () {

  }

  Key::Key (bool _c, bool _m, char k) {
    ctrl = _c;
    meta = _m;
    key  = k;
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
     */

    Key k;

    if (!((spec.size() == 1) || (spec.size () == 3) || (spec.size() == 5))) {
      log << error << "key spec invalid: " << spec << endl;
      return k;
    }

    if (spec.size () == 1) {
      k.ctrl = false;
      k.meta = false;

      k.key  = spec.c_str()[0];

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

      k.key = spec[2];
    }

    if (spec.size () == 5) {
      k.key = '0';
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

      k.key = spec[4];
    }

    if (!isalpha (k.key)) {
      log << error << "key spec invalid, must be a alphabetic: " << spec << endl;
      return k;
    }

    return k;
  } // }}}
}

