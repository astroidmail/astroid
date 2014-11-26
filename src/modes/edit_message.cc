# include <iostream>
# include <vector>
# include <algorithm>
# include <random>
# include <ctime>
# include <memory>

# include <gtkmm.h>
# include <gdk/gdkx.h>
# include <gtkmm/socket.h>

# include <boost/filesystem.hpp>

# include "astroid.hh"
# include "config.hh"
# include "account_manager.hh"
# include "edit_message.hh"
# include "compose_message.hh"
# include "db.hh"
# include "thread_view.hh"
# include "raw_message.hh"
# include "main_window.hh"
# include "message_thread.hh"
# include "utils/ustring_utils.hh"
# include "build_config.hh"
# include "log.hh"

using namespace std;
using namespace boost::filesystem;

namespace Astroid {
  int EditMessage::edit_id = 0;

  EditMessage::EditMessage (MainWindow * mw, ustring _to) :
    EditMessage (mw) {

    to = _to;

    /* reload message */
    prepare_message ();
    read_edited_message ();
  }

  EditMessage::EditMessage (MainWindow * mw) : Mode (mw, true) {
    editor_config = astroid->config->config.get_child ("editor");

    tmpfile_path = astroid->config->runtime_dir;

    set_label ("New message");

    path default_ui ("ui/edit-message.glade");

# ifdef PREFIX

    path ui = path(PREFIX) / path("share/astroid") / default_ui;

# else
    path ui (default_ui);
# endif

    if (!exists (ui)) {
      if (!exists (default_ui)) {
        log << error << "em: cannot find ui file." << endl;
        exit (1);
      } else {
        log << error << "em: cannot find ui in installed path (" << ui.c_str() << "), using local.." << endl;
        ui = default_ui;
      }
    }

    Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create_from_file(ui.c_str(), "box_message");

    builder->get_widget ("box_message", box_message);

    builder->get_widget ("from_combo", from_combo);
    builder->get_widget ("encryption_combo", encryption_combo);
    builder->get_widget ("reply_mode_combo", reply_mode_combo);
    builder->get_widget ("fields_revealer", fields_revealer);
    builder->get_widget ("reply_revealer", reply_revealer);
    builder->get_widget ("encryption_revealer", encryption_revealer);

    builder->get_widget ("editor_box", editor_box);
    /*
    builder->get_widget ("editor_rev", editor_rev);
    builder->get_widget ("thread_rev", thread_rev);
    */

    pack_start (*box_message, true, 5);

    /* set up message id and random server name for gvim */
    id = edit_id++;

    char hostname[1024];
    hostname[1023] = 0;
    gethostname (hostname, 1022);

    int pid = getpid ();

    msg_time = time(0);
    vim_server = UstringUtils::random_alphanumeric (10);

    vim_server = ustring::compose ("%2-astroid-%1-%3-%5@%4", id,
        msg_time, vim_server, hostname, pid);

    if (msg_id == "") {
      msg_id = vim_server;
    }

    log << info << "em: msg id: " << msg_id << endl;
    log << debug << "em: vim server name: " << vim_server << endl;

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
    editor_socket->set_can_focus (true);
    editor_socket->signal_plug_added ().connect (
        sigc::mem_fun(*this, &EditMessage::plug_added) );
    editor_socket->signal_plug_removed ().connect (
        sigc::mem_fun(*this, &EditMessage::plug_removed) );

    editor_socket->signal_realize ().connect (
        sigc::mem_fun(*this, &EditMessage::socket_realized) );

    //editor_rev->add (*editor_socket);
    editor_box->pack_start (*editor_socket, false, false, 2);

    thread_view = Gtk::manage(new ThreadView(main_window));
    thread_view->edit_mode = true;
    //thread_rev->add (*thread_view);
    editor_box->pack_start (*thread_view, false, false, 2);

    thread_view->signal_ready().connect (
        sigc::mem_fun (this, &EditMessage::on_tv_ready));

    thread_view->signal_element_action().connect (
        sigc::mem_fun (this, &EditMessage::on_element_action));

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

    /* encryption combobox */
    enc_store = Gtk::ListStore::create (enc_columns);
    encryption_combo->set_model (enc_store);

    auto row = *(enc_store->append());
    row[enc_columns.encryption_string] = "None";
    row[enc_columns.encryption] = Enc_None;
    row = *(enc_store->append());
    row[enc_columns.encryption_string] = "Sign";
    row[enc_columns.encryption] = Enc_Sign;
    row = *(enc_store->append());
    row[enc_columns.encryption_string] = "Encrypt";
    row[enc_columns.encryption] = Enc_Encrypt;
    row = *(enc_store->append());
    row[enc_columns.encryption_string] = "Sign and encrypt";
    row[enc_columns.encryption] = Enc_SignAndEncrypt;

    encryption_combo->set_active (0);
    encryption_combo->pack_start (enc_columns.encryption_string);

    // TODO: encryption is not yet implemented.
    encryption_revealer->set_reveal_child (false);

    show_all_children ();

    prepare_message ();

    sending_in_progress.store (false);

    editor_toggle (false);

    from_combo->signal_changed().connect (
        sigc::mem_fun (this, &EditMessage::on_from_combo_changed));

    start_vim_on_socket_ready = true;
  }

