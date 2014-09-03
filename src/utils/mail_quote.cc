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

    int current_quote_level = 0;
    int quote_level = 0;

    bool in_newline = false;

    bool in_quote_prefix = false;
    int last_quote_start = 0;

    ustring start_quote = "<blockquote>";
    ustring stop_quote  = "</blockquote>";

    for (int i = 0; i < static_cast<int>(body.size()); i++) {

      if (body[i] == ' ') continue; // skip spaces

      if (body.compare (i, 4, quote) == 0) {
        log << debug << "mq: found quote, level: " << quote_level << endl;

        if (in_quote_prefix) {
          quote_level++;
        } else {
          in_quote_prefix = true;
          quote_level = 1;
          last_quote_start = i;
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

        } else {
          /* other text */

        }
      }
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

