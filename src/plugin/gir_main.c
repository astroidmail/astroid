/*
 * Dummy C program to hand to g-ir-scanner for finding the
 * Activatable classes.
 *
 */

// compatibility with girepository-1.0 and 2.0
#if __has_include(<girepository.h>) // girepository-1.0
  #include <girepository.h>
  #define gi_repository_get_option_group g_irepository_get_option_group
#else
  #include <girepository/girepository.h>
#endif

# include "astroid_activatable.h"

int main (int argc, char ** argv) {

  GOptionContext *ctx;
  GError *error = NULL;

  ctx = g_option_context_new (NULL);
  g_option_context_add_group (ctx, gi_repository_get_option_group ());

  if (!g_option_context_parse (ctx, &argc, &argv, &error)) {
    g_print ("astroid girmain: %s\n", error->message);
    return 1;
  }

  return 0;
}

