# include "tvextension.hh"

# include <webkit2/webkit-web-extension.h>
# include <gmodule.h>
# include <iostream>
# include <glib.h>

extern "C" {

static void
web_page_created_callback (WebKitWebExtension *extension,
                           WebKitWebPage      *web_page,
                           gpointer            user_data )
{
  ext->page_created (extension, web_page, user_data);
}

G_MODULE_EXPORT void
webkit_web_extension_initialize_with_user_data (
    WebKitWebExtension *extension,
    gpointer pipes)
{
  ext = new AstroidExtension (extension, pipes);

  g_signal_connect (extension, "page-created",
      G_CALLBACK (web_page_created_callback),
      NULL);

}
}

AstroidExtension::AstroidExtension (WebKitWebExtension * e,
    gpointer pipes) {
  extension = e;

  std::cout << "ae: inititalize" << std::endl;

  /* retrieve pipes */
  std::cout << "type: " << g_variant_get_type_string ((GVariant *)pipes) << std::endl;

  GVariant * ext_read_v  = g_variant_get_child_value ((GVariant *) pipes, 0);
  GVariant * ext_write_v = g_variant_get_child_value ((GVariant *) pipes, 1);

  ext_read  = g_variant_get_int32 (ext_read_v);
  ext_write = g_variant_get_int32 (ext_write_v);

  std::cout << "ae: init done" << std::endl;
}

void AstroidExtension::page_created (WebKitWebExtension * /* extension */,
    WebKitWebPage * _page,
    gpointer /* user_data */) {

  page = _page;

  std::cout << "ae: page created" << std::endl;

  g_print ("Page %d created for %s\n",
           (int) webkit_web_page_get_id (page),
           webkit_web_page_get_uri (page));
}

