# include <iostream>
# include <vector>
# include <algorithm>
# include <random>
# include <ctime>

# include <gtkmm.h>
# include <gdk/gdkx.h>
# include <gtkmm/socket.h>

# include "astroid.hh"
# include "config.hh"
# include "account_manager.hh"
# include "edit_message.hh"
# include "contacts.hh"
# include "compose_message.hh"
# include "db.hh"
# include "thread_view.hh"
# include "message_thread.hh"

using namespace std;

namespace Astroid {
  int EditMessage::edit_id = 0;

  EditMessage::EditMessage () {
    editor_config = astroid->config->config.get_child ("editor");

    tmpfile_path = astroid->config->runtime_dir;

    tab_widget = new Gtk::Label ("New message");

    Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create_from_file("ui/edit-message.glade", "box_message");

    builder->get_widget ("box_message", box_message);

    builder->get_widget ("from_combo", from_combo);
    builder->get_widget ("encryption_combo", encryption_combo);

    builder->get_widget ("editor_box", editor_box);

    pack_start (*box_message, true, 5);

    /* set up message id and random server name for gvim */
    id = edit_id++;

    char hostname[1024];
    hostname[1024] = 0;
    gethostname (hostname, 1023);

    int pid = getpid ();

    msg_time = time(0);
    const string _chars = "abcdefghijklmnopqrstuvwxyz1234567890";
    random_device rd;
    mt19937 g(rd());

    int len = 10;
    for (int i = 0; i < len; i++)
      vim_server += _chars[g() % _chars.size()];

    vim_server = ustring::compose ("%2-astroid-%1-%3-%5@%4", id,
        msg_time, vim_server, hostname, pid);

    if (msg_id == "") {
      msg_id = vim_server;
    }

    cout << "em: msg id: " << msg_id << endl;
    cout << "em: vim server name: " << vim_server << endl;

    make_tmpfile ();

    /* gvim settings */
    gvim_cmd                = editor_config.get <string>("gvim.cmd");
    gvim_args               = editor_config.get <string>("gvim.args");

    /* gtk::socket:
     * http://stackoverflow.com/questions/13359699/pyside-embed-vim
     * https://developer.gnome.org/gtkmm-tutorial/stable/sec-plugs-sockets-example.html.en
     * https://mail.gnome.org/archives/gtk-list/2011-January/msg00041.html
     */
    editor_socket = Gtk::manage(new Gtk::Socket ());
    editor_socket->signal_plug_added ().connect (
        sigc::mem_fun(*this, &EditMessage::plug_added) );
    editor_socket->signal_plug_removed ().connect (
        sigc::mem_fun(*this, &EditMessage::plug_removed) );

    editor_socket->signal_realize ().connect (
        sigc::mem_fun(*this, &EditMessage::socket_realized) );

    show_all ();
    editor_box->pack_start (*editor_socket, false, false, 2);
    show_all ();

    thread_view = Gtk::manage(new ThreadView());
    //text_view->set_sensitive (false);
    //text_view->set_editable (false);
    editor_box->pack_start (*thread_view, true, 2);
    thread_view->hide ();


    /* defaults */
    accounts = astroid->accounts;

    /* from combobox */
    from_store = Gtk::ListStore::create (from_columns);
    from_combo->set_model (from_store);

    account_no = 0;
    for (Account &a : accounts->accounts) {
      auto row = *(from_store->append ());
      row[from_columns.name_and_address] = a.full_address();
      row[from_columns.account] = &a;

      if (a.isdefault) {
        from_combo->set_active (account_no);
      }

      account_no++;
    }

    from_combo->pack_start (from_columns.name_and_address);

    prepare_message ();

    activate_field (From);
    editor_toggle (false);
  }

  void EditMessage::prepare_message () {
    cout << "em: preparing message from fields.." << endl;

    auto iter = from_combo->get_active ();
    if (!iter) {
      cout << "em: error: no from account selected." << endl;
      return;
    }

    auto row = *iter;
    Account * a = row[from_columns.account];
    auto from_ia = internet_address_mailbox_new (a->name.c_str(), a->email.c_str());
    ustring from = internet_address_to_string (from_ia, true);

    tmpfile.open (tmpfile_path.c_str(), fstream::out);
    tmpfile << "From: " << from << endl;
    tmpfile << "To: " << to << endl;
    tmpfile << "Cc: " << cc << endl;
    if (bcc.size() > 0) {
      tmpfile << "Bcc: " << bcc << endl;
    }
    tmpfile << "Subject: " << endl;
    tmpfile << endl;
    tmpfile << body;

    tmpfile.close ();
  }

