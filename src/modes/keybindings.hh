# pragma once

# include <map>
# include <functional>
# include <atomic>

using namespace std;

namespace Astroid {
  struct Key {
    Key ();
    Key (bool _c, bool _m, guint k, ustring name = "", ustring help = "");
    Key (bool _c, bool _m, char k, ustring name = "", ustring help = "");
    Key (GdkEventKey *, ustring name = "", ustring help = "");

    bool ctrl = false;
    bool meta = false;
    guint key = 0; /* GDK_KEY_* */

    ustring name = "";
    ustring help = "";

    bool hasaliases = false; /* this is a master key with other aliases */
    bool isalias    = false; /* this key is an alias for another master key */
    const Key * master_key;

    bool operator== ( const Key & other ) const;
    bool operator<  ( const Key & other ) const;

    ustring str () const;

    static Key create (string spec);
  };

  /* exceptions */
  class keyspec_error : public runtime_error {
    public:
      keyspec_error (const char *);

  };

  class Keybindings {
    public:
      Keybindings ();
      static void init ();

      typedef pair<Key, function<bool (Key)>> KeyBinding;

      void register_key (Key, ustring name, ustring help, function<bool (Key)>);
      void register_key (ustring spec, ustring name, ustring help, function<bool (Key)>);

      void register_key (Key,
                         vector<Key>,
                         ustring name,
                         ustring help,
                         function<bool (Key)>);

      void register_key (ustring spec,
                         vector<ustring>,
                         ustring name,
                         ustring help,
                         function<bool (Key)>);

      void register_key (Key,
                         vector<ustring>,
                         ustring name,
                         ustring help,
                         function<bool (Key)>);

      void register_key (ustring spec,
                         vector<Key>,
                         ustring name,
                         ustring help,
                         function<bool (Key)>);

      bool handle (GdkEventKey *);

      void clear ();

      ustring short_help ();
      ustring help ();

    private:
      map<Key, function<bool (Key)>> keys;

    public:
      static vector<Key>  user_bindings;
      static atomic<bool> user_bindings_loaded;
      static const char * user_bindings_file;

      static map<guint, ustring> keynames;
  };
}

