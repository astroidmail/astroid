# pragma once

# include <gtkmm/socket.h>

# include "editor.hh"
# include "proto.hh"

namespace Astroid {
  class External : public Editor {
    friend EditMessage;

    public:
      External (EditMessage * em);
      ~External ();

      /* bool ready () override; */
      /* bool started () override; */
      /* void start () override; */
      /* void stop () override; */

      /* void focus () override; */

    protected:
      EditMessage * em;

      /* editor config */
      std::string editor_cmd;
      std::string editor_args;
  };
}

