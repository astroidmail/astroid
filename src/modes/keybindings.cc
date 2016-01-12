# include <atomic>

# include "astroid.hh"
# include "keybindings.hh"
# include "config.hh"
# include "utils/ustring_utils.hh"
# include "utils/vector_utils.hh"

# include "log.hh"

using namespace std;

namespace Astroid {

  /* Keybindings {{{ */
  atomic<bool> Keybindings::user_bindings_loaded (false);
  const char * Keybindings::user_bindings_file = "keybindings";
  vector<Key>  Keybindings::user_bindings;

  map<guint, ustring> keynames = {
    { GDK_KEY_Down,   "Down" },
    { GDK_KEY_Up,     "Up" },
    { GDK_KEY_Tab,    "Tab" },
    { GDK_KEY_Home,   "Home" },
    { GDK_KEY_End,    "End" },
    { GDK_KEY_Return, "Return" },
    { GDK_KEY_KP_Enter, "KP_Enter" },
  };

  void Keybindings::init () {
    if (!user_bindings_loaded) {
      user_bindings_loaded = true;

      path bindings_file = astroid->config->config_dir / path (user_bindings_file);

      if (exists(bindings_file)) {
        log << info << "keybindings: loading user bindings from: " << bindings_file.c_str () << endl;

        /* the bindings file has the format:
         *
         * ```
         * thread_index.next_thread=j
         * thread_index.next_thread=Down
         * thread_index.label=C-j
         *
         * # thread_
         * ```
         *
         * blank lines, or lines starting with # are ignored. a keybinding
         * can be listed several times, in which case they will be interpreted as
         * aliases for the same target.
         */

        std::ifstream bf (bindings_file.c_str());

        while (!bf.eof()) {

          ustring line;

          bf >> line;

          UstringUtils::trim (line);

          if (line.size () == 0) continue;
          if (line[0] == '#') continue;

          log << debug << "ky: parsing line: " << line << endl;
          vector<ustring> parts = VectorUtils::split_and_trim (line, "=");

          if (parts.size () != 2) {
            log << error << "ky: user bindings: too many parts in: " << line << endl;
            continue;
          }

          Key k = Key (parts[1]);
          k.name = parts[0];

          user_bindings.push_back (k);
        }

        bf.close ();
      }
    }
  }

  Keybindings::Keybindings () {
  }

  ustring Keybindings::short_help () {
    ustring h;

    bool first = true;

    for (auto &km : keys) {
      auto k = km.first;

      if (!first) {
        h += ", ";
      }

      first = false;

      h += k.str () + ": " + k.help;
    }

    return h;
  }

  ustring Keybindings::help () {
    ustring h;

    for (auto &km : keys) {
      auto k = km.first;

      if (k.isalias) continue;

      h += "<b>" + k.str ();

      auto aliases = find_if (keys.begin (), keys.end (),
          [&](KeyBinding kb) {
            return (kb.first.isalias && (*(kb.first.master_key) == k));
          });

      while (aliases != keys.end ()) {
        h += "," + aliases->first.str ();

        aliases++;
        aliases = find_if (aliases, keys.end (),
            [&](KeyBinding kb) {
              return (kb.first.isalias && (*(kb.first.master_key) == k));
            });
      }

      h += "</b>: " + k.help + "\n";
    }

    return h;
  }

  void Keybindings::clear () {
    keys.clear ();
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

    register_key (Key (spec), aliases, name, help, t);

  }

  void Keybindings::register_key (ustring spec,
                                  vector<ustring> spec_aliases,
                                  ustring name,
                                  ustring help,
                                  function<bool (Key)> t)
  {

    vector<Key> aliases;
    for (auto &s : spec_aliases)
      aliases.push_back (Key (s));

    register_key (Key (spec), aliases, name, help, t);

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

    log << debug << "key: registering key: " << name << ": " << k.str () << endl;

    /* check if these are user configured */
    auto res = find_if (user_bindings.begin (),
                        user_bindings.end (),
                        [&](Key e) {
                          return (e.name == name);
                        });

    bool userdefined = false;

    // add aliases
    if (res != user_bindings.end ()) {
      userdefined = true;
      Key uk = (*res);

      k.key  = uk.key;
      k.ctrl = uk.ctrl;
      k.meta = uk.meta;

      res++;
      res = find_if (res,
                     user_bindings.end (),
                     [&](Key e) {
                       return (e.name == name);
                     });

      aliases.clear (); // drop any default aliases

      while (res != user_bindings.end ()) {
        Key ak = (*res);

        aliases.push_back (ak);

        res++;
        res = find_if (res++,
                       user_bindings.end (),
                       [&](Key e) {
                         return (e.name == name);
                       });

      }
    }

    k.name = name;
    k.help = help;

    k.userdefined = userdefined;
    k.isalias = false;
    k.hasaliases = !aliases.empty ();

    /* check if key name already exists */
    if (find_if (keys.begin (), keys.end (),
          [&] (KeyBinding mk) {
            return mk.first.name == k.name;
          }) != keys.end ()) {

      log << error << ustring::compose (
            "key: %1, there is a key with name %2 registered already",
            k.str (), k.name) << endl;
      throw duplicatekey_error (ustring::compose (
            "key: %1, there is a key with name %2 registered already",
            k.str (), k.name).c_str ());
    }

    auto r = keys.insert (KeyBinding (k, t));
    if (!r.second) {
      if (!r.first->first.userdefined && k.userdefined) {
        /* default key, removing and replacing with user defined. target of
         * default key will be unreachable.
         */
        ustring wrr = ustring::compose (
            "key: %1 (%2) already exists in map with name: %3, overwriting.",
            k.str (), k.name, r.first->first.name);

        log << warn << wrr << endl;

        keys.erase (r.first);
        r = keys.insert (KeyBinding (k, t));

      } else {
        ustring err = ustring::compose (
            "key: %1 (%2) is already user-configured in map with name: %3",
            k.str (), k.name, r.first->first.name);

        log << error << err << endl;

        throw duplicatekey_error (err.c_str());
      }
    }

    /* get pointer to key in map */
    const Key * master;
    auto s = keys.find (k);
    master = &(s->first);

    for (auto & ka : aliases) {

      log << debug << "key: alias: " << ka.str () <<  "(" << ka.key << ")" << endl;

      ka.name = k.name;
      ka.help = k.help;
      ka.userdefined = userdefined;
      ka.isalias = true;
      ka.master_key = master;
      auto r = keys.insert (KeyBinding (ka, NULL));

      if (!r.second) {
        if (!r.first->first.userdefined && ka.userdefined) {
          /* default key, removing and replacing with user defined. target of
           * default key will be unreachable.
           */
          ustring wrr = ustring::compose (
              "key: %1 (%2) already exists in map with name: %3, overwriting.",
              k.str (), k.name, r.first->first.name);

          log << warn << wrr << endl;

          keys.erase (r.first);
          r = keys.insert (KeyBinding (k, t));

        } else {
          ustring err = ustring::compose (
              "key: %1 (%2) is already user-configured in map with name: %3",
              k.str (), k.name, r.first->first.name);

          log << error << err << endl;

          throw duplicatekey_error (err.c_str());
        }
      }
    }
  }