  EditMessage::~EditMessage () {
    cout << "em: deconstruct." << endl;

    vim_remote_keys ("<ESC>:quit!<CR>");

    if (is_regular_file (tmpfile_path)) {
      boost::filesystem::remove (tmpfile_path);
    }
  }

  void EditMessage::socket_realized ()
  {
    cout << "em: socket realized." << endl;
    socket_ready = true;
  }

  void EditMessage::plug_added () {
    cout << "em: gvim connected" << endl;
    gtk_box_set_child_packing (editor_box->gobj (), GTK_WIDGET(editor_socket->gobj ()), true, true, 5, GTK_PACK_END);

    if (current_field == Editor) {
      /* vim has been started with user input */
      activate_field (current_field);
    }
  }

  bool EditMessage::plug_removed () {
    cout << "em: gvim disconnected" << endl;
    vim_started = false;
    editor_toggle (false);

    /* reset edit state */
    if (current_field == Editor) {
      in_edit = false;
      activate_field (Editor);
    }

    return true;
  }

  /* turn on or off the editor or set up for the editor */
  void EditMessage::editor_toggle (bool on) {
    if (on) {
      editor_socket->show ();
      thread_view->hide ();

      if (!vim_started) {
        vim_start ();
      }

    } else {
      if (vim_started) {
        vim_stop ();
      }

      editor_socket->hide ();
      auto msgt = refptr<MessageThread>(new MessageThread());

      thread_view->show_all ();

      /* make message */
      ComposeMessage * c = make_message ();

      if (c == NULL) {
        cout << "err: could not make message." << endl;
        return;
      }

      /* set account selector to from address email */
      Account * account = c->account;
      for (Gtk::TreeRow row : from_store->children ()) {
        //auto row = *iter;
        if (row[from_columns.account] == account) {
          from_combo->set_active (row);
          break;
        }
      }

      to = c->to;
      cc = c->cc;
      bcc = c->bcc;
      subject = c->subject;
      body = ustring(c->body.str());

      ustring tmpf = c->write_tmp ();
      msgt->add_message (tmpf);
      thread_view->load_message_thread (msgt);

      delete c;

      unlink (tmpf.c_str());
    }
  }

  void EditMessage::activate_field (Field f) {
    cout << "em: activate field: " << f << endl;
    reset_fields ();

    current_field = f;

    if (f == Editor) {
      Gtk::IconSize isize  (Gtk::BuiltinIconSize::ICON_SIZE_BUTTON);

      cout << "em: focus editor." << endl;
      if (!socket_ready) return;

      prepare_message ();

      if (in_edit) {
        editor_socket->set_sensitive (true);
        editor_socket->set_can_focus (true);

        /* TODO: this should probably only be done the first time,
         *       in subsequent calls it should be sufficient to
         *       just do grab_focus(). it probably works because
         *       gvim only has one widget.
         *
         * Also, this requires GTK to be patched as in:
         * https://bugzilla.gnome.org/show_bug.cgi?id=729248
         * https://mail.gnome.org/archives/gtk-list/2014-May/msg00000.html
         *
         * as far as I can see, there are no other way to
         * programatically set focus to a widget inside an embedded
         * child (except for the user to press Tab).
         *
         * TODO: ship, as back up, the necessary code from gtk+
         *       to move focus to do gtk_socket_focus_forward.
         */
        if (vim_started) {
# ifdef HAVE_GTK_SOCKET_FOCUS_FORWARD
          gtk_socket_focus_forward (editor_socket->gobj ());
# endif

          editor_active = true;

        } else {

          editor_toggle (true);

        }
      }


    } else if (f == From) {
      //from_combo->set_sensitive (true);
      from_combo->grab_focus ();
    } else if (f == Encryption) {
      encryption_combo->grab_focus ();
    }

    /* update tab in case something changed */
    if (subject.size () > 0) {
      ((Gtk::Label*)tab_widget)->set_text ("New message: " + subject);
    }
  }

  void EditMessage::reset_fields () {

    /* reset from combo */
    from_combo->popdown ();
    encryption_combo->popdown ();
    //from_combo->set_sensitive (false);

    /* reset editor */
    editor_active = false;
    //editor_socket->set_sensitive (false);
    editor_socket->set_can_focus (false);

    if (!in_edit) {
      add_modal_grab ();
    } else {
      remove_modal_grab ();
    }
  }

