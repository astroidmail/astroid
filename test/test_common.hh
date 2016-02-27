# pragma once

# include "astroid.hh"
# include "log.hh"
# include "proto.hh"

using Astroid::astroid;
using Astroid::error;
using Astroid::info;
using Astroid::warn;
using Astroid::debug;
using Astroid::test;
using std::endl;

void setup () {
  astroid = new Astroid::Astroid ();
  astroid->main_test ();
}

void teardown () {
  delete Astroid::astroid;
}

