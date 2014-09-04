# include <iostream>

# include "log.hh"

# include "mail_quote.hh"

using namespace std;

namespace Astroid {

  void MailQuotes::filter_quotes (ustring &body)
  {
    /* run through html body and wrap quoted parts of the body with
     * quote divs */

    log << debug << "mq: filter quotes: " << body << endl;

    ustring quote = "&gt;";
    ustring newline = "<br>";

    int current_quote_level = 0;
    int quote_level = 0;

    bool in_newline = false;

    bool in_quote_prefix = false;
    int last_quote_start = 0;

    bool drop_quote = true;

    ustring start_quote = "<blockquote>";
    ustring stop_quote  = "</blockquote>";

    for (int i = 0; i < static_cast<int>(body.length()); i++) {
      log << debug << "mq: i = " << i << endl;

      if (body[i] == ' ') continue; // skip spaces
      if (body[i] == '\n') continue; // skip newlines

      bool new_line_now = false;

      /* look for newlines */
      if (body.compare (i, 4, newline) == 0) {
        log << debug << "mq: found newline." << endl;
        i += 3;
        if (in_newline) {
          /* double newline means end of all quotes */
          i++;

          log << debug << "mq: double new line, closing quotes: " << current_quote_level << endl;
          for (; current_quote_level > 0; current_quote_level--) {
            log << debug << "mq: closing blockquote." << endl;
            body.insert (i, stop_quote);
            i += stop_quote.length();
          }

          new_line_now = true;

        } else {
          in_newline = true;
          new_line_now = true;
        }
      }

      if (body.compare (i, 4, quote) == 0) {
        log << debug << "mq: found quote, level: " << quote_level << endl;

        if (in_quote_prefix) {
          quote_level++;
        } else {
          if (in_newline) {
            log << debug << "mq: first after new line, starting counting." << endl;
            /* only start if are after a new line */
            in_quote_prefix = true;
            quote_level = 1;
            last_quote_start = i;
            in_newline = false;
          } else {
            log << debug << "mq: not in new line, skipping." << endl;
          }
        }

        i += 3; // jump past this quote

      } else {

        if (in_quote_prefix) {
          /* start or close quote */
          if (quote_level > current_quote_level) {
            current_quote_level++;
            body.insert (last_quote_start, start_quote);

            i += start_quote.length() + 4;

          } else if (quote_level < current_quote_level) {
            current_quote_level--;
            if (new_line_now) i++;
            body.insert (i, stop_quote);

          }

          in_quote_prefix = false;
          quote_level = 0;
          in_newline = false;

        } else {
          /* other text */
          if (!new_line_now) in_newline = false;

        }
      }
    }

    log << debug << "mq: closing all blockquotes." << endl;
    for (; current_quote_level > 0; current_quote_level--) {
      log << debug << "mq: closing blockquote." << endl;
      body.insert (body.length()-1, stop_quote);
    }
  }

  ustring::iterator MailQuotes::insert_string (
      ustring &to,
      ustring::iterator &to_start,
      const ustring &src )
  {
    for (auto it = src.begin();
         it != src.end(); it++) {
      to_start = to.insert (to_start, *it);
    }

    return to_start;
  }

}

