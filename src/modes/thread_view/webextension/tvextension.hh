# pragma once

# include <webkit2/webkit-web-extension.h>
# include <gmodule.h>

extern "C" {

static void
web_page_created_callback (WebKitWebExtension *extension,
                           WebKitWebPage      *web_page,
                           gpointer            user_data);

G_MODULE_EXPORT void
webkit_web_extension_initialize_with_user_data (WebKitWebExtension *extension, gpointer pipes);


}

class AstroidExtension {
  public:
    AstroidExtension (WebKitWebExtension *, gpointer);

    void page_created (WebKitWebExtension *, WebKitWebPage *, gpointer);

  private:
    WebKitWebExtension * extension;
    WebKitWebPage * page;

    /* pipes to main astroid process */
    gint32 ext_read, ext_write;
};

AstroidExtension * ext;

