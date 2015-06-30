# include <iostream>
# include <vector>
# include <algorithm>

# include "log.hh"
# include "action.hh"
# include "db.hh"

# include "tag_action.hh"
# include "toggle_action.hh"


using namespace std;

namespace Astroid {
  ToggleAction::ToggleAction (refptr<NotmuchThread> nmt, ustring _toggle_tag)
  : TagAction(nmt)
  {
    toggle_tag = _toggle_tag;
  }

  ToggleAction::ToggleAction (vector<refptr<NotmuchThread>> nmts, ustring _toggle_tag)
  : TagAction(nmts)
  {
    toggle_tag = _toggle_tag;
  }

  bool ToggleAction::doit (Db * db) {
    bool res = true;

    for (auto &thread : threads) {
      log << info << "tag_action: " << thread->thread_id << ", add: ";

      if (find (thread->tags.begin(), thread->tags.end(), toggle_tag) != thread->tags.end ()) {
        remove.push_back (toggle_tag);
      } else {
        add.push_back (toggle_tag);
      }

      log << info << "tag_action: " << thread->thread_id << ", add: ";
      for_each (add.begin(),
                add.end(),
                [&](ustring t) {
                  log << t << ", ";
                });

      log << "remove: ";
      for_each (remove.begin(),
                remove.end(),
                [&](ustring t) {
                  log << t << ", ";
                });

      log << endl;


      for_each (add.begin(),
                add.end(),
                [&](ustring t) {
                  res &= thread->add_tag (db, t);
                });

      for_each (remove.begin(),
                remove.end(),
                [&](ustring t) {
                  res &= thread->remove_tag (db, t);
                });


      remove.clear ();
      add.clear ();
    }

    return res;
  }

  SpamAction::SpamAction (refptr<NotmuchThread> nmt)
    : ToggleAction (nmt, "spam") {
    }

  SpamAction::SpamAction (vector<refptr<NotmuchThread>> nmts)
    : ToggleAction (nmts, "spam") {
    }

  MuteAction::MuteAction (refptr<NotmuchThread> nmt)
    : ToggleAction (nmt, "muted") {
    }

  MuteAction::MuteAction (vector<refptr<NotmuchThread>> nmts)
    : ToggleAction (nmts, "muted") {
    }
}
