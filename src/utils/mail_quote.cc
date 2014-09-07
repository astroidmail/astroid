# include <iostream>
# include <iterator>

# include "log.hh"

# include "mail_quote.hh"

using namespace std;

namespace Astroid {

  void MailQuotes::filter_quotes (ustring &body)
  {
    /* run through html body and wrap quoted parts of the body with
     * quote divs */

    if (mq_verbose) log << debug << "mq: quote filtering.." << endl;
    time_t t0 = clock ();

    const ustring quote = "&gt;";
    const ustring newline = "<br>";

    bool delete_quote_markers = MailQuotes::delete_quote_markers;

    const ustring start_quote = "<blockquote>";
    const ustring stop_quote  = "</blockquote>";

    ustring::iterator iter;
    ustring::iterator last_quote_start;

    int current_quote_level = 0;

    unsigned int i = 0;
    while (i <= body.length ()) {
      if (mq_verbose) log << debug << "mq: iter (" << i << " of " << body.length() << ")" << endl;


      /* search through all lines (between <br> and <br>) and check
       * if a line starts with quotes. if it does, insert or decrease
       * blockquotes depending if the level is different from the previous.
       *
       * if two newlines come after earch other, close up the block quotes. */

      // i is now at the end of the previous line.

      /* skip spaces so that we can detect double new lines */
      if (isspace(body[i])) {
        if (mq_verbose) log << debug << "mq: found space, skipping" << endl;
        i++;
        continue;
      }

      /* search for the next new line */
      size_t line_end = body.find (newline.c_str(), i, newline.length ());

      if (line_end == ustring::npos) {
        if (mq_verbose) log << debug << "mq: no more new lines, closing up!" << endl;

        if (mq_verbose) log << warn << "mq: left over blockquotes: " << current_quote_level << endl;
        for (; current_quote_level > 0; current_quote_level--) {
          body.append (stop_quote);
        }

        break;
      }

      if (i == line_end) {
        /* double new lines */
        if (mq_verbose) log << debug << "mq: double new line" << endl;
        i += newline.length ();

        if (mq_verbose) log << warn << "mq: left over blockquotes: " << current_quote_level << endl;
        for (; current_quote_level > 0; current_quote_level--) {
          body.insert (i, stop_quote);
          i += stop_quote.length (); // i is now at end of blockquotes after last of double newline
        }

        continue;
      }

      line_end += newline.length ();

      /* we can now start searching for quote markers, ignoring spaces */
      if (mq_verbose) log << debug << "mq: searching between: " << i << " and " << line_end << endl;
      if (mq_verbose) log << body.substr (i, (line_end-i)) << endl;
      int quote_level = 0;
      int line_start = i;
      int line_left = line_end - i;
      while (i < line_end) {
        if (mq_verbose) log << debug << "mq: at " << i << " looking for quotes." << endl;
        if (isspace(body[i])) {
          if (mq_verbose) log << debug << "mq: found space, skipping" << endl;
          i++;
          line_left--;
          continue;
        }

        size_t q_start = body.find (quote.c_str (), i, quote.length());
        if (q_start == i) {
          quote_level++;
          if (mq_verbose) log << debug << "mq: found quote, level: " << quote_level << endl;
          if (delete_quote_markers) {
            body.erase (i, quote.length());
            line_left -= quote.length ();
            line_end  -= quote.length ();
          } else {
            i += quote.length ();
            line_left -= quote.length ();
          }

          continue;
        } else {
          if (mq_verbose) log << debug << "mq: did not find any more quotes, done working on line." << endl;
          // no more quotes
          break;
        }
      }

      if (mq_verbose) log << debug << "mq: quote level: " << quote_level << " (of " << current_quote_level << ")" << endl;

      if (quote_level > current_quote_level) {
        for (; quote_level > current_quote_level; current_quote_level++) {
          if (mq_verbose) log << debug << "mq: insert blockquote" << endl;
          body.insert (line_start, start_quote);
          i += start_quote.length();
        }

      } else if (quote_level < current_quote_level) {
        for (; quote_level < current_quote_level; current_quote_level--) {
          if (mq_verbose) log << debug << "mq: stop blockquote" << endl;
          body.insert (line_start, stop_quote);
          i += stop_quote.length();
        }
      }

      i += line_left;
    }

    if (current_quote_level > 0) {
      if (mq_verbose) log << warn << "mq: left over blockquotes: " << current_quote_level << endl;
      for (; current_quote_level > 0; current_quote_level--) {
        body.append (stop_quote);
      }
    }

    log << debug << "mq: quote filter done, time: " << ((clock() - t0) * 1000 / CLOCKS_PER_SEC) << " ms." << endl;
  }
}

