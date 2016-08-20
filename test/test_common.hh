# pragma once

# include "astroid.hh"
# include "proto.hh"

using Astroid::astroid;
using Astroid::ustring;

// for logging
# define test trace

void setup () {
  astroid = new Astroid::Astroid ();
  astroid->main_test ();
}

void teardown () {
  delete Astroid::astroid;
}

