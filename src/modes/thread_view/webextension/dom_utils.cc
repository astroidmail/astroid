# include <webkit2/webkit-web-extension.h>

# include <string>
# include <gio/gio.h>
# include <iostream>

# include "dom_utils.hh"

using std::cout;
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
        "email_template");
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

    cout << "looking for: " << selector << endl;

    if (WEBKIT_DOM_IS_DOCUMENT(node)) {
      WebKitDOMElement * b = WEBKIT_DOM_ELEMENT (webkit_dom_document_get_body (WEBKIT_DOM_DOCUMENT(node)));

      e = select (WEBKIT_DOM_NODE(b), selector);
      g_object_unref (b);
      return e;
    }

    /* ..DOMElement */
    WebKitDOMHTMLCollection * es = webkit_dom_element_get_children (WEBKIT_DOM_ELEMENT(node));
    for (unsigned int i = 0; i < webkit_dom_html_collection_get_length (es); i++) {
      WebKitDOMNode * n = webkit_dom_html_collection_item (es, i);
      e = WEBKIT_DOM_HTML_ELEMENT (n);

      cout << webkit_dom_element_get_id (WEBKIT_DOM_ELEMENT(e)) << endl;

      if (webkit_dom_element_webkit_matches_selector (WEBKIT_DOM_ELEMENT(n), selector.c_str (), &gerr)) {
        cout << "MATCH" << endl;
        break;
      }

      e = NULL;
      g_object_unref (n);
    }
    g_object_unref (es);

    if (gerr != NULL)
      std::cout << "ae: dom-utils: clone_s_s_err: " << gerr->message << std::endl;

    return e;
  }

  WebKitDOMElement * DomUtils::get_by_id (WebKitDOMDocument * d, ustring id) {
    WebKitDOMElement * en;

    WebKitDOMElement * b = WEBKIT_DOM_ELEMENT (webkit_dom_document_get_body (WEBKIT_DOM_DOCUMENT(d)));

    WebKitDOMHTMLCollection * es = webkit_dom_element_get_children (WEBKIT_DOM_ELEMENT(d));

    for (unsigned int i = 0; i < webkit_dom_html_collection_get_length (es); i++) {
      WebKitDOMNode * n = webkit_dom_html_collection_item (es, i);

      en = WEBKIT_DOM_ELEMENT (n);

      if (ustring(webkit_dom_element_get_id (en)) == id) break;

      g_object_unref (n);
      en = NULL;
    }

    g_object_unref (es);
    g_object_unref (b);

    return en;
  }

}

