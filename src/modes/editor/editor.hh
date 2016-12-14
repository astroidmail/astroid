# pragma once

# include <gtkmm.h>

namespace Astroid {
  class Editor {
    public:

      virtual bool ready () = 0;
      virtual bool started () = 0;

      virtual void start () = 0;
      virtual void stop () = 0;

      virtual void focus () = 0;

      bool start_editor_when_ready = false;

  };
}