  void EditMessage::on_from_combo_changed () {
    prepare_message ();
    read_edited_message ();
  }

  void EditMessage::prepare_message () {
    log << debug << "em: preparing message from fields.." << endl;

    auto iter = from_combo->get_active ();
    if (!iter) {
      log << warn << "em: error: no from account selected." << endl;
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
    tmpfile << "Subject: " << subject << endl;
    tmpfile << endl;
    tmpfile << body;

    tmpfile.close ();
  }

  EditMessage::~EditMessage () {
    log << debug << "em: deconstruct." << endl;

    if (vim_started) {
      vim_remote_keys ("<ESC>:quit!<CR>");
    }

    if (is_regular_file (tmpfile_path)) {
      boost::filesystem::remove (tmpfile_path);
    }
  }

  void EditMessage::socket_realized ()
  {
    log << debug << "em: socket realized." << endl;
    socket_ready = true;

    if (start_vim_on_socket_ready) {
      editor_toggle (true);
      activate_field (Editor);
      start_vim_on_socket_ready = false;
    }
  }

  void EditMessage::plug_added () {
    log << debug << "em: gvim connected" << endl;

    vim_ready = true;

    if (current_field == Editor) {
      activate_field (Editor);
    }
  }

  bool EditMessage::plug_removed () {
    log << debug << "em: gvim disconnected" << endl;
    vim_ready   = false;
    vim_started = false;
    editor_toggle (false);

    activate_field (Thread);

    return true;
  }


  void EditMessage::set_from (Address a) {
    if (!accounts->is_me (a)) {
      cerr << "em: from address is not a defined account." << endl;
    }

    set_from (accounts->get_account_for_address (a));
  }

  void EditMessage::set_from (Account * a) {
    for (Gtk::TreeRow row : from_store->children ()) {
      if (row[from_columns.account] == a) {
        from_combo->set_active (row);
        break;
      }
    }
  }

  void EditMessage::read_edited_message () {
    /* make message */
    ComposeMessage * c = make_message ();

    if (c == NULL) {
      log << error << "err: could not make message." << endl;
      return;
    }

    /* set account selector to from address email */
    set_from (c->account);

    to = c->to;
    cc = c->cc;
    bcc = c->bcc;
    subject = c->subject;
    body = ustring(c->body.str());

    ustring tmpf = c->write_tmp ();

    auto msgt = refptr<MessageThread>(new MessageThread());
    msgt->add_message (tmpf);
    thread_view->load_message_thread (msgt);

    delete c;

    unlink (tmpf.c_str());
  }

  void EditMessage::on_tv_ready () {
    log << debug << "em: got tv ready." << endl;

    if (warning_str.length() > 0)
      thread_view->set_warning (thread_view->focused_message, warning_str);
    else
      thread_view->hide_warning (thread_view->focused_message);

    if (info_str.length () > 0)
      thread_view->set_info (thread_view->focused_message, info_str);
    else
      thread_view->hide_info (thread_view->focused_message);
  }

  void EditMessage::fields_hide () {
    fields_revealer->set_reveal_child (false);
  }

  void EditMessage::fields_show () {
   fields_revealer->set_reveal_child (true);
  }

  /* turn on or off the editor or set up for the editor */
  void EditMessage::editor_toggle (bool on) {
    log << debug << "em: editor toggle: " << on << endl;

    if (on) {
      prepare_message ();

      current_field = Editor;

      /*
      editor_rev->set_reveal_child (true);
      thread_rev->set_reveal_child (false);
      */

      editor_socket->show ();
      thread_view->hide ();

      gtk_box_set_child_packing (editor_box->gobj (), GTK_WIDGET(editor_socket->gobj ()), true, true, 5, GTK_PACK_START);
      gtk_box_set_child_packing (editor_box->gobj (), GTK_WIDGET(thread_view->gobj ()), false, false, 5, GTK_PACK_START);

      /* future Gtk
      editor_box->set_child_packing (editor_rev, true, true, 2);
      editor_box->set_child_packing (thread_rev, false, false, 2);
      */

      fields_hide ();

      if (!vim_started) {
        vim_start ();
      }

    } else {

      current_field = Thread;

      if (vim_started) {
        vim_stop ();
      }

      /*
      editor_rev->set_reveal_child (false);
      thread_rev->set_reveal_child (true);
      */
      editor_socket->hide ();
      thread_view->show ();

      gtk_box_set_child_packing (editor_box->gobj (), GTK_WIDGET(editor_socket->gobj ()), false, false, 5, GTK_PACK_START);
      gtk_box_set_child_packing (editor_box->gobj (), GTK_WIDGET(thread_view->gobj ()), true, true, 5, GTK_PACK_START);

      /* future Gtk
      editor_box->set_child_packing (editor_rev, false, false, 2);
      editor_box->set_child_packing (thread_rev, true, true, 2);
      */

      fields_show ();

      read_edited_message ();
    }
  }

  void EditMessage::activate_field (Field f) {
    log << debug << "em: activate field: " << f << endl;

    current_field = f;

    reset_fields ();

    if (f == Editor) {
      // Gtk::IconSize isize  (Gtk::BuiltinIconSize::ICON_SIZE_BUTTON);

      log << debug << "em: activate editor." << endl;

      if (!socket_ready) {
        log << warn << "em: activate editor: socket not ready." << endl;
        return;
      }

      /*
       * https://bugzilla.gnome.org/show_bug.cgi?id=729248
       */

      release_modal ();

      if (vim_ready) {

        editor_socket->child_focus (Gtk::DIR_TAB_FORWARD);

      } else {

        log << warn << "em: activate editor, editor not yet started!" << endl;

      }

    } else if (f == Thread) {
      log << debug << "em: focus thread view" << endl;

      grab_modal ();

      thread_view->grab_focus ();

    } else if (f == From) {
      //from_combo->set_sensitive (true);
      from_combo->grab_focus ();

    } else if (f == Encryption) {
      encryption_combo->grab_focus ();
    }

    /* update tab in case something changed */
    if (subject.size () > 0) {
      set_label ("New message: " + subject);
    }
  }

  void EditMessage::reset_fields () {
    if (current_field == Editor) {


    } else {

    }
  }

  bool EditMessage::on_key_press_event (GdkEventKey * event) {
    log << debug << "em: got key press" << endl;

    if (mode_key_handler (event)) return true;

    switch (event->keyval) {
      case GDK_KEY_Return:
        {
          if (!message_sent && !sending_in_progress.load()) {
            editor_toggle (true);
            activate_field (Editor);
          }
          return true;
        }

      case GDK_KEY_y:
        {
          if (!message_sent && !sending_in_progress.load()) {
            ask_yes_no ("Really send message?", [&](bool yes){ if (yes) send_message (); });
          }
          return true;
        }

      case GDK_KEY_V:
        {
          /* view raw source of to be sent message */
          ComposeMessage * c = make_message ();
          ustring tmpf = c->write_tmp ();

          main_window->add_mode (new RawMessage (main_window, tmpf.c_str()));

          delete c;

          unlink (tmpf.c_str());

          return true;
        }

      case GDK_KEY_a:
        {
          /* attach file */
          attach_file ();
          return true;
        }

      case GDK_KEY_x:
        {
          if (sending_in_progress.load ()) {
            /* block closing the window while sending */
            return true;
          } else if (!message_sent) {
            ask_yes_no ("Do you want to close this message? (any changes will be lost)", [&](bool yes){ if (yes) { main_window->close_page(); } });
            return true;
          } else {
            // message has been sent successfully, no need to complain.
            return false;
          }
        }
    }

    return false;
  }

  ModeHelpInfo * EditMessage::key_help () {
    ModeHelpInfo * m = new ModeHelpInfo ();

    m->parent   = Mode::key_help ();
    m->toplevel = false;
    m->title    = "Edit message";

    m->keys = {
      { "Return", "Edit message in editor" },
      { "y", "Send message" },
      { "a", "Attach file" },
      { "d", "Remove attached file" },
    };

    ModeHelpInfo * tv = thread_view->key_help ();
    tv->parent = m;

    return tv;
  }

  bool EditMessage::check_fields () {
    /* TODO: check fields.. */
    return true;
  }

  bool EditMessage::send_message () {
    log << info << "em: sending message.." << endl;

    info_str = "sending message..";
    warning_str = "";
    on_tv_ready ();

    if (!check_fields ()) {
      log << error << "em: error problem with some of the input fields.." << endl;
      return false;
    }

    /* load body */
    editor_toggle (false);
    ComposeMessage * c = make_message ();

    if (c == NULL) return false;

    c->message_sent().connect (
        sigc::mem_fun (this, &EditMessage::send_message_finished));

    fields_hide ();
    sending_in_progress.store (true);
    c->send_threaded ();

    sending_message = c;

    return true;
  }

  void EditMessage::send_message_finished (bool result_from_sender) {
    log << info << "em: message sending done." << endl;
    if (result_from_sender) {
      info_str = "message sent successfully!";
      warning_str = "";
      lock_message_after_send ();
    } else {
      warning_str = "message could not be sent!";
      info_str = "";
      fields_show ();
    }

    sending_in_progress.store (false);

    delete sending_message;

    on_tv_ready ();
  }

  void EditMessage::lock_message_after_send () {
    message_sent = true;

    fields_hide ();
  }

  ComposeMessage * EditMessage::make_message () {

    if (!check_fields ()) {
      log << error << "em: error, problem with some of the input fields.." << endl;
      return NULL;
    }

    ComposeMessage * c = new ComposeMessage ();

    /*
    auto iter = from_combo->get_active ();
    if (!iter) {
      log << error << "em: error: no from account selected." << endl;
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

    c->set_references (references);
    c->set_inreplyto (inreplyto);

    for (shared_ptr<ComposeMessage::Attachment> a : attachments) {
      c->add_attachment (a);
    }

    c->build ();
    c->finalize ();

    return c;
  }

  void EditMessage::on_element_action (int id, char action) {

    if (sending_in_progress.load ()) return;

    if (action == 'd') {
      /* delete attachment */
      log << info << "em: remove attachment: " << id << endl;

      /* TODO: this will not always correspond to the attachment number! */
      attachments.erase (attachments.begin() + (id-1));

      prepare_message ();
      read_edited_message ();

    }
  }

