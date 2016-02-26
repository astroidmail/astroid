# include <iostream>
# include <algorithm>
# include <vector>
# include <functional>

# include "db.hh"
# include "log.hh"
# include "modes/paned_mode.hh"
# include "main_window.hh"
# include "thread_index.hh"
# include "thread_index_list_view.hh"
# include "thread_index_list_cell_renderer.hh"
# include "modes/keybindings.hh"
# include "modes/thread_view/thread_view.hh"
# include "modes/reply_message.hh"
# include "modes/forward_message.hh"
# include "message_thread.hh"
# include "utils/utils.hh"
# include "utils/cmd.hh"
# include "utils/resource.hh"

# include "command_bar.hh"

/* actions */
# include "actions/tag_action.hh"
# include "actions/toggle_action.hh"
# include "actions/difftag_action.hh"

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
    add (oldest_date);
    add (thread_id);
    add (thread);
    add (marked);
  }

  ThreadIndexListStore::ThreadIndexListStore () {
    set_column_types (columns);
  }

  ThreadIndexListStore::~ThreadIndexListStore () {
    log << debug << "tils: deconstuct." << endl;
  }


  /* ---------
   * list view
   * ---------
   */
  ThreadIndexListView::ThreadIndexListView (ThreadIndex * _thread_index, Glib::RefPtr<ThreadIndexListStore> store) {
    using bfs::path;

    set_name ("ThreadIndexListView");

    thread_index = _thread_index;
    main_window  = _thread_index->main_window;
    list_store = store;

    config = astroid->config ("thread_index");
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

    astroid->global_actions->signal_thread_changed ().connect (
        sigc::mem_fun (this, &ThreadIndexListView::on_thread_changed));

    astroid->global_actions->signal_refreshed ().connect (
        sigc::mem_fun (this, &ThreadIndexListView::on_refreshed));

    /* re-draw every minute (check every second) */
    Glib::signal_timeout ().connect (
        sigc::mem_fun (this, &ThreadIndexListView::redraw), 1000);

    /* mouse click */
    signal_row_activated ().connect (
        sigc::mem_fun (this, &ThreadIndexListView::on_my_row_activated));

    /* set up popup menu {{{ */

    /* icon list */
    Gtk::Image * reply = Gtk::manage (new Gtk::Image ());
    reply->set_from_icon_name ("gtk-undo", Gtk::ICON_SIZE_LARGE_TOOLBAR);
    Gtk::MenuItem * reply_i = Gtk::manage (new Gtk::MenuItem (*reply));
    reply_i->signal_activate ().connect (
        sigc::bind (
          sigc::mem_fun (*this, &ThreadIndexListView::popup_activate_generic)
          , PopupItem::Reply));
    reply_i->set_tooltip_text ("Reply to latest message");

    Gtk::Image * forward = Gtk::manage (new Gtk::Image ());
    forward->set_from_icon_name ("gtk-go-forward", Gtk::ICON_SIZE_LARGE_TOOLBAR);
    Gtk::MenuItem * forward_i = Gtk::manage (new Gtk::MenuItem (*forward));
    forward_i->signal_activate ().connect (
        sigc::bind (
          sigc::mem_fun (*this, &ThreadIndexListView::popup_activate_generic)
          , PopupItem::Forward));
    forward_i->set_tooltip_text ("Forward latest message");

    Gtk::Image * flag = Gtk::manage (new Gtk::Image ());
    flag->set_from_icon_name ("starred-symbolic", Gtk::ICON_SIZE_LARGE_TOOLBAR);
    Gtk::MenuItem * flag_i = Gtk::manage (new Gtk::MenuItem (*flag));
    flag_i->signal_activate ().connect (
        sigc::bind (
          sigc::mem_fun (*this, &ThreadIndexListView::popup_activate_generic)
          , PopupItem::Flag));
    flag_i->set_tooltip_text ("Flag");

    Gtk::Image * archive = Gtk::manage (new Gtk::Image ());
    archive->set_from_icon_name ("gtk-apply", Gtk::ICON_SIZE_LARGE_TOOLBAR);
    Gtk::MenuItem * archive_i = Gtk::manage (new Gtk::MenuItem (*archive));
    archive_i->signal_activate ().connect (
        sigc::bind (
          sigc::mem_fun (*this, &ThreadIndexListView::popup_activate_generic)
          , PopupItem::Archive));
    archive_i->set_tooltip_text ("Archive");

    item_popup.attach (*reply_i, 0, 1, 0, 1);
    item_popup.attach (*forward_i, 1, 2, 0, 1);
    item_popup.attach (*flag_i, 2, 3, 0, 1);
    item_popup.attach (*archive_i, 3, 4, 0, 1);

    Gtk::MenuItem * sep = Gtk::manage (new Gtk::SeparatorMenuItem ());
    item_popup.append (*sep);

    Gtk::MenuItem * item = Gtk::manage (new Gtk::MenuItem ("Open"));
    item->signal_activate ().connect (
        sigc::bind (
          sigc::mem_fun (*this, &ThreadIndexListView::popup_activate_generic)
          , PopupItem::Open));
    item_popup.append (*item);

    item = Gtk::manage (new Gtk::MenuItem ("Open in new window"));
    item->signal_activate ().connect (
        sigc::bind (
          sigc::mem_fun (*this, &ThreadIndexListView::popup_activate_generic)
          , PopupItem::OpenNewWindow));
    item_popup.append (*item);

    item_popup.accelerate (*this);
    item_popup.show_all ();

    // }}}

    // background image {{{
    auto css = Gtk::CssProvider::create ();
    auto sc  = get_style_context ();

    path no_mail_img = Resource (true, "ui/no-mail.png").get_path ();

    string css_data =
      ustring::compose (
                      "#ThreadIndexListView  { background-image: url(\"%1\");"
                      "                        background-repeat: no-repeat;"
                      "                        background-position: 98%% 98%%;"
                      " }\n"
                      "#ThreadIndexListView  .hide_bg { background-image: none; }",
                      no_mail_img.c_str ());

    try {
      css->load_from_data (css_data);
    } catch (Gtk::CssProviderError &e) {
      log << error << "ti: attempted to set background image: " << no_mail_img.c_str () << ": " << e.what () << endl;
    }

    auto screen = Gdk::Screen::get_default ();
    sc->add_provider_for_screen (screen, css, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    // }}}

    /* set up keys */
    register_keys ();
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
      r->marked = row[list_store->columns.marked];

    }
  }

  void ThreadIndexListView::set_sort_type (notmuch_sort_t sort) {
    if (sort == NOTMUCH_SORT_NEWEST_FIRST) {
      list_store->set_sort_column (0, Gtk::SortType::SORT_DESCENDING);
    } else if (sort == NOTMUCH_SORT_OLDEST_FIRST) {
      list_store->set_sort_column (1, Gtk::SortType::SORT_ASCENDING);
    } else {
      list_store->set_sort_column (Gtk::TreeSortable::DEFAULT_UNSORTED_COLUMN_ID, Gtk::SortType::SORT_ASCENDING);
    }
  }

  void ThreadIndexListView::register_keys () {

    Keybindings * keys = &(thread_index->keys);

    keys->register_key ("j", { Key(false, false, (guint) GDK_KEY_Down) },
        "thread_index.next_thread", "Next thread",
        [&](Key) {
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
        });

    keys->register_key (Key (GDK_KEY_Tab),
        "thread_index.next_unread",
        "Jump to next unread thread",
        [&] (Key) {
          Gtk::TreePath thispath, path;
          Gtk::TreeIter fwditer;
          Gtk::TreeViewColumn *c;

          get_cursor (path, c);
          path.next ();
          fwditer = list_store->get_iter (path);
          thispath = path;

          Gtk::ListStore::Row row;

          bool found = false;
          while (fwditer) {
            row = *fwditer;

            Glib::RefPtr<NotmuchThread> thread = row[list_store->columns.thread];
            if (thread->unread) {
              path = list_store->get_path (fwditer);
              set_cursor (path);
              found = true;
              break;
            }

            fwditer++;
          }

          /* wrap, and check from start */
          if (!found) {
            fwditer = list_store->children().begin ();

            while (fwditer && list_store->get_path(fwditer) < thispath) {
            row = *fwditer;

            Glib::RefPtr<NotmuchThread> thread = row[list_store->columns.thread];
            if (thread->unread) {
              path = list_store->get_path (fwditer);
              set_cursor (path);
              found = true;
              break;
            }

            fwditer++;
            }
          }

          return true;
        });

    keys->register_key (Key (false, false, true, (guint) GDK_KEY_ISO_Left_Tab),
        "thread_index.previous_unread",
        "Jump to previous unread thread",
        [&] (Key) {
          Gtk::TreePath thispath, path;
          Gtk::TreeIter iter;
          Gtk::TreeViewColumn *c;

          get_cursor (path, c);
          path.prev ();
          iter = list_store->get_iter (path);
          thispath = path;

          Gtk::ListStore::Row row;

          bool found = false;
          while (iter) {
            row = *iter;

            Glib::RefPtr<NotmuchThread> thread = row[list_store->columns.thread];
            if (thread->unread) {
              path = list_store->get_path (iter);
              set_cursor (path);
              found = true;
              break;
            }

            iter--;
          }

          /* wrap, and check from end */
          if (!found) {
            iter = list_store->children().end ();
            iter--;

            while (iter && list_store->get_path(iter) > thispath) {
            row = *iter;

            Glib::RefPtr<NotmuchThread> thread = row[list_store->columns.thread];
            if (thread->unread) {
              path = list_store->get_path (iter);
              set_cursor (path);
              found = true;
              break;
            }

            iter--;
            }
          }

          return true;
        });

    keys->register_key ("k", { Key(false, false, (guint) GDK_KEY_Up) },
        "thread_index.prev_thread", "Previous thread",
        [&](Key) {
            Gtk::TreePath path;
            Gtk::TreeViewColumn *c;
            get_cursor (path, c);
            path.prev ();
            if (path) {
              set_cursor (path);
            }
            return true;
          });


    /* set up for multi key handler */
    multi_keys.register_key ("N",
                             "thread_index.multi.mark_unread",
                             "Mark unread",
                             bind (&ThreadIndexListView::multi_key_handler, this, MUnread, _1));

    multi_keys.register_key ("*",
                             "thread_index.multi.flag",
                             "Flag",
                             bind (&ThreadIndexListView::multi_key_handler, this, MFlag, _1));

    multi_keys.register_key ("a",
                             "thread_index.multi.archive",
                             "Archive",
                             bind (&ThreadIndexListView::multi_key_handler, this, MArchive, _1));

    multi_keys.register_key ("S",
                             "thread_index.multi.mark_spam",
                             "Mark spam",
                             bind (&ThreadIndexListView::multi_key_handler, this, MSpam, _1));

    multi_keys.register_key ("l",
                             "thread_index.multi.tag",
                             "Tag",
                             bind (&ThreadIndexListView::multi_key_handler, this, MTag, _1));


    multi_keys.register_key ("C-m",
                             "thread_index.multi.mute",
                             "Mute",
                             bind (&ThreadIndexListView::multi_key_handler, this, MMute, _1));

    multi_keys.register_key ("t",
                             "thread_index.multi.toggle",
                             "Toggle marked",
                             bind (&ThreadIndexListView::multi_key_handler, this, MToggle, _1));

    keys->register_key ("+",
          "thread_index.multi",
          "Apply action to marked threads",
          [&] (Key k) {

            /* check if anything is marked */
            Gtk::TreePath path;
            Gtk::TreeIter fwditer;

            fwditer = list_store->get_iter ("0");
            Gtk::ListStore::Row row;

            bool found = false;

            while (fwditer) {
              row = *fwditer;
              if (row[list_store->columns.marked]) {
                found = true;
                break;
              }

              fwditer++;
            }

            if (found) {
              thread_index->multi_key (multi_keys, k);
            }

            return true;
          });

    keys->register_key ("J", "thread_index.scroll_down",
        "Scroll down",
        [&] (Key) {
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
        });

    keys->register_key ("K", "thread_index.scroll_up",
        "Scroll up",
        [&] (Key) {
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
        });

    keys->register_key ("1", { Key(GDK_KEY_Home) }, "thread_index.scroll_home",
        "Scroll to first line",
        [&] (Key) {
          /* select first */
          set_cursor (Gtk::TreePath("0"));
          return true;
        });

    keys->register_key ("0", { Key (GDK_KEY_End) }, "thread_index.scroll_end",
        "Scroll to last line",
        [&] (Key) {
          if (list_store->children().size() >= 1) {
            auto it = list_store->children().end ();
            auto p  = list_store->get_path (--it);
            if (p) set_cursor (p);
          }

          return true;
        });

    keys->register_key (Key (GDK_KEY_Return), { Key (GDK_KEY_KP_Enter) },
        "thread_index.open_thread",
        "Open thread",
        [&] (Key) {
          /* open message in split pane (if so configured) */
          auto thread = get_current_thread ();

          if (thread)
            thread_index->open_thread (thread, true);

          return true;
        });

    keys->register_key (Key (false, true, (guint) GDK_KEY_Return),
        { Key (false, true, (guint) GDK_KEY_KP_Enter) },
        "thread_index.open_paned",
        "Open thread in pane",
        [&] (Key) {
          /* open message in split pane (if so configured) */
          auto thread = get_current_thread ();

          if (thread)
            thread_index->open_thread (thread, false);

          return true;
        });

    keys->register_key (Key (true, false, (guint) GDK_KEY_Return),
        { Key (true, false, (guint) GDK_KEY_KP_Enter) },
        "thread_index.open_new_window",
        "Open thread in new window",
        [&] (Key) {
          /* open message in split pane (if so configured) */
          auto thread = get_current_thread ();

          if (thread)
            thread_index->open_thread (thread, false, true);

          return true;
        });

    keys->register_key ("r", "thread_index.reply",
        "Reply to last message in thread",
        [&] (Key) {
          auto thread = get_current_thread ();
          if (thread) {

            MessageThread mthread (thread);
            Db db (Db::DbMode::DATABASE_READ_ONLY);

            mthread.load_messages (&db);

            /* reply to last message */
            main_window->add_mode (new ReplyMessage (main_window, *(--mthread.messages.end())));

          }
          return true;
        });

    keys->register_key ("G", "thread_index.reply_all",
        "Reply all to last message in thread",
        [&] (Key) {
          auto thread = get_current_thread ();
          if (thread) {

            MessageThread mthread (thread);
            Db db (Db::DbMode::DATABASE_READ_ONLY);

            mthread.load_messages (&db);

            /* reply to last message */
            main_window->add_mode (new ReplyMessage (main_window, *(--mthread.messages.end()), ReplyMessage::ReplyMode::Rep_All));

          }
          return true;
        });

    keys->register_key ("f", "thread_index.forward",
        "Forward last message in thread",
        [&] (Key) {
          auto thread = get_current_thread ();
          if (thread) {

            MessageThread mthread (thread);
            Db db (Db::DbMode::DATABASE_READ_ONLY);

            mthread.load_messages (&db);

            /* reply to last message */
            main_window->add_mode (new ForwardMessage (main_window, *(--mthread.messages.end())));

          }
          return true;
        });

    keys->register_key (UnboundKey (), "thread_index.forward_inline",
        "Forward last message in thread inlined",
        [&] (Key) {
          auto thread = get_current_thread ();
          if (thread) {

            MessageThread mthread (thread);
            Db db (Db::DbMode::DATABASE_READ_ONLY);

            mthread.load_messages (&db);

            /* reply to last message */
            main_window->add_mode (new ForwardMessage (main_window, *(--mthread.messages.end()), ForwardMessage::FwdDisposition::FwdInline));

          }
          return true;
        });

    keys->register_key (UnboundKey (), "thread_index.forward_attached",
        "Forward last message in thread attached",
        [&] (Key) {
          auto thread = get_current_thread ();
          if (thread) {

            MessageThread mthread (thread);
            Db db (Db::DbMode::DATABASE_READ_ONLY);

            mthread.load_messages (&db);

            /* reply to last message */
            main_window->add_mode (new ForwardMessage (main_window, *(--mthread.messages.end()), ForwardMessage::FwdDisposition::FwdAttach));

          }
          return true;
        });

    keys->register_key ("t", "thread_index.toggle_marked",
        "Mark thread",
        [&] (Key) {
          if (list_store->children().size() < 1)
            return true;

          Gtk::TreePath path;
          Gtk::TreeViewColumn *c;
          get_cursor (path, c);
          Gtk::TreeIter iter;

          iter = list_store->get_iter (path);

          if (iter) {
            Gtk::ListStore::Row row = *iter;
            row[list_store->columns.marked] = !row[list_store->columns.marked];
          }

          return true;
        });

    keys->register_key ("T", "thread_index.toggle_marked_all",
        "Toggle arked on all loaded threads",
        [&] (Key) {
          Gtk::TreePath path;
          Gtk::TreeIter fwditer;
          fwditer = list_store->get_iter ("0");
          Gtk::ListStore::Row row;
          while (fwditer) {
            row = *fwditer;
            row[list_store->columns.marked] = !row[list_store->columns.marked];
            fwditer++;
          }
          return true;
        });

    keys->register_key ("M", "thread_index.load_more",
        "Load more threads",
        [&] (Key) {
          thread_index->load_more_threads ();
          return true;
        });

    keys->register_key (Key(GDK_KEY_exclam), "thread_index.load_all",
        "Load all threads",
        [&] (Key) {
          thread_index->load_more_threads (true);
          return true;
        });

    keys->register_key ("a", "thread_index.archive",
        "Toggle 'inbox' tag on thread",
        [&] (Key) {
          auto thread = get_current_thread ();
          if (thread) {
            Db db (Db::DbMode::DATABASE_READ_WRITE);
            main_window->actions.doit (&db, refptr<Action>(new ToggleAction(thread, "inbox")));
          }

          return true;
        });

    keys->register_key (Key (GDK_KEY_asterisk), "thread_index.flag",
        "Toggle 'flagged' tag on thread",
        [&] (Key) {
          auto thread = get_current_thread ();
          if (thread) {

            Db db (Db::DbMode::DATABASE_READ_WRITE);
            main_window->actions.doit (&db, refptr<Action>(new ToggleAction(thread, "flagged")));
          }

          return true;
        });

    keys->register_key ("N", "thread_index.unread",
        "Toggle 'unread' tag on thread",
        [&] (Key) {
          auto thread = get_current_thread ();
          if (thread) {

            Db db (Db::DbMode::DATABASE_READ_WRITE);
            main_window->actions.doit (&db, refptr<Action>(new ToggleAction(thread, "unread")));
          }

          return true;
        });

    keys->register_key ("S", "thread_index.spam",
        "Toggle 'spam' tag on thread",
        [&] (Key) {
          auto thread = get_current_thread ();
          if (thread) {

            Db db (Db::DbMode::DATABASE_READ_WRITE);
            main_window->actions.doit (&db, refptr<Action>(new SpamAction(thread)));
          }

          return true;
        });

    keys->register_key ("C-m", "thread_index.mute",
        "Toggle 'muted' tag on thread, it will be excluded from searches",
        [&] (Key) {
          auto thread = get_current_thread ();
          if (thread) {

            Db db (Db::DbMode::DATABASE_READ_WRITE);
            main_window->actions.doit (&db, refptr<Action>(new MuteAction(thread)));
          }

          return true;
        });

    keys->register_key ("l", "thread_index.label",
        "Edit tags for thread",
        [&] (Key) {
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

          return true;
        });

    keys->register_key ("E", "thread_index.edit_draft",
        "Edit first message marked as draft or last message in thread as new",
        [&] (Key) {
          auto thread = get_current_thread ();
          if (thread) {

            MessageThread mthread (thread);
            Db db (Db::DbMode::DATABASE_READ_ONLY);

            mthread.load_messages (&db);

            bool found = false;

            /* find first message marked as draft */
            for (auto m : mthread.messages) {
              if (any_of (db.draft_tags.begin (),
                          db.draft_tags.end (),
                          [&](ustring t) {
                            return has (m->tags, t);
                          }))
              {


                main_window->add_mode (new EditMessage (main_window, m));

                found = true;
                break;
              }
            }

            if (!found) {
              /* edit last message as draft */
              main_window->add_mode (new EditMessage (main_window, *(--mthread.messages.end())));
            }
          }
          return true;
        });

    keys->register_run ("thread_index.run",
        [&] (Key, ustring cmd) {
          auto t = get_current_thread ();

          if (t) {
            cmd = ustring::compose (cmd, t->thread_id);
            int r = Cmd ("thread_index.run", cmd).run ();

            if (r == 0) {
              Db db;
              astroid->global_actions->emit_thread_updated (&db, t->thread_id);
            }
          }

          return true;
        });
  }


  bool ThreadIndexListView::multi_key_handler (
      ThreadIndexListView::multi_key_action maction,
      Key) {
    log << debug << "tl: m k h" << endl;

    Gtk::TreePath path;
    Gtk::TreeIter fwditer;

    /* forward iterating is much faster than going backwards:
     * https://developer.gnome.org/gtkmm/3.11/classGtk_1_1TreeIter.html
     */
    fwditer = list_store->get_iter ("0");
    Gtk::ListStore::Row row;

    switch (maction) {
      case MFlag:
      case MUnread:
      case MSpam:
      case MMute:
      case MArchive:
      case MTag:
        {
          vector<refptr<NotmuchTaggable>> threads;

          while (fwditer) {
            row = *fwditer;
            if (row[list_store->columns.marked]) {

              // row[list_store->columns.marked] = false;
              refptr<NotmuchThread> thread = row[list_store->columns.thread];

              threads.push_back (refptr<NotmuchTaggable>::cast_dynamic(thread));
            }

            fwditer++;
          }

          refptr<Action> a;
          switch (maction) {
            case MArchive:
              a = refptr<Action>(new ToggleAction(threads, "inbox"));
              break;

            case MFlag:
              a = refptr<Action>(new ToggleAction(threads, "flagged"));
              break;

            case MUnread:
              a = refptr<Action>(new ToggleAction(threads, "unread"));
              break;

            case MSpam:
              a = refptr<Action>(new SpamAction(threads));
              break;

            case MMute:
              a = refptr<Action>(new MuteAction(threads));
              break;

            case MTag:
              {
                /* ask for tags */
                main_window->enable_command (CommandBar::CommandMode::DiffTag,
                    "",
                    [&, threads](ustring tgs) {
                      log << debug << "ti: got difftags: " << tgs << endl;

                      refptr<Action> ma = refptr<DiffTagAction> (DiffTagAction::create (threads, tgs));
                      if (ma) {
                        Db db (Db::DbMode::DATABASE_READ_WRITE);
                        main_window->actions.doit (&db, ma);
                      }
                    });
                return true;
              }
              break;

            default:
              throw runtime_error ("impossible.");
          }

          if ((maction != MTag) && a) {
            Db db (Db::DbMode::DATABASE_READ_WRITE);
            main_window->actions.doit (&db, a);
          }

          return true;
        }
        break;

      case MToggle:
        {
          while (fwditer) {
            row = *fwditer;
            if (row[list_store->columns.marked]) {

              row[list_store->columns.marked] = false;

            }

            fwditer++;
          }

          return true;
        }
    }
    return false;
  }

  bool ThreadIndexListView::on_key_press_event (GdkEventKey * e) {
    /* bypass scrolled window */
    return thread_index->on_key_press_event (e);
  }

  bool ThreadIndexListView::on_button_press_event (GdkEventButton *ev) {
    /* Open context menu. */

    bool return_value = TreeView::on_button_press_event(ev);

    if ((ev->type == GDK_BUTTON_PRESS) && (ev->button == 3)) {

      item_popup.popup (ev->button, ev->time);

    }

    return return_value;
  }

  void ThreadIndexListView::popup_activate_generic (enum PopupItem pi) {
    auto thread = get_current_thread ();

    switch (pi) {
      case Reply:
        {
          if (thread) {

            MessageThread mthread (thread);
            Db db (Db::DbMode::DATABASE_READ_ONLY);

            mthread.load_messages (&db);

            /* reply to last message */
            main_window->add_mode (new ReplyMessage (main_window, *(--mthread.messages.end())));

          }
        }
        break;

      case Forward:
        {
          if (thread) {

            MessageThread mthread (thread);
            Db db (Db::DbMode::DATABASE_READ_ONLY);

            mthread.load_messages (&db);

            /* reply to last message */
            main_window->add_mode (new ForwardMessage (main_window, *(--mthread.messages.end())));

          }
        }
        break;

      case Flag:
        {
          if (thread) {

            Db db (Db::DbMode::DATABASE_READ_WRITE);
            main_window->actions.doit (&db, refptr<Action>(new ToggleAction(thread, "flagged")));

          }
        }
        break;

      case Archive:
        {
          if (thread) {

            Db db (Db::DbMode::DATABASE_READ_WRITE);
            main_window->actions.doit (&db, refptr<Action>(new ToggleAction(thread, "inbox")));

          }
        }
        break;

      case Open:
        {
          if (thread) {
            log << info << "ti_list: loading: " << thread->thread_id << endl;

            thread_index->open_thread (thread, true);
          }
        }
        break;

      case OpenNewWindow:
        {
          if (thread) {
            log << info << "ti_list: loading: " << thread->thread_id << endl;

            thread_index->open_thread (thread, true, true);
          }
        }
        break;
    }
  }

  void ThreadIndexListView::on_my_row_activated (
      const Gtk::TreeModel::Path &,
      Gtk::TreeViewColumn *) {

    auto thread = get_current_thread ();

    if (thread) {
      log << info << "ti_list: loading: " << thread->thread_id << endl;

      /* open message in new tab  */
      thread_index->open_thread (thread, true);
    }
  }

  void ThreadIndexListView::on_refreshed () {
    log << debug << "til: got refreshed signal." << endl;
    thread_index->refresh (false, max(thread_index->thread_load_step, thread_index->current_thread), false);
  }

  void ThreadIndexListView::on_thread_changed (Db * db, ustring thread_id) {
    log << info << "til: got changed thread signal: " << thread_id << endl;

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
        row[list_store->columns.oldest_date] = thread->oldest_date;

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
        newrow[list_store->columns.oldest_date] = t->oldest_date;
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

  void ThreadIndexListView::update_bg_image () {
    bool hide = (list_store->children().empty());

    if (!hide) {
      auto sc = get_style_context ();
      if (!sc->has_class ("hide_bg")) sc->add_class ("hide_bg");
    } else {
      auto sc = get_style_context ();
      if (sc->has_class ("hide_bg")) sc->remove_class ("hide_bg");
    }
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

