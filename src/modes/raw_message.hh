# include "proto.hh"
# include "astroid.hh"

# include "mode.hh"

using namespace std;

namespace Astroid {
  class RawMessage : public Mode {
    public:
      RawMessage (refptr<Message>);

      refptr<Message> msg;

      void grab_modal () override;
      void release_modal () override;

    private:
      Gtk::ScrolledWindow scroll;
      Gtk::TextView       tv;


    protected:
      virtual bool on_key_press_event (GdkEventKey *) override;
  };
}

