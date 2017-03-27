# pragma once

# ifndef DISABLE_EMBEDDED

# include <gtkmm/socket.h>

# include "editor.hh"
# include "proto.hh"

namespace Astroid {
  class Plugin : public Editor {
    friend EditMessage;

    public:
      Plugin (EditMessage * em, ustring server);
      ~Plugin ();

      bool ready () override;
      bool started () override;
      void start () override;
      void stop () override;

      void focus () override;

      Gtk::Box bin;

    protected:
      EditMessage * em;

      void plug_added ();
      bool plug_removed ();

      Gtk::Socket * editor_socket;
      void socket_realized ();
      bool socket_ready = false;

      bool editor_ready = false;
      bool editor_started = false;

      bool editor_focused = false;

      /* socket and server */
      ustring server_name;

      /* editor config */
      std::string editor_cmd;
      std::string editor_args;
  };
}

# endif

