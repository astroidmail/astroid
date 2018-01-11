# include "tvextension.hh"

# include <webkit2/webkit-web-extension.h>
# include <gmodule.h>

extern "C" {

static void
web_page_created_callback (WebKitWebExtension *extension,
                           WebKitWebPage      *web_page,
                           gpointer            user_data)
{
    g_print ("Page %d created for %s\n",
             (int) webkit_web_page_get_id (web_page),
             webkit_web_page_get_uri (web_page));
}

G_MODULE_EXPORT void
webkit_web_extension_initialize (WebKitWebExtension *extension)
{
    g_signal_connect (extension, "page-created",
                      G_CALLBACK (web_page_created_callback),
                      NULL);
}

}

