# pragma once

# include "astroid.hh"
# include "proto.hh"
# include <boost/log/core.hpp>

namespace logging = boost::log;

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
  logging::core::get()->remove_all_sinks ();
}

