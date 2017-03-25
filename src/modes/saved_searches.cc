# include "saved_searches.hh"
# include "astroid.hh"
# include "config.hh"
# include "main_window.hh"
# include "thread_index/thread_index.hh"
# include "db.hh"

# include <boost/property_tree/ptree.hpp>
# include <boost/property_tree/json_parser.hpp>

using boost::property_tree::ptree;
using std::endl;

namespace Astroid {
  Glib::Dispatcher SavedSearches::m_reload;
  std::vector<ustring> SavedSearches::history;

  SavedSearches::SavedSearches (MainWindow * mw) : Mode (mw) {
    set_label ("Saved searches");

    page_jump_rows = astroid->config("thread_index").get<int>("page_jump_rows");

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

    keys.register_key ("s",
        "searches.save",
        "Save recent query as saved search",
        [&] (Key) {
          Gtk::TreePath path;
          Gtk::TreeViewColumn *c;
          tv.get_cursor (path, c);

          Gtk::TreeIter it = store->get_iter (path);
          auto row = *it;

          if (row[m_columns.m_col_history]) {
            ustring query = row[m_columns.m_col_query];

            LOG (info) << "searches: saving query: " << query;
            save_query (query);

          } else {
            LOG (debug) << "searches: entry not a recent search.";
            return true;
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
            bool issaved  = row[m_columns.m_col_saved];
            bool ishistory = row[m_columns.m_col_history];

            ask_yes_no ("Do you want to delete the selected query?", [&, name, query, issaved, ishistory] (bool yes) {

                LOG (debug) << "yes";

                if (yes) {

                  bool changed = false;

                  if (issaved) {
                    ptree sa = load_searches ();
                    ptree  s = sa.get_child ("saved");

                    /* TODO: warning, this will delete the first occurence of the query */
                    for (auto it = s.begin (); it != s.end ();) {
                      if (it->second.data() == query) {
                        LOG (info) << "searches: deleting: " << name << ":" << query;

                        it = s.erase (it);
                        changed = true;
                        break;
                      } else {
                        it++;
                      }
                    }

                    sa.put_child ("saved", s);

                    if (changed) write_back_searches (sa);

                  } else if (ishistory) {
                    LOG (info) << "searches: deleting " << query;
                    for (auto it = history.rbegin (); it != history.rend (); it++) {
                      if (*it == query) {
                        history.erase (std::next(it).base ());
                        m_reload ();
                        break;
                      }
                    }
                  }

                  if (changed) {
                    /* select new row */
                    path.prev ();
                    Gtk::TreeIter it = store->get_iter (path);
                    if (it) {
                      tv.set_cursor (path);
                    } else {
                      tv.set_cursor (Gtk::TreePath ("1"));
                    }
                  }
                }
            });
          }

          return true;
        });

    keys.register_key ("C",
        "searches.clear_history",
        "Clear search history",
        [&] (Key) {

          LOG (info) << "searches: clearing search history..";

          ask_yes_no ("Do you want to clear the search history?", [&] (bool yes) {
              if (yes) {
                history.clear ();
                m_reload ();
              }
            });

          return true;
        });


    keys.register_key ("J",
        "searches.page_down",
        "Page down",
        [&] (Key) {
          if (store->children().size() >= 2) {

            Gtk::TreePath path;
            Gtk::TreeViewColumn *c;
            tv.get_cursor (path, c);

            for (int i = 0; i < page_jump_rows; i++) {
              if (!path) break;
              path.next ();
            }

            /* skip headers */
            while (path) {
              Gtk::TreeIter it = store->get_iter (path);

              if (!it) break; // we're at the end or something

              if (!(*it)[m_columns.m_col_description]) break;

              path.next ();
            }

            Gtk::TreeIter it = store->get_iter (path);

            if (it) {
              tv.set_cursor (path);
            } else {
              /* move to last */
              auto it = store->children().end ();
              auto p  = store->get_path (--it);
              if (p) tv.set_cursor (p);
            }
          }

          return true;
        });

    keys.register_key ("K",
        "searches.page_up",
        "Page up",
        [&] (Key) {
          Gtk::TreePath path;
          Gtk::TreeViewColumn *c;
          tv.get_cursor (path, c);
          for (int i = 0; i < page_jump_rows; i++) {
            if (!path) break;
            path.prev ();
          }

          /* skip headers */
          bool is_desc = true;
          while (path) {
            Gtk::TreeIter it = store->get_iter (path);
            if (!it) break; // beyond first
            if (!(*it)[m_columns.m_col_description]) {
              is_desc = false;
              break;
            }
            if (!path.prev ()) break; // probably at first
          }

          if (path && !is_desc) {
            tv.set_cursor (path);
          } else {
            /* move to first */
            auto p = Gtk::TreePath("1");
            if (p) tv.set_cursor (p);
          }

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

          if (!row[m_columns.m_col_description]) {
            ustring query = row[m_columns.m_col_query];
            ustring name  = row[m_columns.m_col_name];

            Mode * ti = new ThreadIndex (main_window, query, name);
            main_window->add_mode (ti);
          }

          return true;
        });

    keys.register_key ("!",
        "searches.show_all_history",
        "Show all history lines",
        [&] (Key) {

          show_all_history = true;
          reload ();
          return true;

        });

    /* }}} */

    reload ();
    tv.set_cursor (Gtk::TreePath("1"));

    tv.signal_row_activated ().connect (
        sigc::mem_fun (this, &SavedSearches::on_my_row_activated));

    SavedSearches::m_reload.connect (
        sigc::mem_fun (this, &SavedSearches::reload));

    astroid->actions->signal_thread_changed ().connect (
        sigc::mem_fun (this, &SavedSearches::on_thread_changed));

    astroid->actions->signal_refreshed ().connect (
        sigc::mem_fun (this, &SavedSearches::reload));
  }

