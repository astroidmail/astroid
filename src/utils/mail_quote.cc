# include <iostream>

# include "log.hh"

# include "mail_quote.hh"

using namespace std;

namespace Astroid {

  void MailQuotes::filter_quotes (ustring &body)
  {
    /* run through html body and wrap quoted parts of the body with
     * quote divs */

    ustring quote = "&gt;";
    ustring newline = "<br>";

    int current_quote_level = 0;
    int quote_level = 0;

    bool in_newline = false;

    bool in_quote_prefix = false;
    int last_quote_start = 0;

    ustring start_quote = "<blockquote>";
    ustring stop_quote  = "</blockquote>";

    for (int i = 0; i < static_cast<int>(body.length()); i++) {
      if (body[i] == ' ') continue; // skip spaces
      if (body[i] == '\n') continue; // skip newlines

      bool new_line_now = false;

      /* look for newlines */
      if (body.compare (i, 4, newline) == 0) {
        i += 4;
        if (in_newline) {
          /* double newline means end of all quotes */

          for (; current_quote_level > 0; current_quote_level--) {
            body.insert (i, stop_quote);
            i += stop_quote.length();
          }


        } else {
          in_newline = true;
        }

        new_line_now = true;
      }

      if (body.compare (i, 4, quote) == 0) {
        if (in_quote_prefix) {
          quote_level++;
        } else {
          if (in_newline) {
            /* only start if are after a new line */
            in_quote_prefix = true;
            quote_level = 1;
            last_quote_start = i;
            in_newline = false;
          } else {
            /* quote not after newline, skipping */
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

    /* close up all remaining quotes */
    for (; current_quote_level > 0; current_quote_level--) {
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

