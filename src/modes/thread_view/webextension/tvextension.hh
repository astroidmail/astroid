# pragma once

# include <webkit2/webkit-web-extension.h>
# include <gmodule.h>

extern "C" {

static void
web_page_created_callback (WebKitWebExtension *extension,
                           WebKitWebPage      *web_page,
                           gpointer            user_data);

G_MODULE_EXPORT void
webkit_web_extension_initialize (WebKitWebExtension *extension);

}