  void SavedSearches::on_my_row_activated (
      const Gtk::TreeModel::Path &,
      Gtk::TreeViewColumn *) {

    Gtk::TreePath path;
    Gtk::TreeViewColumn *c;
    tv.get_cursor (path, c);
    Gtk::TreeIter iter;

    iter = store->get_iter (path);
    Gtk::ListStore::Row row = *iter;

    if (!row[m_columns.m_col_description]) {
      ustring query = row[m_columns.m_col_query];
      ustring name  = row[m_columns.m_col_name];

      Mode * ti = new ThreadIndex (main_window, query, name);
      main_window->add_mode (ti);
    }
  }


  void SavedSearches::reload () {
    Gtk::TreePath path;
    Gtk::TreeViewColumn *c;
    tv.get_cursor (path, c);

    store->clear ();
    load_startup_queries ();
    load_saved_searches ();
    refresh_stats ();

    tv.set_cursor (path);
  }

  void SavedSearches::add_query (ustring name, ustring query, bool saved, bool history) {
    auto iter = store->append();
    auto row  = *iter;

    row[m_columns.m_col_name] = name;
    row[m_columns.m_col_query] = query;
    row[m_columns.m_col_saved] = saved;
    row[m_columns.m_col_history] = history;
  }

  void SavedSearches::on_thread_changed (Db * db, ustring) {
    refresh_stats_db (db);
  }

  void SavedSearches::refresh_stats () {
    Db db;
    refresh_stats_db (&db);
  }

