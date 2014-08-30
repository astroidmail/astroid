/* based partly on:
 *
 * https://github.com/Manu343726/Cpp11CustomLogClass/blob/master/Log.h
 * http://stackoverflow.com/questions/17595957/operator-overloading-in-c-for-logging-purposes/17605306#17605306
 */

# pragma once

# include "proto.hh"

# include <iostream>
# include <sstream>
# include <vector>

using namespace std;

namespace Astroid {
  enum LogLevel {
    debug,
    info,
    warn,
    error,
  };

  class Log {
    public:


    private:
      bool          _next_is_begin;
      enum LogLevel _next_level;
      stringstream  _next_line;

      vector <ostream *> out_streams;
      vector <LogView *> log_views;


      // this is the key: std::endl is a template function, and this is the signature of that function (For std::ostream).
      using endl_type = std::ostream&(std::ostream&);


    public:

      // constructor: User passes a custom log header and output stream, or uses defaults.
      Log();

      // Overload for std::endl only:
      Log& operator<< (endl_type endl);

      // Overload for level only:
      Log& operator<< (LogLevel lvl);
      static ustring level_string (LogLevel lvl);

      // Overload for anything else, this must be defined
      // in the header file so that the implementation using it
      // have access to the definition of the template.
      template<typename T>
      Log& operator<< (const T& data)
      {
          _next_line << data;

          return *this;
      }

      void add_out_stream (ostream *);
      void del_out_stream (ostream *);
      void add_log_view   (LogView *);
      void del_log_view   (LogView *);
  };

  /* globally available log instance */
  extern Log log;

}
