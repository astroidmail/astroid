/* Incomplete list of functionality that needs to be implemented:
 *
 * Messages:
 *  update_tags (change rendered list of tags)
 *  update_css_tags (change corresponding CSS tags) can go together with the one above
 *
 *
 *
 */

function testJs() {
  document.write ('hello');
}

function add_message (message) {
  /* message is an object with the following members:
   *
   * author.name
   * author.email
   *
   * to:  [ { to1.name, to1.email }, .. ]
   * cc:  ...
   * bcc: ...
   *
   * subject (string)
   * tags    (list)
   *
   * preview (string)
   * body    (html string)
   *
   *
   */

}

function update_tags (message) {

  update_css_tags (message);
}

function update_css_tags (message) {

}