  void SavedSearches::refresh_stats_db (Db *db) {
    LOG (debug) << "searches: refreshing..";

    if (!main_window->is_current (this)) {
      LOG (debug) << "searches: skipping, not visible.";
      needs_refresh = true;
      return;
    }

    needs_refresh = false;

    for (auto row : store->children ()) {
      if (row[m_columns.m_col_description]) continue;

      ustring query = row[m_columns.m_col_query];

      unsigned int total_messages, unread_messages;
      notmuch_status_t st = NOTMUCH_STATUS_SUCCESS;

      /* get stats */
      notmuch_query_t * query_t =  notmuch_query_create (db->nm_db, query.c_str ());
      for (ustring & t : db->excluded_tags) {
        notmuch_query_add_tag_exclude (query_t, t.c_str());
      }
      notmuch_query_set_omit_excluded (query_t, NOTMUCH_EXCLUDE_TRUE);
      st = notmuch_query_count_messages (query_t, &total_messages); // destructive
      if (st != NOTMUCH_STATUS_SUCCESS) total_messages = 0;
      notmuch_query_destroy (query_t);

      ustring unread_q_s = "(" + query + ") AND tag:unread";
      notmuch_query_t * unread_q = notmuch_query_create (db->nm_db, unread_q_s.c_str());
      for (ustring & t : db->excluded_tags) {
        notmuch_query_add_tag_exclude (unread_q, t.c_str());
      }
      notmuch_query_set_omit_excluded (unread_q, NOTMUCH_EXCLUDE_TRUE);
      st = notmuch_query_count_messages (unread_q, &unread_messages); // destructive
      if (st != NOTMUCH_STATUS_SUCCESS) unread_messages = 0;
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

      /* LOG (info) << "saved searches: got query: " << name << ": " << query; */
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

      /* LOG (info) << "saved searches: got query: " << name << ": " << query; */
      add_query (name, query, true);
    }

    int history_lines = astroid->config ("saved_searches").get<int> ("history_lines_to_show");

    if (!show_all_history && history_lines == 0) return;

    /* load search history */
    iter = store->append();
    row  = *iter;

    row[m_columns.m_col_name] = "<b>Search history</b>";
    row[m_columns.m_col_description] = true;


    int i = 0;

    for (auto it = history.rbegin (); it != history.rend (); ++it) {
      if (!show_all_history && (!((history_lines < 0) || (i < history_lines))))
        break;

      add_query ("", *it, false, true);

      i++;
    }

  }

  std::vector<ustring> SavedSearches::get_history () {
    return history;
  }

  ptree SavedSearches::load_searches () {
    ptree s;

    if (is_regular_file (astroid->standard_paths().searches_file))
    {
      LOG (info) << "searches: loading saved searches..";
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
    LOG (info) << "searches: writing back saved searches..";
    write_json (astroid->standard_paths().searches_file.c_str (), s);
    m_reload ();
  }

  void SavedSearches::save_query (ustring q) {
    LOG (info) << "searches: adding query: " << q;

    ptree s = load_searches ();
    s.add ("saved.none", q);
    write_back_searches (s);
  }

  void SavedSearches::add_query_to_history (ustring q) {
    history.push_back (q);
    m_reload ();
  }

  void SavedSearches::init () {
    ptree h = load_searches ().get_child ("history");
    LOG (debug) << "searches: loading history..";

    for (auto &kv : h) {
      ustring name = kv.first;
      ustring query = kv.second.data();

      if (name == "none") name = "";

      /* LOG (debug) << "saved searches, history: got query: " << name << ": " << query; */
      history.push_back (query);
    }
  }

  void SavedSearches::destruct () {
    /* writing search history */
    bool save = astroid->config ("saved_searches").get<bool> ("save_history");
    unsigned int maxh = astroid->config ("saved_searches").get<unsigned int>  ("history_lines");

    if (save) {
      LOG (debug) << "searches: saving history..";
      ptree s = load_searches ();
      ptree h;

      if (history.size () > maxh) {
        history.erase (history.end () - maxh, history.end ());
      }

      for (auto &k : history) {
        h.add ("none", k);
      }

      s.put_child ("history", h);
      write_back_searches (s);
    }
  }

  void SavedSearches::grab_modal () {
    if (needs_refresh) refresh_stats ();
    add_modal_grab ();
    grab_focus ();
  }

  void SavedSearches::release_modal () {
    remove_modal_grab ();
  }



}


