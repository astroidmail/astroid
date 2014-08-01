# include <iostream>
# include <algorithm>
# include <vector>

# include "db.hh"
# include "paned_mode.hh"
# include "main_window.hh"
# include "thread_index.hh"
# include "thread_index_list_view.hh"
# include "thread_index_list_cell_renderer.hh"
# include "reply_message.hh"
# include "message_thread.hh"
# include "utils/utils.hh"

# include "command_bar.hh"

/* actions */
# include "actions/tag_action.hh"
# include "actions/toggle_action.hh"

using namespace std;

namespace Astroid {
  /* ----------
   * scrolled window
   * ----------
   */

  ThreadIndexScrolled::ThreadIndexScrolled (
      Glib::RefPtr<ThreadIndexListStore> _list_store,
      ThreadIndexListView * _list_view) {

    list_store = _list_store;
    list_view  = _list_view;

    tab_widget = new Gtk::Label ("None");

    scroll.add (*list_view);
    pack_start (scroll, true, true, 0);
    scroll.show_all ();
  }

  void ThreadIndexScrolled::grab_modal () {
    list_view->add_modal_grab ();
    list_view->grab_focus ();
  }

  void ThreadIndexScrolled::release_modal () {
    list_view->remove_modal_grab ();
  }

  /* ----------
   * list store
   * ----------
   */
  ThreadIndexListStore::ThreadIndexListStoreColumnRecord::ThreadIndexListStoreColumnRecord () {
    add (thread_id);
    add (thread);
  }

  ThreadIndexListStore::ThreadIndexListStore () {
    set_column_types (columns);
  }


  /* ---------
   * list view
   * ---------
   */
  ThreadIndexListView::ThreadIndexListView (ThreadIndex * _thread_index, Glib::RefPtr<ThreadIndexListStore> store) {
    thread_index = _thread_index;
    main_window  = _thread_index->main_window;
    list_store = store;

    config = astroid->config->config.get_child ("thread_index");
    open_paned_default = config.get<bool>("open_default_paned");

    set_model (list_store);
    set_enable_search (false);

    set_show_expanders (false);
    set_headers_visible (false);
    set_grid_lines (Gtk::TreeViewGridLines::TREE_VIEW_GRID_LINES_NONE);

    //append_column ("Thread IDs", list_store->columns.thread_id);

    /* add thread column */
    ThreadIndexListCellRenderer * renderer =
      Gtk::manage ( new ThreadIndexListCellRenderer () );
    int cols_count = append_column ("Thread", *renderer);
    Gtk::TreeViewColumn * column = get_column (cols_count - 1);

    column->set_cell_data_func (*renderer,
        sigc::mem_fun(*this, &ThreadIndexListView::set_thread_data) );
  }

  void ThreadIndexListView::set_thread_data (
      Gtk::CellRenderer * renderer,
      const Gtk::TreeIter &iter) {

    ThreadIndexListCellRenderer * r =
      (ThreadIndexListCellRenderer*) renderer;

    //cout << "setting thread.." << r <<  endl;
    if (iter) {

      Gtk::ListStore::Row row = *iter;
      r->thread = row[list_store->columns.thread];

    }
  }

