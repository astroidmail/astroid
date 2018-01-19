# include <webkit2/webkit-web-extension.h>

# include <string>
# include <vector>
# include <gio/gio.h>
# include <iostream>

# include "dom_utils.hh"

using std::cout;
using std::endl;
using std::vector;

namespace Astroid {
  std::string DomUtils::assemble_data_uri (ustring mime_type, gchar * &data, gsize len) {

    std::string base64 = "data:" + mime_type + ";base64," + Glib::Base64::encode (std::string(data, len));

    return base64;
  }

  /* clone and create html elements */
  WebKitDOMHTMLElement * DomUtils::make_message_div (WebKitDOMDocument * d) {
    /* clone div from template in html file */
    WebKitDOMHTMLElement * e = clone_node (WEBKIT_DOM_NODE(get_by_id (d, "email_template")));
    return e;
  }

  WebKitDOMHTMLElement * DomUtils::clone_get_by_id (
      WebKitDOMNode * node,
      ustring         id,
      bool            deep) {

    return clone_node (WEBKIT_DOM_NODE (get_by_id (node, id)), deep);
  }

  WebKitDOMHTMLElement * DomUtils::clone_select_by_classes (
      WebKitDOMNode * node,
      vector<ustring> classes,
      bool            deep) {
    return clone_node (WEBKIT_DOM_NODE (select_by_classes (node, classes)), deep);
  }

  WebKitDOMHTMLElement * DomUtils::clone_node (
      WebKitDOMNode * node,
      bool            deep) {

    GError * gerr = NULL;
    return WEBKIT_DOM_HTML_ELEMENT(webkit_dom_node_clone_node_with_error (node, deep, &gerr));
  }

  WebKitDOMHTMLElement * DomUtils::select_by_classes (WebKitDOMNode * node, vector<ustring> classes) {
    if (WEBKIT_DOM_IS_DOCUMENT(node)) {

      WebKitDOMHTMLCollection * c = webkit_dom_document_get_children (WEBKIT_DOM_DOCUMENT(node));

      WebKitDOMHTMLElement * e = WEBKIT_DOM_HTML_ELEMENT(search_by_classes (c, classes));
      g_object_unref (c);

      return e;

    } {

      /* WebKitDOMElement */
      WebKitDOMHTMLCollection * c = webkit_dom_element_get_children (WEBKIT_DOM_ELEMENT(node));

      WebKitDOMHTMLElement * e = WEBKIT_DOM_HTML_ELEMENT(search_by_classes (c, classes));
      g_object_unref (c);

      return e;
    }
  }

  WebKitDOMNode * DomUtils::search_by_classes (WebKitDOMHTMLCollection * c, vector<ustring> classes) {
    /* Search recursively for first element matching all classes, this method
     * probably doesn't match the query selector completely since it does not require the
     * class match to be exclusive. I.e. if the element has more classes than those
     * specified as criteria it will still match. */

    for (unsigned int i = 0; i < webkit_dom_html_collection_get_length (c); i++) {
      /* we will reduce this one as we ascend the tree */
      vector<ustring> local_classes = classes;

      WebKitDOMHTMLElement * e = WEBKIT_DOM_HTML_ELEMENT(webkit_dom_html_collection_item (c, i));

      /* check if this element matches */
      WebKitDOMDOMTokenList * cls = webkit_dom_element_get_class_list (WEBKIT_DOM_ELEMENT(e));

      for (unsigned j = 0; j < webkit_dom_dom_token_list_get_length(cls); j++) {
        ustring jc = ustring(webkit_dom_dom_token_list_item (cls, j));
        local_classes.erase (std::remove (local_classes.begin (), local_classes.end (), jc), local_classes.end());
      }
      g_object_unref (cls);

      if (local_classes.empty()) {
        return WEBKIT_DOM_NODE(e);
      } else {
        WebKitDOMHTMLCollection * ch = webkit_dom_element_get_children (WEBKIT_DOM_ELEMENT (e));
        g_object_unref (e);

        e = WEBKIT_DOM_HTML_ELEMENT(search_by_classes (ch, local_classes));
        g_object_unref (ch);

        if (e) return WEBKIT_DOM_NODE(e);
      }
    }

    return NULL; // length == 0
  }


  WebKitDOMElement * DomUtils::get_by_id (WebKitDOMDocument * d, ustring id) {
    WebKitDOMElement * en;

    WebKitDOMElement * b = WEBKIT_DOM_ELEMENT (webkit_dom_document_get_body (WEBKIT_DOM_DOCUMENT(d)));

    en = get_by_id (WEBKIT_DOM_NODE(b), id);
    g_object_unref (b);

    return en;
  }

  WebKitDOMElement * DomUtils::get_by_id (WebKitDOMNode * b, ustring id) {
    WebKitDOMElement * en;
    WebKitDOMHTMLCollection * es = webkit_dom_element_get_children (WEBKIT_DOM_ELEMENT(b));

    for (unsigned int i = 0; i < webkit_dom_html_collection_get_length (es); i++) {
      WebKitDOMNode * n = webkit_dom_html_collection_item (es, i);

      en = WEBKIT_DOM_ELEMENT (n);

      if (ustring(webkit_dom_element_get_id (en)) == id) break;

      g_object_unref (n);
      en = NULL;
    }

    g_object_unref (es);
    return en;
  }

}

