# include "saved_searches.hh"
# include "astroid.hh"
# include "config.hh"
# include "log.hh"
# include "main_window.hh"
# include "thread_index/thread_index.hh"
# include "db.hh"

# include <boost/property_tree/ptree.hpp>
# include <boost/property_tree/json_parser.hpp>

using boost::property_tree::ptree;
using std::endl;

namespace Astroid {
  Glib::Dispatcher SavedSearches::m_reload;

  SavedSearches::SavedSearches (MainWindow * mw) : Mode (mw) {
    set_label ("Saved searches");

    scroll.add (tv);
    pack_start (scroll);

    store = Gtk::ListStore::create (m_columns);
    tv.set_model (store);

    /* tv.append_column ("Name", m_columns.m_col_name); */

    Gtk::CellRendererText * renderer_text = Gtk::manage (new Gtk::CellRendererText);
    /* renderer_text->property_family().set_value ("monospace"); */
    int cols_count = tv.append_column ("Name", *renderer_text);
    Gtk::TreeViewColumn * pcolumn = tv.get_column (cols_count -1);
    if (pcolumn) {
      pcolumn->add_attribute (renderer_text->property_markup(), m_columns.m_col_name);
    }

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

          while (path) {
            Gtk::TreeIter it = store->get_iter (path);
            if (!it) return true;
            if (!(*it)[m_columns.m_col_description]) break;
            path.next ();
          }

          if (path) {
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

          if (!path.prev ()) return true;

          while (path) {
            Gtk::TreeIter it = store->get_iter (path);
            if (!it) return true;
            if (!(*it)[m_columns.m_col_description]) break;
            if (!path.prev ()) return true;
          }

          if (path) {
            tv.set_cursor (path);
          }
          return true;
        });

    keys.register_key ("d",
        "searches.delete",
        "Delete saved query",
        [&] (Key) {
          Gtk::TreePath path;
          Gtk::TreeViewColumn *c;
          tv.get_cursor (path, c);

          Gtk::TreeIter it = store->get_iter (path);
          if (it) {

            auto row = *it;
            ustring name  = row[m_columns.m_col_name];
            ustring query = row[m_columns.m_col_query];

            if (row[m_columns.m_col_saved]) {
              ptree s = load_searches ().get_child ("saved");
              bool changed = false;

              /* TODO: warning, this will delete the first occurence of the query */
              for (auto it = s.begin (); it != s.end ();) {
                if (it->second.data() == query) {
                  log << info << "searches: deleting: " << name << ":" << query << endl;

                  it = s.erase (it);
                  changed = true;
                  break;
                } else {
                  it++;
                }
              }

              if (changed) write_back_searches (s);

            } else if (row[m_columns.m_col_history]) {
              ptree s = load_searches ().get_child ("history");
              bool changed = false;

              /* TODO: warning, this will delete the first occurence of the query */
              for (auto it = s.begin (); it != s.end ();) {
                if (it->second.data() == query) {
                  log << info << "searches: deleting: " << name << ":" << query << endl;

                  it = s.erase (it);
                  changed = true;
                  break;
                } else {
                  it++;
                }
              }

              if (changed) write_back_searches (s);
            }


          }

          return true;
        });