  bool ThreadIndexListView::on_key_press_event (GdkEventKey *event) {
    switch (event->keyval) {
      case GDK_KEY_j:
      case GDK_KEY_Down:
        {
          if (list_store->children().size() < 2)
            return true;

          Gtk::TreePath path;
          Gtk::TreeViewColumn *c;
          get_cursor (path, c);

          path.next ();
          Gtk::TreeIter it = list_store->get_iter (path);

          if (it) {
            set_cursor (path);
          } else {
            /* try to load more threads */
            // TODO: async and lock
            thread_index->load_more_threads ();

            // retry to move down
            it = list_store->get_iter (path);
            if (it) set_cursor (path);
          }

          return true;
        }
        break;

      case GDK_KEY_k:
      case GDK_KEY_Up:
        {
          Gtk::TreePath path;
          Gtk::TreeViewColumn *c;
          get_cursor (path, c);
          path.prev ();
          if (path) {
            set_cursor (path);
          }
          return true;
        }
        break;

      case GDK_KEY_J:
        {
          auto adj = get_vadjustment ();
          adj->set_value (adj->get_value() + adj->get_step_increment ());
          return true;
        }

      case GDK_KEY_K:
        {
          auto adj = get_vadjustment ();
          adj->set_value (adj->get_value() - adj->get_step_increment ());
          return true;
        }

      case GDK_KEY_Home:
      case GDK_KEY_1:
        {
          /* select first */
          set_cursor (Gtk::TreePath("0"));
          return true;
        }

      case GDK_KEY_End:
      case GDK_KEY_0:
        {
          /* select last */
          auto it = list_store->children().end ();
          auto p  = list_store->get_path (--it);
          set_cursor (p);

          return true;
        }


      case GDK_KEY_Return:
        {
          auto thread = get_current_thread ();

          if (thread) {
            cout << "ti_list: loading: " << thread->thread_id << endl;

            if (event->state & GDK_SHIFT_MASK) {
              /* open message in new tab (if so configured) */
              thread_index->open_thread (thread, open_paned_default);
            } else {
              /* open message in split pane (if so configured) */
              thread_index->open_thread (thread, !open_paned_default);
            }
          }
          return true;
        }

      /* reply */
      case GDK_KEY_r:
        {
          auto thread = get_current_thread ();
          if (thread) {

            MessageThread mthread (thread);
            mthread.load_messages ();

            /* reply to last message */
            main_window->add_mode (new ReplyMessage (main_window, *(--mthread.messages.end())));

          }
          return true;
        }

      /* load more threads */
      case GDK_KEY_M:
        {
          thread_index->load_more_threads ();
          return true;
        }

      /* load all threads */
      case GDK_KEY_exclam:
        {
          thread_index->load_more_threads (true);
          return true;
        }

      /* toggle archived */
      case GDK_KEY_a:
        {
          auto thread = get_current_thread ();
          if (thread) {

            main_window->actions.doit (refptr<Action>(new ToggleAction(thread, "inbox")));

            /* update row */
            update_current_row ();
          }

          return true;
        }

      /* toggle starred */
      case GDK_KEY_asterisk:
        {
          auto thread = get_current_thread ();
          if (thread) {

            main_window->actions.doit (refptr<Action>(new ToggleAction(thread, "starred")));

            /* update row */
            update_current_row ();
          }

          return true;
        }

      /* toggle unread */
      case GDK_KEY_N:
        {
          auto thread = get_current_thread ();
          if (thread) {

            main_window->actions.doit (refptr<Action>(new ToggleAction(thread, "unread")));

            /* update row */
            update_current_row ();
          }

          return true;
        }

      /* toggle spam */
      case GDK_KEY_S:
        {
          auto thread = get_current_thread ();
          if (thread) {

            main_window->actions.doit (refptr<Action>(new SpamAction(thread)));

            /* update row */
            update_current_row ();
          }

          return true;
        }

      case GDK_KEY_l:
        {
          auto thread = get_current_thread ();
          if (thread) {
            ustring tag_list = VectorUtils::concat_tags (thread->tags);

            main_window->enable_command (CommandBar::CommandMode::Tag,
                tag_list,
                [&,thread](ustring tgs) {
                  cout << "ti: got tags: " << tgs << endl;

                  vector<ustring> tags = VectorUtils::split_and_trim (tgs, ",");

                  sort (tags.begin (), tags.end ());
                  sort (thread->tags.begin (), thread->tags.end ());

                  vector<ustring> rem;
                  vector<ustring> add;

                  /* find tags that have been removed */
                  set_difference (thread->tags.begin (),
                                  thread->tags.end (),
                                  tags.begin (),
                                  tags.end (),
                                  std::back_inserter (rem));

                  /* find tags that should be added */
                  set_difference (tags.begin (),
                                  tags.end (),
                                  thread->tags.begin (),
                                  thread->tags.end (),
                                  std::back_inserter (add));


                  if (add.size () == 0 &&
                      rem.size () == 0) {
                    cout << "ti: nothing to do." << endl;
                  } else {
                    main_window->actions.doit (
                       refptr<Action>(new TagAction (thread, add, rem)));
                  }
                });
          }
        }
        return true;
    }

    return false;
  }

  void ThreadIndexListView::update_current_row () {
    Gtk::TreePath path;
    Gtk::TreeViewColumn *c;
    get_cursor (path, c);
    Gtk::TreeIter iter;

    iter = list_store->get_iter (path);

    if (iter) {
      list_store->row_changed (path, iter);
    }

  }

  refptr<NotmuchThread> ThreadIndexListView::get_current_thread () {
    if (list_store->children().size() < 1)
      return refptr<NotmuchThread>();

    Gtk::TreePath path;
    Gtk::TreeViewColumn *c;
    get_cursor (path, c);
    Gtk::TreeIter iter;

    iter = list_store->get_iter (path);

    if (iter) {
      Gtk::ListStore::Row row = *iter;
      return row[list_store->columns.thread];

    } else {
      return refptr<NotmuchThread>();
    }
  }

  ustring ThreadIndexListView::get_current_thread_id () {
    if (list_store->children().size() < 1)
      return "";

    Gtk::TreePath path;
    Gtk::TreeViewColumn *c;
    get_cursor (path, c);
    Gtk::TreeIter iter;

    iter = list_store->get_iter (path);

    if (iter) {
      Gtk::ListStore::Row row = *iter;
      ustring thread_id = row[list_store->columns.thread_id];

      return thread_id;

    } else {
      return "";
    }
  }
}

