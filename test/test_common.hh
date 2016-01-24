# pragma once

# include <string>

# include "astroid.hh"
# include "log.hh"
# include "proto.hh"
# include "config.hh"

struct TestAstroid : Astroid::Astroid {
  void set_database_path (const std::string& path) {
    m_config->notmuch_config.put ("db", path);
  }
};

// ~Astroid() is not virtual, so keep an object for proper destruction
static TestAstroid* test_astroid;

void setup () {
  test_astroid = new TestAstroid ();
  Astroid::astroid = test_astroid;
  test_astroid->main_test ();
}

void teardown () {
  delete test_astroid;
}

