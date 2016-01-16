# pragma once

# include "astroid.hh"
# include "log.hh"
# include "proto.hh"

void setup () {
  using Astroid::astroid;
  astroid = new Astroid::Astroid ();
  astroid->main_test ();
}

void teardown () {
  delete Astroid::astroid;
}

