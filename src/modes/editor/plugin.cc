# include <string>

# include <gtkmm/socket.h>

# include "plugin.hh"
# include "modes/edit_message.hh"
# include "log.hh"

using std::endl;

namespace Astroid {
  Plugin::Plugin (EditMessage * _em, ustring _server) : server_name (_server) {
    em = _em;

    log << debug << "em: editor server name: " << server_name << endl;

    /* editor settings */
    editor_cmd  = em->editor_config.get <std::string>("cmd");

    /* gtk::socket:
     * http://stackoverflow.com/questions/13359699/pyside-embed-vim
     * https://developer.gnome.org/gtkmm-tutorial/stable/sec-plugs-sockets-example.html.en
     * https://mail.gnome.org/archives/gtk-list/2011-January/msg00041.html
     */
    editor_socket = Gtk::manage(new Gtk::Socket ());
    editor_socket->set_can_focus (true);
    editor_socket->signal_plug_added ().connect (
        sigc::mem_fun(*this, &Plugin::plug_added) );
    editor_socket->signal_plug_removed ().connect (
        sigc::mem_fun(*this, &Plugin::plug_removed) );

    editor_socket->signal_realize ().connect (
        sigc::mem_fun(*this, &Plugin::socket_realized) );

    add (*editor_socket);
  }

  Plugin::~Plugin () {

  }

  bool Plugin::ready () {
    return editor_ready;
  }

  bool Plugin::started () {
    return editor_started;
  }

  void Plugin::start () {
    if (socket_ready) {
      /* the editor gets the following args:
       *
       * %1: file name
       * %2: server name
       * %3: socket id (or parent id)
       *
       */

      ustring cmd = ustring::compose (editor_cmd,
          em->tmpfile_path.c_str (),
          server_name,
          editor_socket->get_id ());

      log << info << "em: starting editor: " << cmd << endl;
      Glib::spawn_command_line_async (cmd.c_str());
      editor_started = true;
    } else {
      start_editor_when_ready = true; // TODO: not thread-safe
      log << debug << "em: editor, waiting for socket.." << endl;
    }

  }

  void Plugin::focus () {
    grab_focus ();
    if (!editor_focused) {
      child_focus (Gtk::DIR_TAB_FORWARD);
      editor_focused = true;
    }
  }

  void Plugin::stop () {
    log << error << "editor: don't know how to stop editor!" << endl;
  }

  void Plugin::socket_realized ()
  {
    log << debug << "em: socket realized." << endl;
    socket_ready = true;

    if (start_editor_when_ready) {
      em->editor_toggle (true);
      start_editor_when_ready = false;
    }
  }

  void Plugin::plug_added () {
    log << debug << "em: editor connected" << endl;

    editor_ready = true;
    em->activate_editor ();
  }

  bool Plugin::plug_removed () {
    log << debug << "em: editor disconnected" << endl;
    editor_ready   = false;
    editor_started = false;
    editor_focused = false;
    em->editor_toggle (false);

    return true;
  }

}


