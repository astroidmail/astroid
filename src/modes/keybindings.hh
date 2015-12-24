# pragma once

# include <map>
# include <functional>

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

    ustring name;
    ustring help;

    bool hasaliases;
    bool isalias; /* this is an alias for another master key */
    Key  * master_key;

    bool operator== ( const Key & other ) const;
    bool operator<  ( const Key & other ) const;

    ustring str ();

    static Key create (string spec);
  };

  class Keybindings {
    public:
      Keybindings ();

      typedef pair<Key, function<bool (Key)>> KeyBinding;
      void register_key (Key, ustring name, ustring help, function<bool (Key)>);
      void register_key (ustring spec, ustring name, ustring help, function<bool (Key)>);

      void register_key (Key, vector<Key>, ustring name, ustring help, function<bool (Key)>);
      void register_key (ustring spec, vector<ustring>, ustring name, ustring help, function<bool (Key)>);

      void register_key (Key, vector<ustring>, ustring name, ustring help, function<bool (Key)>);
      void register_key (ustring spec, vector<Key>, ustring name, ustring help, function<bool (Key)>);

      bool handle (GdkEventKey *);

    private:
      map<Key, function<bool (Key)>> keys;
  };
}

