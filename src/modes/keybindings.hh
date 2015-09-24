# pragma once

# include <map>
# include <functional>

using namespace std;

namespace Astroid {
  struct Key {
    Key ();
    Key (bool _c, bool _m, char k);

    bool ctrl = false;
    bool meta = false;

    char key  = '0';

    bool operator== ( const Key & other ) const;
    bool operator<  ( const Key & other ) const;

    static Key create (string spec);
  };

  class Keybindings {
    public:
      Keybindings ();

      typedef pair<Key, function<void(Key)>> KeyBinding;
      void register_key (Key, function<void(Key)>);

    private:
      map<Key, function<void(Key)>> keys;
  };
}

