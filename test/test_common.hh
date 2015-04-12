# pragma once

# include "astroid.hh"
# include "log.hh"

using namespace Astroid;

void setup () {
  Astroid::astroid = new Astroid::Astroid ();
  astroid->main_test ();
}

void teardown () {
  delete Astroid::astroid;
}

