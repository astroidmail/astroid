# include "proto.hh"

# include "mode.hh"

namespace Astroid {
  class RawMessage : public Mode {
    public:
      RawMessage (MainWindow *);
      RawMessage (MainWindow *, refptr<Message>);
      RawMessage (MainWindow *, const char *);

      refptr<Message> msg;

      void grab_modal () override;
      void release_modal () override;

    private:
      Gtk::ScrolledWindow scroll;
      Gtk::TextView       tv;
  };
}

