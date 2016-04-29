# include "saved_searches.hh"
# include "astroid.hh"
# include "config.hh"
# include "log.hh"
# include "main_window.hh"
# include "thread_index/thread_index.hh"
# include "db.hh"

using std::endl;

namespace Astroid {

  SavedSearches::SavedSearches (MainWindow * mw) : Mode (mw) {
    set_label ("Saved searches");

    scroll.add (tv);
    pack_start (scroll);

    store = Gtk::ListStore::create (m_columns);
    tv.set_model (store);

    tv.append_column ("Name", m_columns.m_col_name);
    tv.append_column ("Query", m_columns.m_col_query);
    tv.append_column ("Unread messages", m_columns.m_col_unread_messages);
    tv.append_column ("Total messages ", m_columns.m_col_total_messages);

    tv.set_headers_visible (false);
    tv.set_sensitive (true);
    set_sensitive (true);
    set_can_focus (true);

    show_all_children ();

    /* register keys {{{ */
    keys.title = "Saved searches";
    keys.register_key ("j", { Key (GDK_KEY_Down) },
        "searches.down",
        "Move cursor down",
        [&] (Key) {
          if (store->children().size() < 2)
            return true;

          Gtk::TreePath path;
          Gtk::TreeViewColumn *c;
          tv.get_cursor (path, c);

          path.next ();
          Gtk::TreeIter it = store->get_iter (path);

          if (it) {
            tv.set_cursor (path);
          }

          return true;
        });

    keys.register_key ("k", { Key (GDK_KEY_Up) },
        "searches.up",
        "Move cursor up",
        [&] (Key) {
          Gtk::TreePath path;
          Gtk::TreeViewColumn *c;
          tv.get_cursor (path, c);
          path.prev ();
          if (path) {
            tv.set_cursor (path);
          }
          return true;
        });

    keys.register_key ("J",
        "searches.page_down",
        "Page down",
        [&] (Key) {
          auto adj = tv.get_vadjustment ();
          adj->set_value (adj->get_value() + adj->get_step_increment ());
          return true;
        });

    keys.register_key ("K",
        "searches.page_up",
        "Page up",
        [&] (Key) {
          auto adj = tv.get_vadjustment ();
          adj->set_value (adj->get_value() - adj->get_step_increment ());
          return true;
        });

    keys.register_key ("1", { Key (GDK_KEY_Home) },
        "searches.home",
        "Scroll home",
        [&] (Key) {
          /* select first */
          tv.set_cursor (Gtk::TreePath("0"));
          return true;
        });

    keys.register_key ("0", { Key (GDK_KEY_End) },
        "searches.end",
        "Scroll to end",
        [&] (Key) {
          /* select last */
          auto it = store->children().end ();
          auto p  = store->get_path (--it);
          tv.set_cursor (p);

          return true;
        });

    keys.register_key (Key (GDK_KEY_Return), { Key (GDK_KEY_KP_Enter) },
        "searches.open",
        "Open query",
        [&] (Key) {

          Gtk::TreePath path;
          Gtk::TreeViewColumn *c;
          tv.get_cursor (path, c);
          Gtk::TreeIter iter;

          iter = store->get_iter (path);
          Gtk::ListStore::Row row = *iter;
          /* return row[list_store->columns.thread]; */

          ustring query = row[m_columns.m_col_query];
          ustring name  = row[m_columns.m_col_name];

          Mode * ti = new ThreadIndex (main_window, query, name);
          main_window->add_mode (ti); 

          return true;
        });

    /* }}} */

    load_startup_queries ();

    tv.set_cursor (Gtk::TreePath("0"));
  }

  void SavedSearches::add_query (ustring name, ustring query) {
    auto iter = store->append();
    auto row  = *iter;

    row[m_columns.m_col_name] = name;
    row[m_columns.m_col_query] = query;

    unsigned int total_messages, unread_messages;

    /* get stats */
    Db db;
    notmuch_query_t * query_t =  notmuch_query_create (db.nm_db, query.c_str ());
    for (ustring & t : db.excluded_tags) {
      notmuch_query_add_tag_exclude (query_t, t.c_str());
    }
    notmuch_query_set_omit_excluded (query_t, NOTMUCH_EXCLUDE_TRUE);
    /* st = */ notmuch_query_count_messages_st (query_t, &total_messages); // destructive
    notmuch_query_destroy (query_t);

    ustring unread_q_s = "(" + query + ") AND tag:unread";
    notmuch_query_t * unread_q = notmuch_query_create (db.nm_db, unread_q_s.c_str());
    for (ustring & t : db.excluded_tags) {
      notmuch_query_add_tag_exclude (unread_q, t.c_str());
    }
    notmuch_query_set_omit_excluded (unread_q, NOTMUCH_EXCLUDE_TRUE);
    /* st = */ notmuch_query_count_messages_st (unread_q, &unread_messages); // destructive
    notmuch_query_destroy (unread_q);

    row[m_columns.m_col_total_messages] = total_messages;
    row[m_columns.m_col_unread_messages] = unread_messages;
  }

  void SavedSearches::load_startup_queries () {
    ptree qpt = astroid->config ("startup.queries");

    for (const auto &kv : qpt) {
      ustring name = kv.first;
      ustring query = kv.second.data();

      log << info << "saved searches: got query: " << name << ": " << query << endl;
      add_query (name, query);
    }
  }

  void SavedSearches::grab_modal () {
    add_modal_grab ();
    grab_focus ();
  }

  void SavedSearches::release_modal () {
    remove_modal_grab ();
  }



}


