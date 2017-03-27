# pragma once

# include "editor.hh"
# include "proto.hh"

namespace Astroid {
  class External : public Editor, public sigc::trackable {
    friend EditMessage;

    public:
      External (EditMessage * em);
      ~External ();

      bool ready () override;
      bool started () override;
      void start () override;
      void stop () override;

      void focus () override;

    protected:
      EditMessage * em;


      /* editor config */
      std::string editor_cmd;
      std::string editor_args;

      bool editor_ready = false;
      bool editor_started = false;

      int pid;
      int stdin;
      int stdout;
      int stderr;
      refptr<Glib::IOChannel> ch_stdout;
      refptr<Glib::IOChannel> ch_stderr;

      sigc::connection out_c;
      sigc::connection err_c;
      bool log_out (Glib::IOCondition);
      bool log_err (Glib::IOCondition);
      void child_watch (GPid, int);

      void on_stop ();

      refptr<Gio::File> draft_file;
      refptr<Gio::FileMonitor> draft_watch;
      void on_draft_changed (const Glib::RefPtr<Gio::File>& file, const Glib::RefPtr<Gio::File>& other_file, Gio::FileMonitorEvent event_type);
  };
}

