# include "astroid.hh"

int main (int argc, char **argv) {
  Astroid::astroid = Astroid::Astroid::create ();
  return Astroid::astroid->run (argc, argv);
}

