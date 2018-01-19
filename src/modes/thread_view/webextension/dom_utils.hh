# pragma once

# include <string>
# include <vector>
# include <gio/gio.h>
# include <webkit2/webkit-web-extension.h>

# include "proto.hh"

using std::vector;

namespace Astroid {
  class DomUtils {
    public:
      static std::string assemble_data_uri (ustring, gchar *&, gsize);

      static WebKitDOMHTMLElement * make_message_div (WebKitDOMDocument *);

      /* webkit dom utils */
      static WebKitDOMHTMLElement * clone_get_by_id (
          WebKitDOMNode * node,
          ustring         id,
          bool            deep = true);

      static WebKitDOMHTMLElement * clone_select_by_classes (
          WebKitDOMNode * node,
          vector<ustring> classes,
          bool            deep = true);

      static WebKitDOMHTMLElement * clone_node (
          WebKitDOMNode * node,
          bool            deep = true);

      static WebKitDOMHTMLElement * select_by_classes (
          WebKitDOMNode * n,
          vector<ustring> classes);

      static WebKitDOMNode * search_by_classes (
          WebKitDOMHTMLCollection * c,
          vector<ustring> classes);

      static WebKitDOMElement * get_by_id (
          WebKitDOMDocument * n, ustring id);

      static WebKitDOMElement * get_by_id (
          WebKitDOMNode * n, ustring id);
  };
}

