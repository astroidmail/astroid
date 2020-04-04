# include <boost/log/core.hpp>
# include "astroid.hh"

namespace logging = boost::log;

int main (int argc, char **argv) {
  Astroid::astroid = Astroid::Astroid::create ();
  int r = Astroid::astroid->run (argc, argv);
  logging::core::get()->remove_all_sinks ();
  return r;
}

