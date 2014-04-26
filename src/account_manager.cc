# include <iostream>
# include <vector>
# include <algorithm>

# include "astroid.hh"
# include "account_manager.hh"
# include "config.hh"

using namespace std;

namespace Astroid {
  void AccountManager::initialize () {
    cout << "ac: initializing accounts.." << endl;

  }

  void AccountManager::deinitialize () {
    cout << "ac: deinitializing." << endl;

  }
}

