# include <vector>
# include <string>

# include "external.hh"

# include "modes/edit_message.hh"

using std::endl;

namespace Astroid {
  External::External (EditMessage * _em) {
    em = _em;

    /* editor settings */
    editor_cmd  = em->editor_config.get <std::string>("cmd");
  }

  External::~External () {

  }

  bool External::ready () {
    return editor_ready;
  }

  bool External::started () {
    /* if the editor is started then the draft should be locked for sending */
    return editor_started;
  }

  void External::start () {
    editor_started = true;

    ustring cmd = ustring::compose (editor_cmd,
        em->tmpfile_path.c_str ());

    LOG (debug) << "em: ex: launching editor: " << cmd;

    /* std::vector<std::string> args = {cmd.c_str()}; */
    auto args = Glib::shell_parse_argv (cmd);

    try {
      Glib::spawn_async_with_pipes ("",
                        args,
                        Glib::SPAWN_SEARCH_PATH | Glib::SPAWN_DO_NOT_REAP_CHILD,
                        sigc::slot <void> (),
                        &pid,
                        &stdin,
                        &stdout,
                        &stderr
                        );
    } catch (Glib::SpawnError &ex) {
      LOG (error) << "em: ex: exception while running external editor: " <<  ex.what ();
      on_stop ();
      return;
    }

    /* connect channels */
    out_c = Glib::signal_io().connect (sigc::mem_fun (this, &External::log_out), stdout, Glib::IO_IN | Glib::IO_HUP);
    err_c = Glib::signal_io().connect (sigc::mem_fun (this, &External::log_err), stderr, Glib::IO_IN | Glib::IO_HUP);
    Glib::signal_child_watch().connect (sigc::mem_fun (this, &External::child_watch), pid);

    ch_stdout = Glib::IOChannel::create_from_fd (stdout);
    ch_stderr = Glib::IOChannel::create_from_fd (stderr);

    /* monitor file */
    GFile * f = g_file_new_for_path (em->tmpfile_path.c_str ());
    draft_file = Glib::wrap (f, true);
    draft_watch = draft_file->monitor ();
    draft_watch->signal_changed ().connect (sigc::mem_fun (this, &External::on_draft_changed));

    em->set_info ("Editing..");
  }

  void External::on_draft_changed (const Glib::RefPtr<Gio::File>&, const Glib::RefPtr<Gio::File>&, Gio::FileMonitorEvent event_type)
  {
    if (event_type == Gio::FileMonitorEvent::FILE_MONITOR_EVENT_CHANGES_DONE_HINT ||
        event_type == Gio::FileMonitorEvent::FILE_MONITOR_EVENT_CREATED)
    {
      if (bfs::exists (em->tmpfile_path)) {
        LOG (debug) << "em: ex: file changed, updating preview..";
        if (!em->in_read) {
          em->read_edited_message ();
          em->set_info ("Editing..");
        }
      }
    }
  }

  bool External::log_out (Glib::IOCondition cond) {
    if (cond == Glib::IO_HUP) {
      ch_stdout.clear();
      return false;
    }

    if ((cond & Glib::IO_IN) == 0) {
      LOG (error) << "em: ex: invalid fifo response";
    } else {
      Glib::ustring buf;

      ch_stdout->read_line(buf);
      if (*(--buf.end()) == '\n') buf.erase (--buf.end());

      LOG (debug) << "em: ex: " << buf;

    }
    return true;
  }

  bool External::log_err (Glib::IOCondition cond) {
    if (cond == Glib::IO_HUP) {
      ch_stderr.clear();
      return false;
    }

    if ((cond & Glib::IO_IN) == 0) {
      LOG (error) << "em: ex: invalid fifo response";
    } else {
      Glib::ustring buf;

      ch_stderr->read_line(buf);
      if (*(--buf.end()) == '\n') buf.erase (--buf.end());

      LOG (warn) << "em: ex: " << buf;
    }
    return true;
  }

  void External::child_watch (GPid pid, int child_status) {
    if (child_status != 0) {
      LOG (error) << "em: ex: editor did not exit successfully.";
    }

    draft_watch->cancel ();

    /* close process */
    Glib::spawn_close_pid (pid);


    out_c.disconnect();
    err_c.disconnect();

    on_stop ();
  }

  void External::on_stop () {
    editor_started = false;
    em->editor_toggle (false);
  }

  void External::stop () {
  }

  void External::focus () {
    /* no-op */
  }

}