  bool Keybindings::handle (GdkEventKey * event) {
    Key ek (event);
    auto s = keys.find (ek);

    if (s != keys.end ()) {
      if (loghandle)
        log << debug << "ky: " << title << ", handling: " << s->first.str () << " (" << s->first.name << ")" << endl;

      if (s->first.isalias) {
        auto m = keys.find (*(s->first.master_key));
        return m->second (s->first);
      } else {
        return s->second (s->first);
      }
    } else {
      if (loghandle)
        log << debug << "ky: " << title << ",  unknown key: " << ek.str () << endl;
    }

    return false;
  }

  // }}}

  /* Key {{{ */
  Key::Key () { }

  Key::Key (ustring spec, ustring _n, ustring _h) { // {{{
    /* generate key from spec like:
     * k      : for key 'k'
     * C-k    : for Ctrl-k
     * M-k    : for Alt-k
     * C-M-K  : for Ctrl-Alt-K
     * K      : for Shift-k
     *
     * TODO: other keys supported: check keynames map.
     *
     */

    name = _n;
    help = _h;

    if (!((spec.size() == 1) || (spec.size () == 3) || (spec.size() == 5))) {
      log << error << "key spec invalid: " << spec << endl;
      throw keyspec_error ("invalid length of spec");
    }

    if (spec.size () == 1) {
      ctrl = false;
      meta = false;

      key = gdk_unicode_to_keyval (spec[0]);
    }

    if (spec.size () >= 3) {
      /* one modifier */
      char M = spec[0];
      if (!((M == 'C') || (M == 'M'))) {
        log << error << "key spec invalid: " << spec << endl;
        throw keyspec_error ("invalid modifier in key spec");
      }

      if (M == 'C') ctrl = true;
      if (M == 'M') meta = true;

      if (spec[1] != '-') {
        log << error << "key spec invalid: " << spec << endl;
        throw keyspec_error ("invalid delimiter");
      }

      key = gdk_unicode_to_keyval (spec[2]);
    }

    if (spec.size () == 5) {
      key = 0;
      char M = spec[2];

      if (!((M == 'C') || (M == 'M'))) {
        log << error << "key spec invalid: " << spec << endl;
        throw keyspec_error ("invalid modifier");
      }

      if (M == 'C') {
        if (ctrl) {
          log << error << "key spec invalid: " << spec << endl;
          throw keyspec_error ("modifier already specified");
        }
        ctrl = true;
      }
      if (M == 'M') {
        if (meta) {
          log << error << "key spec invalid: " << spec << endl;
          throw keyspec_error ("modifier already specified");
        }
        meta = true;
      }

      if (spec[3] != '-') {
        log << error << "key spec invalid: " << spec << endl;
        throw keyspec_error ("invalid delimiter");
      }

      key = gdk_unicode_to_keyval (spec[4]);
    }
  } // }}}

  Key::Key (guint k, ustring _n, ustring _h) {
    ctrl = false;
    meta = false;
    key  = k;
    name = _n;
    help = _h;
  }

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

  ustring Key::str () const {
    ustring s;
    if (ctrl) s += "C-";
    if (meta) s += "M-";

    char k = gdk_keyval_to_unicode (key);
    if (isgraph (k)) {
      s += k;
    } else {
      try {
        ustring kk = keynames.at (key);
        s += kk;
      } catch (exception &ex) {
        s += ustring::compose ("%1", key);
      }
    }

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

  /************
   * exceptions
   * **********
   */

  keyspec_error::keyspec_error (const char * w) : runtime_error (w)
  {
  }

  duplicatekey_error::duplicatekey_error (const char * w) : runtime_error (w)
  {
  }

  // }}}

}

