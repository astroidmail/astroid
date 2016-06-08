/*
 * Dummy C program to hand to g-ir-scanner for finding the
 * Activatable classes.
 *
 */

# include <girepository.h>

# include "astroid_activatable.h"

int main (int argc, char ** argv) {

  GOptionContext *ctx;
  GError *error = NULL;

  ctx = g_option_context_new (NULL);
  g_option_context_add_group (ctx, g_irepository_get_option_group ());

  if (!g_option_context_parse (ctx, &argc, &argv, &error)) {
    g_print ("astroid girmain: %s\n", error->message);
    return 1;
  }

  return 0;
}

