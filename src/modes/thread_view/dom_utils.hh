# pragma once

# include <string>
# include <gio/gio.h>
# include <webkit/webkit.h>

# include "proto.hh"

namespace Astroid {
  class DomUtils {
    public:
      static std::string assemble_data_uri (ustring, gchar *&, gsize);

      static WebKitDOMHTMLElement * make_message_div (WebKitWebView *);

      /* webkit dom utils */
      static WebKitDOMHTMLElement * clone_select (
          WebKitDOMNode * node,
          ustring         selector,
          bool            deep = true);

      static WebKitDOMHTMLElement * clone_node (
          WebKitDOMNode * node,
          bool            deep = true);

      static WebKitDOMHTMLElement * select (
          WebKitDOMNode * node,
          ustring         selector);
  };
}