    keys.register_key ("C",
        "searches.clear_history",
        "Clear search history",
        [&] (Key) {

          log << info << "searches: clearing search history.." << endl;

          ptree s = load_searches ();
          for (auto it = s.begin (); it != s.end ();) {
            if (it->first == "history") {
              it = s.erase (it);
              break;
            } else it++;
          }

          write_back_searches (s);

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

    reload ();
    tv.set_cursor (Gtk::TreePath("1"));

    SavedSearches::m_reload.connect (
        sigc::mem_fun (this, &SavedSearches::reload));

    astroid->actions->signal_thread_changed ().connect (
        sigc::mem_fun (this, &SavedSearches::on_thread_changed));

    astroid->actions->signal_refreshed ().connect (
        sigc::mem_fun (this, &SavedSearches::reload));
  }

  void SavedSearches::reload () {
    Gtk::TreePath path;
    Gtk::TreeViewColumn *c;
    tv.get_cursor (path, c);

    store->clear ();
    load_startup_queries ();
    load_saved_searches ();

    tv.set_cursor (path);
  }

  void SavedSearches::add_query (ustring name, ustring query, bool saved, bool history) {
    auto iter = store->append();
    auto row  = *iter;

    row[m_columns.m_col_name] = name;
    row[m_columns.m_col_query] = query;
    row[m_columns.m_col_saved] = saved;
    row[m_columns.m_col_history] = history;

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

    row[m_columns.m_col_unread_messages] = ustring::compose ("(unread: %1)", unread_messages);
    row[m_columns.m_col_total_messages] = ustring::compose ("(total: %1)", total_messages);
  }

  void SavedSearches::on_thread_changed (Db *, ustring) {
    refresh_stats ();
  }

  void SavedSearches::refresh_stats () {
    for (auto row : store->children ()) {
      if (row[m_columns.m_col_description]) continue;

      ustring query = row[m_columns.m_col_query];

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

      row[m_columns.m_col_unread_messages] = ustring::compose ("(unread: %1)", unread_messages);
      row[m_columns.m_col_total_messages] = ustring::compose ("(total: %1)", total_messages);
    }
  }

  void SavedSearches::load_startup_queries () {
    /* add description */
    auto iter = store->append();
    auto row  = *iter;

    row[m_columns.m_col_name] = "<b>Startup queries</b>";
    row[m_columns.m_col_description] = true;


    /* load start up queries */
    ptree qpt = astroid->config ("startup.queries");

    for (const auto &kv : qpt) {
      ustring name = kv.first;
      ustring query = kv.second.data();

      log << info << "saved searches: got query: " << name << ": " << query << endl;
      add_query (name, query);
    }
  }

  void SavedSearches::load_saved_searches () {
    /* add description */
    auto iter = store->append();
    auto row  = *iter;

    row[m_columns.m_col_name] = "<b>Saved searches</b>";
    row[m_columns.m_col_description] = true;

    ptree searches = load_searches ();

    /* load saved queries */
    for (auto &kv : searches.get_child ("saved")) {
      ustring name = kv.first;
      ustring query = kv.second.data();

      if (name == "none") name = "";

      log << info << "saved searches: got query: " << name << ": " << query << endl;
      add_query (name, query, true);
    }

    /* load search history */
    iter = store->append();
    row  = *iter;

    row[m_columns.m_col_name] = "<b>Search history</b>";
    row[m_columns.m_col_description] = true;

    std::vector<std::pair<ustring, ustring>> history;

    for (auto &kv : searches.get_child ("history")) {
      ustring name = kv.first;
      ustring query = kv.second.data();

      if (name == "none") name = "";

      log << debug << "saved searches, history: got query: " << name << ": " << query << endl;
      history.push_back (std::make_pair (name, query));
    }

    for (auto it = history.rbegin (); it != history.rend (); ++it) {
      add_query (it->first, it->second, false, true);
    }

  }

  std::vector<ustring> SavedSearches::get_history () {
    ptree searches = load_searches ();

    /* load search history */
    std::vector<ustring> history;

    for (auto &kv : searches.get_child ("history")) {
      ustring name = kv.first;
      ustring query = kv.second.data();

      if (name == "none") name = "";

      log << debug << "saved searches, history: got query: " << name << ": " << query << endl;
      history.push_back (query);
    }

    std::reverse (history.begin (), history.end ());

    return history;
  }

  ptree SavedSearches::load_searches () {
    ptree s;

    if (is_regular_file (astroid->standard_paths().searches_file))
    {
      log << info << "searches: loading saved searches.." << endl;
      read_json (astroid->standard_paths().searches_file.c_str(), s);
    }


    if (s.count ("saved") == 0) {
      s.put ("saved", "");
    }

    if (s.count ("history") == 0) {
      s.put ("history", "");
    }

    return s;
  }

  void SavedSearches::write_back_searches (ptree s) {
    log << info << "searches: writing back saved searches.." << endl;
    write_json (astroid->standard_paths().searches_file.c_str (), s);
    m_reload ();
  }

  void SavedSearches::save_query (ustring q) {
    log << info << "searches: adding query: " << q << endl;

    ptree s = load_searches ();
    s.add ("saved.none", q);
    write_back_searches (s);
  }

  void SavedSearches::add_query_to_history (ustring q) {
    bool save = astroid->config ("saved_searches").get<bool> ("save_history");

    if (save) {
      ptree s = load_searches ();
      s.add ("history.none", q);
      write_back_searches (s);
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