  void EditMessage::vim_remote_keys (ustring keys) {
    ustring cmd = ustring::compose ("%1 --servername %2 --remote-send %3", gvim_cmd, vim_server, keys);
    log << info << "em: to vim: " << cmd << endl;
    if (!vim_started) {
      log << error << "em: to vim: error, vim not started." << endl;
    } else {
      Glib::spawn_command_line_async (cmd.c_str());
    }
  }

  void EditMessage::vim_remote_files (ustring files) {
    ustring cmd = ustring::compose ("%1 --servername %2 --remote %3", gvim_cmd, vim_server, files);
    log << info << "em: to vim: " << cmd << endl;
    if (!vim_started) {
      log << error << "em: to vim: error, vim not started." << endl;
    } else {
      Glib::spawn_command_line_async (cmd.c_str());
    }
  }

  void EditMessage::vim_remote_expr (ustring expr) {
    ustring cmd = ustring::compose ("%1 --servername %2 --remote-expr %3", gvim_cmd, vim_server, expr);
    log << info << "em: to vim: " << cmd << endl;
    if (!vim_started) {
      log << error << "em: to vim: error, vim not started." << endl;
    } else {
      Glib::spawn_command_line_async (cmd.c_str());
    }
  }

  void EditMessage::vim_start () {
    if (socket_ready) {
      ustring cmd = ustring::compose ("%1 -geom 10x10 --servername %3 --socketid %4 %2 %5",
          gvim_cmd, gvim_args, vim_server, editor_socket->get_id (),
          tmpfile_path.c_str());
      log << info << "em: starting gvim: " << cmd << endl;
      Glib::spawn_command_line_async (cmd.c_str());
      vim_started = true;
    } else {
      start_vim_on_socket_ready = true; // TODO: not thread-safe
      log << debug << "em: gvim, waiting for socket.." << endl;
    }
  }

