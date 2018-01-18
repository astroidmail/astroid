# pragma once

# include <string>
# include <gio/gio.h>
# include <webkit2/webkit-web-extension.h>

# include "proto.hh"

namespace Astroid {
  class DomUtils {
    public:
      static std::string assemble_data_uri (ustring, gchar *&, gsize);

      static WebKitDOMHTMLElement * make_message_div (WebKitWebPage *);

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

      static WebKitDOMElement * get_by_id (
          WebKitDOMDocument * n, ustring id);
  };
}

