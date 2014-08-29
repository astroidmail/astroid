/* based mostly on:
 *
 * https://github.com/Manu343726/Cpp11CustomLogClass/blob/master/Log.h
 * http://stackoverflow.com/questions/17595957/operator-overloading-in-c-for-logging-purposes/17605306#17605306
 */

# pragma once

# include "modes/log_view.hh"

# include <type_traits>
# include <iostream>
# include <chrono>
# include <ctime>
# include <string>
# include <sstream>
# include <vector>

using namespace std;

namespace Astroid {
  class Log {
    public:

      enum Level {
        debug,
        info,
        warn,
        error,
      };

    private:
      bool         _next_is_begin;
      enum Level   _next_level;

      stringstream _next_line;

      vector <ostream *> out_streams;
      vector <LogView *> log_views;


      // this is the key: std::endl is a template function, and this is the signature of that function (For std::ostream).
      using endl_type = std::ostream&(std::ostream&);


    public:

      // constructor: User passes a custom log header and output stream, or uses defaults.
      Log();

      Log& operator<<(endl_type endl);

      // log level
      Log& operator<<(Level lvl) {
        _next_level = lvl;
        return *this;
      }

      string level_string (Level lvl) {
        switch (lvl) {
          case debug: return "debug";
          case info:  return "info";
          case warn:  return "warn";
          case error: return "error";
          default:    return "unknown error level!";
        }
      }

      //Overload for anything else:
      template<typename T>
      Log& operator<< (const T& data)
      {
          auto now        = std::chrono::system_clock::now();
          auto now_time_t = std::chrono::system_clock::to_time_t( now );
          auto now_tm     = std::localtime( &now_time_t );

          if( _next_is_begin ) {
            _next_line << "[" << level_string (_next_level) << "] " << "(" << now_tm->tm_hour << ":" << now_tm->tm_min << ":" << now_tm->tm_sec << "): " << data;
          } else {
            _next_line << data;
          }

          _next_is_begin = false;

          return *this;
      }

  /*
      // Overload for std::endl only:
      Log& operator<< (endl_type endl);

      // Overload for level only:
      Log& operator<< (Level lvl);
      string level_string (Level lvl);

      // Overload for anything else:
      template<typename T>
      Log& operator<< (const T& data);

      */
      void add_out_stream (ostream *);
      void del_out_stream (ostream *);
      void add_log_view   (LogView *);
      void del_log_view   (LogView *);
  };


}