  void EditMessage::vim_stop () {
    vim_remote_keys (ustring::compose("<ESC>:b %1<CR>", tmpfile_path.c_str()));
    vim_remote_keys ("<ESC>:wq!<CR>");
    vim_started = false;
  }

  void EditMessage::make_tmpfile () {
    tmpfile_path = tmpfile_path / path(msg_id);

    log << info << "em: tmpfile: " << tmpfile_path << endl;

    if (!is_directory(astroid->config->runtime_dir)) {
      log << warn << "em: making runtime dir.." << endl;
      create_directories (astroid->config->runtime_dir);
    }

    if (is_regular_file (tmpfile_path)) {
      log << error << "em: error: tmpfile already exists!" << endl;
      throw runtime_error ("em: tmpfile already exists!");
    }

    tmpfile.open (tmpfile_path.c_str(), fstream::out);

    if (tmpfile.fail()) {
      log << error << "em: error: could not create tmpfile!" << endl;
      throw runtime_error ("em: coult not create tmpfile!");
    }

    tmpfile.close ();
  }

  void EditMessage::attach_file () {
    log << info << "em: attach file.." << endl;

    Gtk::FileChooserDialog dialog ("Choose file to attach..",
        Gtk::FILE_CHOOSER_ACTION_OPEN);

    dialog.add_button ("_Cancel", Gtk::RESPONSE_CANCEL);
    dialog.add_button ("_Select", Gtk::RESPONSE_OK);
    dialog.set_select_multiple (true);

    int result = dialog.run ();

    switch (result) {
      case (Gtk::RESPONSE_OK):
        {
          vector<string> fnames = dialog.get_filenames ();
          for (string &fname : fnames) {
            path p (fname.c_str());

            if (!is_regular (p)) {
              log << error << "em: attach: file is not regular: " << p.c_str() << endl;
            } else {
              log << info << "em: attaching file: " << p.c_str() << endl;
              add_attachment (new ComposeMessage::Attachment (p));
            }
          }

          prepare_message ();
          read_edited_message ();

          break;
        }

      default:
        {
          log << debug << "em: attach: cancelled." << endl;
        }
    }
  }

  void EditMessage::add_attachment (ComposeMessage::Attachment * a) {
    if (a->valid) {
      attachments.push_back (shared_ptr<ComposeMessage::Attachment> (a));
    } else {
      log << error << "em: invalid attachment, not adding: " << a->name << endl;
      delete a;
    }
  }

  void EditMessage::grab_modal () {
    if (current_field != Editor) add_modal_grab ();
  }

  void EditMessage::release_modal () {
    remove_modal_grab ();
  }

}

