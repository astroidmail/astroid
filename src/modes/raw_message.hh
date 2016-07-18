# include <boost/filesystem.hpp>

# include "proto.hh"
# include "mode.hh"

namespace bfs = boost::filesystem;

namespace Astroid {
  class RawMessage : public Mode {
    public:
      RawMessage (MainWindow *);
      RawMessage (MainWindow *, refptr<Message>);
      RawMessage (MainWindow *, const char *, bool delete_on_close = false);
      ~RawMessage ();

      refptr<Message> msg;

      void grab_modal () override;
      void release_modal () override;

    private:
      bool delete_on_close = false;
      bfs::path fname;

      Gtk::ScrolledWindow scroll;
      Gtk::TextView       tv;
  };
}

