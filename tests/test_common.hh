# pragma once

# include "astroid.hh"
# include "proto.hh"

using Astroid::astroid;
using Astroid::ustring;

// for logging
# define test trace

void setup () {
  astroid = Astroid::Astroid::create ();
  astroid->main_test ();
}

void teardown () {
  Astroid::astroid.clear ();
}

