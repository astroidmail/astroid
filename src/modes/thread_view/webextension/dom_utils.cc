# include <webkit2/webkit-web-extension.h>

# include <string>
# include <gio/gio.h>
# include <iostream>

# include "dom_utils.hh"

using std::endl;

namespace Astroid {
  std::string DomUtils::assemble_data_uri (ustring mime_type, gchar * &data, gsize len) {

    std::string base64 = "data:" + mime_type + ";base64," + Glib::Base64::encode (std::string(data, len));

    return base64;
  }

  /* clone and create html elements */
  WebKitDOMHTMLElement * DomUtils::make_message_div (WebKitWebPage * webpage) {
    /* clone div from template in html file */
    WebKitDOMDocument *d = webkit_web_page_get_dom_document (webpage);
    WebKitDOMHTMLElement *e = clone_select (WEBKIT_DOM_NODE(d),
        "#email_template");
    g_object_unref (d);
    return e;
  }

  WebKitDOMHTMLElement * DomUtils::clone_select (
      WebKitDOMNode * node,
      ustring         selector,
      bool            deep) {

    return clone_node (WEBKIT_DOM_NODE(select (node, selector)), deep);
  }

  WebKitDOMHTMLElement * DomUtils::clone_node (
      WebKitDOMNode * node,
      bool            deep) {

    GError * gerr = NULL;
    return WEBKIT_DOM_HTML_ELEMENT(webkit_dom_node_clone_node_with_error (node, deep, &gerr));
  }

  WebKitDOMHTMLElement * DomUtils::select (
      WebKitDOMNode * node,
      ustring         selector) {

    GError * gerr = NULL;
    WebKitDOMHTMLElement *e;

    if (WEBKIT_DOM_IS_DOCUMENT(node)) {
      e = WEBKIT_DOM_HTML_ELEMENT(
        webkit_dom_document_query_selector (WEBKIT_DOM_DOCUMENT(node),
                                            selector.c_str(),
                                            &gerr));
    } else {
      /* ..DOMElement */
      e = WEBKIT_DOM_HTML_ELEMENT(
        webkit_dom_element_query_selector (WEBKIT_DOM_ELEMENT(node),
                                            selector.c_str(),
                                            &gerr));
    }

    if (gerr != NULL)
      std::cout << "ae: dom-utils: clone_s_s_err: " << gerr->message << std::endl;

    return e;
  }

}

