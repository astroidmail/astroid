# include <atomic>
# include <functional>

# include "astroid.hh"
# include "keybindings.hh"
# include "config.hh"
# include "utils/ustring_utils.hh"
# include "utils/vector_utils.hh"

using std::function;
using std::vector;
using std::endl;

namespace Astroid {

  /* Keybindings {{{ */
  std::atomic<bool> Keybindings::user_bindings_loaded (false);
  const char * Keybindings::user_bindings_file = "keybindings";
  std::vector<Key>  Keybindings::user_bindings;
  std::vector<std::pair<Key, ustring>> Keybindings::user_run_bindings;

  std::map<guint, ustring> Keybindings::keyval_to_keynames = {
    { GDK_KEY_Down,   "Down" },
    { GDK_KEY_Up,     "Up" },
    { GDK_KEY_Tab,    "Tab" },
    { GDK_KEY_ISO_Left_Tab, "ISO_Left_Tab" },
    { GDK_KEY_Home,   "Home" },
    { GDK_KEY_End,    "End" },
    { GDK_KEY_Return, "Return" },
    { GDK_KEY_KP_Enter, "KP_Enter" },
    { GDK_KEY_Page_Up, "Page_Up" },
    { GDK_KEY_Page_Down, "Page_Down" },
    { GDK_KEY_Escape, "Esc" },
    { GDK_KEY_equal, "Equal" },
    { GDK_KEY_minus, "Minus" },
    { GDK_KEY_space, "Space" },
  };

  std::map<ustring, guint> Keybindings::keynames_to_keyval;

  void Keybindings::init () {
    /* build keynames_to_keyval */
    keynames_to_keyval.clear ();
    for (auto &s : keyval_to_keynames) {
      keynames_to_keyval.insert (std::make_pair (s.second, s.first));
    }

    if (!user_bindings_loaded) {
      user_bindings_loaded = true;

      using bfs::path;
      path bindings_file = astroid->standard_paths ().config_dir / path (user_bindings_file);

      if (exists(bindings_file)) {
        LOG (info) << "keybindings: loading user bindings from: " << bindings_file.c_str ();

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
         * blank lines, or lines starting with # are ignored. a keybinding can
         * be listed several times, in which case they will be interpreted as
         * aliases for the same target.
         *
         * shell hooks follow the format:
         *
         *  thread_index.run(cmd)=key
         *
         *  it will be parsed by searching for:
         *
         *  1. prefix.run(
         *  2. then search backwards for =
         *  3. split out key
         *  4. search backwards for )
         *  5. split out cmd
         *
         * a mode then registers run targets which is a function that gets the
         * target, runs it and signals update_thread or similar if necessary.
         *
         * a helper for running the command should be located here.
         *
         */

        std::ifstream bf (bindings_file.c_str());

        while (!bf.eof()) {

          std::string sline;
          std::getline (bf, sline);
          ustring line (sline);

          UstringUtils::trim (line);

          if (line.size () == 0) continue;
          if (line[0] == '#') continue;

          std::size_t fnd;

          /* check if this is a run line */
          fnd = line.find (".run");
          if (fnd != std::string::npos) {
            LOG (debug) << "ky: parsing run-hook: " << line;

            /* get full name */
            ustring name = line.substr (0, fnd + 4);

            /* get target, between  () */
            fnd = line.find ("(", fnd);

            if (fnd == std::string::npos) {
              LOG (error) <<  "ky: invalid 'run'-specification: no '('";
              continue;
            }

            std::size_t rfnd = line.rfind ("=");
            if (rfnd == std::string::npos) {
              LOG (error) << "ky: invalid 'run'-specification: no '='";
              continue;
            }

            if (rfnd < fnd) {
              LOG (error) << "ky: invalid 'run'-specification:  '=' before '('";
              continue;
            }

            ustring keyspec = line.substr (rfnd+1, std::string::npos);
            UstringUtils::trim (keyspec);

            rfnd = line.rfind (")", rfnd);
            if (rfnd == std::string::npos) {
              LOG (error) << "ky: invalid 'run'-specfication: no ')'";
              continue;
            }

            if (rfnd < fnd) {
              LOG (error) << "ky: invalid 'run'-specification: ')' before '('";
              continue;
            }

            ustring target = line.substr (fnd + 1, rfnd - fnd -1);
            UstringUtils::trim (target);

            Key k (keyspec);

            LOG (debug) << "ky: run: " << name << "(" << k.str () << "): " << target;

            k.name = name;
            k.allow_duplicate_name = true;
            k.userdefined = true;

            user_run_bindings.push_back (std::make_pair (k, target));

            continue;
          }

          /* cut off comments appended to the end of the line */
          fnd = line.find ("#");
          if (fnd != std::string::npos) {
            line = line.substr (0, fnd);
          }

          LOG (debug) << "ky: parsing line: " << line;
          vector<ustring> parts = VectorUtils::split_and_trim (line, "=");

          if (parts.size () != 2) {
            LOG (error) << "ky: user bindings: too many parts in: " << line;
            continue;
          }

          ustring spec = parts[1];
          UstringUtils::trim (spec);

          Key k;

          if (spec.empty ()) {
            k = UnboundKey ();
          } else {
            k = Key (spec);
          }

          k.name = parts[0];

          user_bindings.push_back (k);
        }

        bf.close ();
      }
    }
  }

