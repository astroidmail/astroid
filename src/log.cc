# include <iostream>
# include <iomanip>
# include <chrono>
# include <ctime>

# include "astroid.hh"
# include "log.hh"
# include "modes/log_view.hh"

using namespace std;

namespace Astroid {

  Log::Log ()
  {
    _next_level = debug;
  }

  //Overload for std::endl only:
  Log& Log::operator<<(endl_type endl)
  {
      auto now        = std::chrono::system_clock::now();
      auto now_time_t = std::chrono::system_clock::to_time_t( now );
      auto now_tm     = std::localtime( &now_time_t );

      stringstream time_str;
      time_str << "("
           << setw(2) << setfill('0') << now_tm->tm_hour << ":"
           << setw(2) << setfill('0') << now_tm->tm_min << ":"
           << setw(2) << setfill('0') << now_tm->tm_sec << ")";


      for (auto &o : out_streams) {
        *o << "[" << level_string (_next_level) << "] "
           << time_str.str () << ": "
           << _next_line.str () << endl;
      }

      for (auto &lv : log_views) {
        lv->log_line (_next_level, time_str.str(),  _next_line.str());
      }

      _next_line.str (string());
      _next_line.clear ();

      return *this;
  }

  Log& Log::operator<<(endl_type endl);

  // log level
  Log& Log::operator<<(LogLevel lvl) {
    _next_level = lvl;
    return *this;
  }

  ustring Log::level_string (LogLevel lvl) {
    switch (lvl) {
      case debug: return "debug";
      case info:  return "info ";
      case warn:  return "warn ";
      case error: return "error";
      default:    return "unknown error level!";
    }
  }

  void Log::add_out_stream (ostream *o) {
    out_streams.push_back (o);
  }

  void Log::del_out_stream (ostream *o) {
    out_streams.erase (std::remove (out_streams.begin (), out_streams.end (), o), out_streams.end ());
  }

  void Log::add_log_view (LogView * lv) {
    log_views.push_back (lv);
  }

  void Log::del_log_view (LogView * lv) {
    log_views.erase (std::remove (log_views.begin (), log_views.end (), lv), log_views.end ());
  }
}

