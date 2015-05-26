# include <iostream>
# include <algorithm>
# include <vector>

# include "db.hh"
# include "log.hh"
# include "paned_mode.hh"
# include "main_window.hh"
# include "thread_index.hh"
# include "thread_index_list_view.hh"
# include "thread_index_list_cell_renderer.hh"
# include "reply_message.hh"
# include "forward_message.hh"
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
      MainWindow *mw,
      Glib::RefPtr<ThreadIndexListStore> _list_store,
      ThreadIndexListView * _list_view) : Mode (mw) {

    list_store = _list_store;
    list_view  = Gtk::manage(_list_view);

    scroll.add (*list_view);
    pack_start (scroll, true, true, 0);
    scroll.show_all ();
  }

  ThreadIndexScrolled::~ThreadIndexScrolled () {
    log << debug << "tis: deconstruct." << endl;
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
    add (newest_date);
    add (thread_id);
    add (thread);
  }

  ThreadIndexListStore::ThreadIndexListStore () {
    set_column_types (columns);
    set_sort_column (0, Gtk::SortType::SORT_DESCENDING);
  }

  ThreadIndexListStore::~ThreadIndexListStore () {
    log << debug << "tils: deconstuct." << endl;
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
    page_jump_rows     = config.get<int>("page_jump_rows");

    set_model (list_store);
    set_enable_search (false);

    set_show_expanders (false);
    set_headers_visible (false);
    set_grid_lines (Gtk::TreeViewGridLines::TREE_VIEW_GRID_LINES_NONE);

    //append_column ("Thread IDs", list_store->columns.thread_id);

    /* add thread column */
    renderer =
      Gtk::manage ( new ThreadIndexListCellRenderer () );
    int cols_count = append_column ("Thread", *renderer);
    Gtk::TreeViewColumn * column = get_column (cols_count - 1);

    column->set_cell_data_func (*renderer,
        sigc::mem_fun(this, &ThreadIndexListView::set_thread_data) );

    astroid->global_actions->signal_thread_updated ().connect (
        sigc::mem_fun (this, &ThreadIndexListView::on_thread_updated));

    astroid->global_actions->signal_refreshed ().connect (
        sigc::mem_fun (this, &ThreadIndexListView::on_refreshed));

    /* mouse click */
    signal_row_activated ().connect (
        sigc::mem_fun (this, &ThreadIndexListView::on_my_row_activated));

    /* re-draw every minute (check every second) */
    Glib::signal_timeout ().connect (
        sigc::mem_fun (this, &ThreadIndexListView::redraw), 1000);
  }

  ThreadIndexListView::~ThreadIndexListView () {
    log << debug << "tilv: deconstruct." << endl;
  }

  bool ThreadIndexListView::redraw () {
    chrono::duration<double> elapsed = chrono::steady_clock::now() - last_redraw;

    if (elapsed.count () >= 60) {

      queue_draw ();
      last_redraw = chrono::steady_clock::now();

    }

    return true;
  }


  void ThreadIndexListView::set_thread_data (
      Gtk::CellRenderer * renderer,
      const Gtk::TreeIter &iter) {

    ThreadIndexListCellRenderer * r =
      (ThreadIndexListCellRenderer*) renderer;

    //log << debug << "setting thread.." << r <<  endl;
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
          if (list_store->children().size() >= 2) {

            Gtk::TreePath path;
            Gtk::TreeViewColumn *c;
            get_cursor (path, c);

            for (int i = 0; i < page_jump_rows; i++) {
              if (!path) break;
              path.next ();
            }

            Gtk::TreeIter it = list_store->get_iter (path);

            if (it) {
              set_cursor (path);
            } else {
              /* try to load more threads */
              // TODO: async and lock
              thread_index->load_more_threads ();

              // retry to move down
              it = list_store->get_iter (path);
              if (it) {
                set_cursor (path);
              } else {
                /* move to last */
                auto it = list_store->children().end ();
                auto p  = list_store->get_path (--it);
                if (p) set_cursor (p);
              }
            }
          }

          return true;
        }

      case GDK_KEY_K:
        {
          Gtk::TreePath path;
          Gtk::TreeViewColumn *c;
          get_cursor (path, c);
          for (int i = 0; i < page_jump_rows; i++) {
            if (!path) break;
            path.prev ();
          }

          if (path) {
            set_cursor (path);
          } else {
            /* move to first */
            auto p = Gtk::TreePath("0");
            if (p) set_cursor (p);
          }

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
          if (list_store->children().size() >= 1) {
            auto it = list_store->children().end ();
            auto p  = list_store->get_path (--it);
            if (p) set_cursor (p);
          }

          return true;
        }


      case GDK_KEY_Return:
        {
          auto thread = get_current_thread ();

          if (thread) {
            log << info << "ti_list: loading: " << thread->thread_id << endl;

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
            Db db (Db::DbMode::DATABASE_READ_ONLY);

            mthread.load_messages (&db);

            /* reply to last message */
            main_window->add_mode (new ReplyMessage (main_window, *(--mthread.messages.end())));

          }
          return true;
        }

      /* reply */
      case GDK_KEY_G:
        {
          auto thread = get_current_thread ();
          if (thread) {

            MessageThread mthread (thread);
            Db db (Db::DbMode::DATABASE_READ_ONLY);

            mthread.load_messages (&db);

            /* reply to last message */
            main_window->add_mode (new ReplyMessage (main_window, *(--mthread.messages.end()), ReplyMessage::ReplyMode::Rep_All));

          }
          return true;
        }

      /* forward */
      case GDK_KEY_f:
        {
          auto thread = get_current_thread ();
          if (thread) {

            MessageThread mthread (thread);
            Db db (Db::DbMode::DATABASE_READ_ONLY);

            mthread.load_messages (&db);

            /* reply to last message */
            main_window->add_mode (new ForwardMessage (main_window, *(--mthread.messages.end())));

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
            Db db (Db::DbMode::DATABASE_READ_WRITE);
            main_window->actions.doit (&db, refptr<Action>(new ToggleAction(thread, "inbox")));
          }

          return true;
        }

      /* toggle flagged */
      case GDK_KEY_asterisk:
        {
          auto thread = get_current_thread ();
          if (thread) {

            Db db (Db::DbMode::DATABASE_READ_WRITE);
            main_window->actions.doit (&db, refptr<Action>(new ToggleAction(thread, "flagged")));
          }

          return true;
        }

      /* toggle unread */
      case GDK_KEY_N:
        {
          auto thread = get_current_thread ();
          if (thread) {

            Db db (Db::DbMode::DATABASE_READ_WRITE);
            main_window->actions.doit (&db, refptr<Action>(new ToggleAction(thread, "unread")));
          }

          return true;
        }

      /* toggle spam */
      case GDK_KEY_S:
        {
          auto thread = get_current_thread ();
          if (thread) {

            Db db (Db::DbMode::DATABASE_READ_WRITE);
            main_window->actions.doit (&db, refptr<Action>(new SpamAction(thread)));
          }

          return true;
        }

      /* toggle muted: C-m */
      case GDK_KEY_m:
        {
          if ((event->state & GDK_CONTROL_MASK)) {
            auto thread = get_current_thread ();
            if (thread) {

              Db db (Db::DbMode::DATABASE_READ_WRITE);
              main_window->actions.doit (&db, refptr<Action>(new MuteAction(thread)));
            }

            return true;
          } else {
            return false;
          }
        }

      case GDK_KEY_l:
        {
          /* edit tags */
          auto thread = get_current_thread ();
          if (thread) {
            ustring tag_list = VectorUtils::concat_tags (thread->tags) + ", ";

            main_window->enable_command (CommandBar::CommandMode::Tag,
                tag_list,
                [&,thread](ustring tgs) {
                  log << debug << "ti: got tags: " << tgs << endl;

                  vector<ustring> tags = VectorUtils::split_and_trim (tgs, ",");

                  /* remove empty */
                  tags.erase (std::remove (tags.begin (), tags.end (), ""), tags.end ());

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
                    log << debug << "ti: nothing to do." << endl;
                  } else {
                    Db db (Db::DbMode::DATABASE_READ_WRITE);
                    main_window->actions.doit (&db,
                       refptr<Action>(new TagAction (thread, add, rem)));
                  }
                });
          }
        }
        return true;
    }

    return false;
  }

  ModeHelpInfo * ThreadIndexListView::key_help () {
    ModeHelpInfo * m = new ModeHelpInfo ();

    m->parent   = NULL;
    m->toplevel = false;
    m->title    = "Thread Index List View";

    m->keys = {
      { "j,Down", "Next line" },
      { "k,Up", "Previous line" },
      { "J,K", "Scroll view" },
      { "1,Home", "Go to first line" },
      { "0,End", "Go to last line" },
      { "Return", "Open thread (default action)" },
      { "S+Return", "Open thread in pane (default action)" },
      { "r", "Reply to last message in thread" },
      { "G", "Reply all to last message in thread" },
      { "f", "Forward last message in thread" },
      { "M", "Load more threads in query" },
      { "!", "Load all threads in query" },
      { "a", "Toggle 'inbox' tag on thread" },
      { "*", "Toggle 'flagged' tag on thread" },
      { "N", "Toggle 'unread' tag on thread" },
      { "S", "Toggle 'spam' tag on thread" },
      { "C-m", "Toggle 'muted' tag on thread, it will be excluded from searches." },
      { "l", "Edit tags for thread" },
    };

    return m;
  }

  void ThreadIndexListView::on_my_row_activated (
      const Gtk::TreeModel::Path & path,
      Gtk::TreeViewColumn * column) {

    auto thread = get_current_thread ();

    if (thread) {
      log << info << "ti_list: loading: " << thread->thread_id << endl;

      /* open message in new tab (if so configured) */
      thread_index->open_thread (thread, !open_paned_default);
    }
  }

  void ThreadIndexListView::on_refreshed () {
    log << debug << "til: got refreshed signal." << endl;
    thread_index->refresh (false, max(thread_index->thread_load_step, thread_index->current_thread), false);
  }

  void ThreadIndexListView::on_thread_updated (Db * db, ustring thread_id) {
    log << info << "til: got updated thread signal: " << thread_id << endl;

    /* we now have three options:
     * - a new thread has been added (unlikely)
     * - a thread has been deleted (kind of likely)
     * - a thread has been updated (most likely)
     *
     * none of them needs to affect the threads that match the query in this
     * list.
     *
     */

    time_t t0 = clock ();

    Gtk::TreePath path;
    Gtk::TreeIter fwditer;

    /* forward iterating is much faster than going backwards:
     * https://developer.gnome.org/gtkmm/3.11/classGtk_1_1TreeIter.html
     */

    bool found = false;
    fwditer = list_store->get_iter ("0");

    Gtk::ListStore::Row row;

    while (fwditer) {

      row = *fwditer;

      if (row[list_store->columns.thread_id] == thread_id) {
        found = true;
        break;
      }

      fwditer++;
    }

    /* test if thread is in the current query */
    lock_guard<Db> grd (*db);
    bool in_query = db->thread_in_query (thread_index->query_string, thread_id);

    if (found) {
      /* thread has either been updated or deleted from current query */
      log << debug << "til: updated: found thread in: " << ((clock() - t0) * 1000.0 / CLOCKS_PER_SEC) << " ms." << endl;

      if (in_query) {
        /* updated */
        log << debug << "til: updated" << endl;
        refptr<NotmuchThread> thread = row[list_store->columns.thread];
        thread->refresh (db);
        row[list_store->columns.newest_date] = thread->newest_date;

      } else {
        /* deleted */
        log << debug << "til: deleted" << endl;
        path = list_store->get_path (fwditer);
        list_store->erase (fwditer);
      }

    } else {
      /* thread has possibly been added to the current query */
      log << debug << "til: updated: did not find thread, time used: " << ((clock() - t0) * 1000.0 / CLOCKS_PER_SEC) << " ms." << endl;
      if (in_query) {
        log << debug << "til: new thread for query, adding.." << endl;
        auto iter = list_store->prepend ();
        Gtk::ListStore::Row newrow = *iter;

        NotmuchThread * t;

        db->on_thread (thread_id, [&](notmuch_thread_t *nmt) {

            t = new NotmuchThread (nmt);

          });

        newrow[list_store->columns.newest_date] = t->newest_date;
        newrow[list_store->columns.thread_id]   = t->thread_id;
        newrow[list_store->columns.thread]      = Glib::RefPtr<NotmuchThread>(t);

        /* check if we should select it (if this is the only item) */
        if (list_store->children().size() == 1) {
          auto p = Gtk::TreePath("0");
          if (p) set_cursor (p);
        }
      }
    }

    thread_index->refresh_stats (db);
  }

  /*
  void ThreadIndexListView::update_current_row (Db &db) {
    Gtk::TreePath path;
    Gtk::TreeViewColumn *c;
    get_cursor (path, c);
    Gtk::TreeIter iter;

    iter = list_store->get_iter (path);

    if (iter) {
      Gtk::ListStore::Row row = *iter;

      refptr<NotmuchThread> thread = row[list_store->columns.thread];
      thread->refresh (&db);

      list_store->row_changed (path, iter);
    }

  }
  */

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