  bool EditMessage::on_key_press_event (GdkEventKey * event) {
    cout << "em: got key press" << endl;
    switch (event->keyval) {
      case GDK_KEY_Return:
        {
          if (!editor_active) {
            editor_active = true;
            in_edit       = true;
            activate_field (Editor);
            return true;
          }
        }
    }

    return false;
  }

  bool EditMessage::check_fields () {
    /* TODO: check fields.. */
    return true;
  }

  bool EditMessage::send_message () {
    cout << "em: sending message.." << endl;

    if (!check_fields ()) {
      cout << "em: error problem with some of the input fields.." << endl;
      return false;
    }

    /* load body */
    editor_toggle (false);
    ComposeMessage * c = make_message ();

    if (c == NULL) return false;

    bool success = c->send ();

    delete c;

    return success;
  }

  ComposeMessage * EditMessage::make_message () {

    if (!check_fields ()) {
      cout << "em: error, problem with some of the input fields.." << endl;
      return NULL;
    }

    ComposeMessage * c = new ComposeMessage ();

    /*
    auto iter = from_combo->get_active ();
    if (!iter) {
      cout << "em: error: no from account selected." << endl;
      return NULL;
    }
    c->set_id (msg_id);
    auto row = *iter;
    c->set_from (row[from_columns.account]);
    c->set_to (to);
    c->set_cc (cc);
    c->set_bcc (bcc);
    c->set_subject (subject);

    tmpfile.open (tmpfile_path.c_str(), fstream::in);
    c->body << tmpfile.rdbuf();
    tmpfile.close ();
    */
    c->load_message (msg_id, tmpfile_path.c_str());


    c->build ();
    c->finalize ();

    return c;
  }

  void EditMessage::vim_remote_keys (ustring keys) {
    ustring cmd = ustring::compose ("%1 --servername %2 --remote-send %3", gvim_cmd, vim_server, keys);
    cout << "em: to vim: " << cmd << endl;
    if (!vim_started) {
      cout << "em: to vim: error, vim not started." << endl;
    } else {
      Glib::spawn_command_line_async (cmd.c_str());
    }
  }

  void EditMessage::vim_remote_files (ustring files) {
    ustring cmd = ustring::compose ("%1 --servername %2 --remote %3", gvim_cmd, vim_server, files);
    cout << "em: to vim: " << cmd << endl;
    if (!vim_started) {
      cout << "em: to vim: error, vim not started." << endl;
    } else {
      Glib::spawn_command_line_async (cmd.c_str());
    }
  }

  void EditMessage::vim_remote_expr (ustring expr) {
    ustring cmd = ustring::compose ("%1 --servername %2 --remote-expr %3", gvim_cmd, vim_server, expr);
    cout << "em: to vim: " << cmd << endl;
    if (!vim_started) {
      cout << "em: to vim: error, vim not started." << endl;
    } else {
      Glib::spawn_command_line_async (cmd.c_str());
    }
  }

  void EditMessage::vim_start () {
    gtk_box_set_child_packing (editor_box->gobj (), GTK_WIDGET(editor_socket->gobj ()), false, false, 5, GTK_PACK_END);
    ustring cmd = ustring::compose ("%1 -geom 10x10 --servername %3 --socketid %4 %2 %5",
        gvim_cmd, gvim_args, vim_server, editor_socket->get_id (),
        tmpfile_path.c_str());
    cout << "em: starting gvim: " << cmd << endl;
    Glib::spawn_command_line_async (cmd.c_str());
    vim_started = true;
  }

  void EditMessage::vim_stop () {
    vim_remote_keys (ustring::compose("<ESC>:b %1<CR>", tmpfile_path.c_str()));
    vim_remote_keys ("<ESC>:wq!<CR>");
    vim_started = false;
  }

  void EditMessage::make_tmpfile () {
    tmpfile_path = tmpfile_path / path(msg_id);

    cout << "em: tmpfile: " << tmpfile_path << endl;

    if (!is_directory(astroid->config->runtime_dir)) {
      cout << "em: making runtime dir.." << endl;
      create_directories (astroid->config->runtime_dir);
    }

    if (is_regular_file (tmpfile_path)) {
      cout << "em: error: tmpfile already exists!" << endl;
      exit (1);
    }

    tmpfile.open (tmpfile_path.c_str(), fstream::out);

    if (tmpfile.fail()) {
      cout << "em: error: could not create tmpfile!" << endl;
      exit (1);
    }

    tmpfile.close ();
  }

  void EditMessage::grab_modal () {
    add_modal_grab ();
  }

  void EditMessage::release_modal () {
    remove_modal_grab ();
  }

}