  Keybindings::Keybindings () {
    /* loghandle = true; */
  }

  void Keybindings::set_prefix (ustring t, ustring p) {
    title  = t;
    prefix = p;
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

    /* LOG (debug) << "key: registering key: " << name << ": " << k.str (); */

    /* check if these are user configured */
    auto res = find_if (user_bindings.begin (),
                        user_bindings.end (),
                        [&](Key e) {
                          return (e.name == name);
                        });

    bool userdefined = k.userdefined;

    // add aliases
    if (res != user_bindings.end ()) {
      userdefined = true;
      Key uk = (*res);

      if (uk.unbound) {
        /* user defined and unbound, key binding is dropped */
        LOG (debug) << "ky: key: " << k.str () << " dropped." << endl;
        return;
      }

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

      if (k.unbound) {
        LOG (info) << "key: binding unbound target: " << k.name;
        k.unbound = false;
      }
    }

    if (k.unbound) {
      /* LOG (info) << "key: unbound key: " << name << " does not have a key associated."; */
      return;
    }

    k.name = name;
    k.help = help;

    k.userdefined = userdefined;
    k.isalias = false;
    bool has_aliases = false;

    /* check if key name already exists */
    if (!k.allow_duplicate_name) {
      if (find_if (keys.begin (), keys.end (),
            [&] (KeyBinding mk) {
              return mk.first.name == k.name;
            }) != keys.end ()) {

        LOG (error) << ustring::compose (
              "key: %1, there is a key with name %2 registered already",
              k.str (), k.name);
        throw duplicatekey_error (ustring::compose (
              "key: %1, there is a key with name %2 registered already",
              k.str (), k.name).c_str ());
      }
    }

    bool has_master = true;

    auto r = keys.insert (KeyBinding (k, t));
    if (!r.second) {
      /* LOG (debug) << "user def: " << k.userdefined; */
      if (!r.first->first.userdefined && k.userdefined) {
        /* default key, removing and replacing with user defined. target of
         * default key will be unreachable.
         */
        ustring wrr = ustring::compose (
            "key: %1 (%2) already exists in map with name: %3, overwriting.",
            k.str (), k.name, r.first->first.name);

        LOG (warn) << wrr;

        keys.erase (r.first);
        r = keys.insert (KeyBinding (k, t));

      } else if (r.first->first.userdefined && k.userdefined) {
        ustring err = ustring::compose (
            "key: %1 (%2) is already user-configured in map with name: %3",
            k.str (), k.name, r.first->first.name);

        LOG (error) << err;

        throw duplicatekey_error (err.c_str());

      } else if (r.first->first.userdefined && !k.userdefined) {
        ustring err = ustring::compose (
            "key: %1 (%2) is user-configured in map with name: %3, will try aliases.",
            k.str (), k.name, r.first->first.name);

        LOG (warn) << err;
        has_master = false;

      } else {
        /* neither are user-configured, we have a hardcoded conflict */

        ustring err = ustring::compose (
            "key: %1 (%2) is already mapped with name: %3",
            k.str (), k.name, r.first->first.name);

        LOG (error) << err;

        throw duplicatekey_error (err.c_str());
      }
    }

    /* get pointer to key in map */
    const Key * master;
    if (has_master) {
      auto s = keys.find (k);
      master = &(s->first);
    }

    for (auto & ka : aliases) {

      /* LOG (debug) << "key: alias: " << ka.str () <<  "(" << ka.key << ")"; */

      ka.name = k.name;
      ka.help = k.help;
      ka.userdefined = userdefined;
      if (has_master) {
        ka.isalias = true;
        ka.master_key = master;
      }

      auto r = keys.insert (KeyBinding (ka, NULL));

      if (!r.second) {
        if (!r.first->first.userdefined && ka.userdefined) {
          /* default key, removing and replacing with user defined. target of
           * default key will be unreachable.
           */
          ustring wrr = ustring::compose (
              "key alias: %1 (%2) already exists in map with name: %3, overwriting.",
              k.str (), k.name, r.first->first.name);

          LOG (warn) << wrr;

          keys.erase (r.first);
          r = keys.insert (KeyBinding (k, t));

          if (has_master) has_aliases = true;
          has_master = true;
          auto s = keys.find (k);
          master = &(s->first);

        } else if (r.first->first.userdefined && k.userdefined) {
          ustring err = ustring::compose (
              "key alias: %1 (%2) is already user-configured in map with name: %3",
              k.str (), k.name, r.first->first.name);

          LOG (error) << err;

          throw duplicatekey_error (err.c_str());
        } else {
          ustring err = ustring::compose (
              "key alias: %1 (%2) is user-configured in map with name: %3, will try other aliases.",
              k.str (), k.name, r.first->first.name);

          LOG (warn) << err;
        }
      } else {
        if (has_master) has_aliases = true;
        has_master = true;
        auto s = keys.find (k);
        master = &(s->first);
      }
    }

    /* we dont know if any of the master key or any of the aliases
     * got successfully added */
    if (has_master) {
      std::map<Key, std::function<bool (Key)>>::iterator s = keys.find (k);
      s->first.hasaliases = has_aliases;
    }
  }

