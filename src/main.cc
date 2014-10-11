# include "astroid.hh"

int main (int argc, char **argv) {
  Astroid::astroid = new Astroid::Astroid ();
  return Astroid::astroid->main (argc, argv);
}

