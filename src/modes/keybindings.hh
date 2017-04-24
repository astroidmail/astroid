# pragma once

# include <map>
# include <functional>
# include <atomic>
# include <gtkmm.h>
# include "proto.hh"


namespace Astroid {
  struct Key {
    Key ();
    Key (ustring spec, ustring name = "", ustring help = "");
    Key (guint, ustring name = "", ustring help = "");
    Key (bool _c, bool _m, guint k, ustring name = "", ustring help = "");
    Key (bool _c, bool _m, char k, ustring name = "", ustring help = "");
    Key (GdkEventKey *, ustring name = "", ustring help = "");

    /* only these three memebers define the logical state of the Key */
    bool ctrl = false;
    bool meta = false;
    guint key = 0; /* GDK_KEY_* */

    mutable ustring name = "";
    mutable ustring help = "";

    mutable bool unbound     = false;
    mutable bool userdefined = false;

    mutable bool allow_duplicate_name = false; /* used for run targets */

    mutable bool hasaliases = false; /* this is a master key with other aliases */
    mutable bool isalias    = false; /* this key is an alias for another master key */
    mutable const Key * master_key;

    bool operator== ( const Key & other ) const;
    bool operator<  ( const Key & other ) const;

    ustring str () const;

    static Key create (ustring spec);
    static guint get_keyval (ustring k);
  };

  /* unbound keys are used as targets that are not bound to a key
   * by default, but may be bound using a custom keybinding */
  struct UnboundKey : public Key {
    UnboundKey ();
  };

  /* exceptions */
  class keyspec_error : public std::runtime_error {
    public:
      keyspec_error (const char *);

  };

  class duplicatekey_error : public std::runtime_error {
    public:
      duplicatekey_error (const char *);

  };

  class Keybindings {
    public:
      Keybindings ();
      static void init ();

      void set_prefix (ustring title, ustring prefix);

      ustring title; /* title of keybinding set */
      bool loghandle = false; /* log handling */

      typedef std::pair<Key, std::function<bool (Key)>> KeyBinding;

      void register_key (Key, ustring name, ustring help, std::function<bool (Key)>);
      void register_key (ustring spec, ustring name, ustring help, std::function<bool (Key)>);

      void register_key (Key,
                         std::vector<Key>,
                         ustring name,
                         ustring help,
                         std::function<bool (Key)>);

      void register_key (ustring spec,
                         std::vector<ustring>,
                         ustring name,
                         ustring help,
                         std::function<bool (Key)>);

      void register_key (Key,
                         std::vector<ustring>,
                         ustring name,
                         ustring help,
                         std::function<bool (Key)>);

      void register_key (ustring spec,
                         std::vector<Key>,
                         ustring name,
                         ustring help,
                         std::function<bool (Key)>);

      void register_run (ustring name,
                         std::function<bool (Key, ustring)>);

      bool handle (GdkEventKey *);
      bool handle (ustring name);

      void clear ();

      ustring short_help ();
      ustring help ();

    private:
      std::map<Key, std::function<bool (Key)>> keys;
      ustring prefix = "";

      static std::vector<Key>  user_bindings;
      static std::vector<std::pair<Key, ustring>> user_run_bindings;

      static std::atomic<bool> user_bindings_loaded;
      static const char * user_bindings_file;
  };
}