  void Keybindings::register_run (ustring name,
      std::function<bool (Key, ustring)> cb) {

    /* name will be look up in user targets, cb will be
     * called with target cmd string */
    auto b = user_run_bindings.begin ();

    while (b = std::find_if (b, user_run_bindings.end (),
          [&](std::pair<Key, ustring> p) {
            return p.first.name == name;
          }),
        b != user_run_bindings.end ()) {

      /* b is now a matching binding */
      LOG (info) << "ky: run, binding: " << name << "(" << b->first.str () << ") (userdefined: " << b->first.userdefined << ") to: " << b->second;

      register_key (b->first,
                    b->first.name,
                    ustring::compose ("Run hook: %1", b->second),
                    bind (cb, _1, b->second)
                    );

      b++;
    }
  }

  bool Keybindings::handle (GdkEventKey * event) {

    if (keys.empty ()) return false;

    Key ek (event);
    auto s = keys.find (ek);

    if (s != keys.end ()) {
      if (loghandle)
        LOG (debug) << "ky: " << title << ", handling: " << s->first.str () << " (" << s->first.name << ")";

      if (s->first.isalias) {
        auto m = keys.find (*(s->first.master_key));
        return m->second (s->first);
      } else {
        return s->second (s->first);
      }
    } else {
      if (loghandle)
        LOG (debug) << "ky: " << title << ",  unknown key: " << ek.str ();
    }

    return false;
  }

  // }}}

  /* Key {{{ */
  Key::Key () { }

  guint Key::get_keyval (ustring k) {
    if (k.size () == 1) {
      return gdk_unicode_to_keyval (k[0]);
    } else {
      /* check if key is in map */
      UstringUtils::trim (k);
      const char *k_str = k.c_str ();
      guint kk = gdk_keyval_from_name (k_str);

      return kk;
    }
  }

  Key::Key (ustring spec, ustring _n, ustring _h) { // {{{
    /* generate key from spec like:
     * k      : for key 'k'
     * C-k    : for Ctrl-k
     * M-k    : for Alt-k
     * C-M-K  : for Ctrl-Alt-K
     * K      : for Shift-k
     *
     */

    name = _n;
    help = _h;

    vector<ustring> spec_parts = VectorUtils::split_and_trim (spec, "-");

    if (spec_parts.size () > 3) {
      LOG (error) << "key spec invalid: " << spec;
      throw keyspec_error ("invalid length of spec");
    }

    if (spec_parts.size () == 1) {
      ctrl = false;
      meta = false;

      key = get_keyval (spec_parts[0]);

      return;
    }

    if (spec_parts.size () >= 2) {
      /* one modifier */
      char M = spec_parts[0][0];
      if (!((M == 'C') || (M == 'M'))) {
        LOG (error) << "key spec invalid: " << spec;
        throw keyspec_error ("invalid modifier in key spec");
      }

      if (M == 'C') ctrl = true;
      if (M == 'M') meta = true;

      if (spec_parts.size () == 2) {
        key = get_keyval (spec_parts[1]);
        return;
      }
    }

    if (spec_parts.size () >= 3) {
      char M = spec_parts[1][0];

      if (!((M == 'C') || (M == 'M'))) {
        LOG (error) << "key spec invalid: " << spec;
        throw keyspec_error ("invalid modifier in key spec");
      }

      if (M == 'C') {
        if (ctrl) {
          LOG (error) << "key spec invalid: " << spec;
          throw keyspec_error ("modifier already specified");
        }
        ctrl = true;
      }
      if (M == 'M') {
        if (meta) {
          LOG (error) << "key spec invalid: " << spec;
          throw keyspec_error ("modifier already specified");
        }
        meta = true;
      }

      key = get_keyval (spec_parts[2]);
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
        ustring kk = gdk_keyval_name (key);
        s += kk;
      } catch (std::exception &ex) {
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

  UnboundKey::UnboundKey () {
    unbound = true;
    userdefined = false;
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

