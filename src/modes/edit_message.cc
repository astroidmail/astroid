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
# include "thread_view/thread_view.hh"
# include "raw_message.hh"
# include "main_window.hh"
# include "message_thread.hh"
# include "chunk.hh"
# include "utils/utils.hh"
# include "utils/ustring_utils.hh"
# include "utils/resource.hh"
# include "actions/onmessage.hh"

# include "editor/plugin.hh"

using namespace std;
using namespace boost::filesystem;

namespace Astroid {
  int EditMessage::edit_id = 0;

  EditMessage::EditMessage (MainWindow * mw, ustring _to) :
    EditMessage (mw) { // {{{

    to = _to;

    /* reload message */
    prepare_message ();
    read_edited_message ();
  }

  EditMessage::EditMessage (MainWindow * mw, refptr<Message> msg) :
    EditMessage (mw) {
    /* load draft */
    LOG (info) << "em: loading draft from: " << msg->fname;

    /* if this is a previously saved draft: use it, otherwise make new draft
     * based on original message */

    if (any_of (Db::draft_tags.begin (),
                Db::draft_tags.end (),
                [&](ustring t) {
                  return has (msg->tags, t);
                }))
    {
      draft_msg = msg;
      msg_id = msg->mid;
    }

    for (auto &c : msg->attachments ()) {
      add_attachment (new ComposeMessage::Attachment (c));
    }

    inreplyto = msg->inreplyto;
    references = msg->references;

    /* write msg to new tmpfile */
    msg->save_to (tmpfile_path.c_str ());

    /* reload message */
    read_edited_message ();
  }

  EditMessage::EditMessage (MainWindow * mw) : Mode (mw) {
    editor_config = astroid->config ("editor");

    save_draft_on_force_quit = editor_config.get <bool> ("save_draft_on_force_quit");

    tmpfile_path = astroid->standard_paths ().runtime_dir;

    set_label ("New message");

    path ui = Resource (false, "ui/edit-message.glade").get_path ();

    Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create_from_file(ui.c_str(), "box_message");

    builder->get_widget ("box_message", box_message);

    builder->get_widget ("from_combo", from_combo);
    builder->get_widget ("switch_encrypt", switch_encrypt);
    builder->get_widget ("switch_sign", switch_sign);
    builder->get_widget ("reply_mode_combo", reply_mode_combo);
    builder->get_widget ("fields_revealer", fields_revealer);
    builder->get_widget ("reply_revealer", reply_revealer);
    builder->get_widget ("encryption_revealer", encryption_revealer);

    builder->get_widget ("editor_box", editor_box);
    builder->get_widget ("switch_signature", switch_signature);
    /*
    builder->get_widget ("editor_rev", editor_rev);
    builder->get_widget ("thread_rev", thread_rev);
    */

    pack_start (*box_message, true, 5);

    /* set up message id and random server name for editor */
    id = edit_id++;

    ustring _mid = "";
    msg_time = time(0);

# ifndef DISABLE_PLUGINS
    if (!astroid->plugin_manager->astroid_extension->generate_mid (_mid)) {
# endif

      char _hostname[1024];
      _hostname[1023] = 0;
      gethostname (_hostname, 1023);

      char _domainname[1024];
      _domainname[1023] = 0;
      getdomainname (_domainname, 1023);

      ustring hostname = astroid->config ().get <string> ("mail.message_id_fqdn");
      UstringUtils::trim (hostname);
      if (hostname.empty ()) {
        if (*_hostname != 0) {
          hostname = _hostname;

          if (*_domainname != 0) {
            ustring d (_domainname);
            d = UstringUtils::replace (d, "(", ""); // often (none) is returned
            d = UstringUtils::replace (d, ")", "");
            hostname += ".";
            hostname += d;
          }

          if (hostname.find (".", 0) == std::string::npos) {
            /* add a top level domain */
            hostname += ".none";
          }
        } else {
          hostname = UstringUtils::random_alphanumeric (10) + ".none";
        }
      }


      ustring user = astroid->config ().get<string> ("mail.message_id_user");
      UstringUtils::trim (user);
      if (user.empty ()) user = "astroid";

      _mid = UstringUtils::random_alphanumeric (10);

      _mid = ustring::compose ("%1.%2.%3@%4", msg_time, _mid, user, hostname);

# ifndef DISABLE_PLUGINS
    }
# endif

    if (msg_id == "") {
      msg_id = _mid;
    }

    LOG (info) << "em: msg id: " << msg_id;

    make_tmpfile ();

    editor = new Plugin (this, _mid);

    //editor_rev->add (*editor_socket);
    editor_box->pack_start (*editor, false, false, 2);

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
    /*
    from_combo->signal_key_press_event ().connect (
        sigc::mem_fun (this, &EditMessage::on_from_combo_key_press));
        */



    show_all_children ();

    prepare_message ();

    sending_in_progress.store (false);

    editor_toggle (false);

    from_combo->signal_changed().connect (
        sigc::mem_fun (this, &EditMessage::on_from_combo_changed));

    /* editor->start_editor_when_ready = true; */

    switch_signature->property_active().signal_changed ().connect (
        sigc::mem_fun (*this, &EditMessage::switch_signature_set));

    switch_encrypt->property_active().signal_changed ().connect (
        sigc::mem_fun (*this, &EditMessage::switch_signature_set));

    switch_sign->property_active().signal_changed ().connect (
        sigc::mem_fun (*this, &EditMessage::switch_signature_set));

    reset_signature ();

    /* register keys {{{ */
    keys.title = "Edit mode";
    keys.register_key (Key (GDK_KEY_Return), { Key (GDK_KEY_KP_Enter) },
        "edit_message.edit",
        "Edit message in editor",
        [&] (Key) {
          if (!message_sent && !sending_in_progress.load()) {
            editor_toggle (true);
          }
          return true;
        });

    keys.register_key ("y", "edit_message.send",
        "Send message",
        [&] (Key) {
          if (!message_sent && !sending_in_progress.load()) {
            ask_yes_no ("Really send message?", [&](bool yes){ if (yes) send_message (); });
          }
          return true;
        });

    keys.register_key ("C-c", "edit_message.cancel",
        "Cancel sending message (unreliable)",
        [&] (Key) {
          if (!message_sent && sending_in_progress.load ()) {
            sending_message->cancel_sending ();
          }

          return true;
        });

    keys.register_key ("V", "edit_message.view_raw",
        "View raw message",
        [&] (Key) {
          /* view raw source of to be sent message */
          ComposeMessage * c = make_message ();
          ustring tmpf = c->write_tmp ();

          main_window->add_mode (new RawMessage (main_window, tmpf.c_str(), true));
          /* tmp file deleted by RawMessage */

          delete c;

          return true;
        });

    keys.register_key ("f", "edit_message.cycle_from",
        "Cycle through From selector",
        [&] (Key) {
          /* cycle through from combo box */
          if (!message_sent && !sending_in_progress.load()) {
            if (from_store->children().size() > 1) {
              int i = from_combo->get_active_row_number ();
              if (i >= (static_cast<int>(from_store->children().size())-1)) i = 0;
              else i++;
              from_combo->set_active (i);
              reset_signature ();
            }
          }

          return true;
        });

    keys.register_key ("a", "edit_message.attach",
        "Attach file",
        [&] (Key) {
          attach_file ();
          return true;
        });

    keys.register_key ("s", "edit_message.save_draft",
        "Save draft",
        [&] (Key) {
          if (sending_in_progress.load ()) {
            /* block closing the window while sending */
            LOG (error) << "em: message is being sent, it cannot be saved as draft anymore.";
          } else {

            bool r;

            r = save_draft ();

            if (!r) {
              on_tv_ready ();
            } else {
              close (true);
            }
          }
          return true;
        });

    keys.register_key ("D", "edit_message.delete_draft",
        "Delete draft",
        [&] (Key) {
          if (!draft_msg) {
            LOG (debug) << "em: not a draft.";
            return true;
          }

          if (sending_in_progress.load ()) {
            /* block closing the window while sending */
            LOG (error) << "em: message is being sent, cannot delete draft now. it will be deleted upon successfully sent message.";
          } else if (!message_sent) {
            ask_yes_no ("Do you want to delete this draft and close it? (any changes will be lost)",
                [&](bool yes) {
                  if (yes) {
                    delete_draft ();
                    close (true);
                  }
                });
          } else {
            // message has been sent successfully, no need to complain.
            close ();
          }
          return true;
        });


    keys.register_key ("S", "edit_message.toggle_signature",
        "Toggle signature",
        [&] (Key) {
          auto iter = from_combo->get_active ();
          auto row = *iter;
          Account * a = row[from_columns.account];
          if (a->has_signature) {
            switch_signature->set_state (!switch_signature->get_active ());
          }
          return true;
        });

    keys.register_key ("E", "edit_message.toggle_encrypt",
        "Toggle encryption and signature",
        [&] (Key) {

          auto iter = from_combo->get_active ();
          if (!iter) {
            return true;
          }

          auto row = *iter;
          Account * a = row[from_columns.account];

          if (a->has_gpg) {
            if (!switch_encrypt->get_active () && !switch_sign->get_active ()) {
              switch_encrypt->set_active (false);
              switch_sign->set_active (true);
            } else if (!switch_encrypt->get_active() && switch_sign->get_active ()) {
              switch_encrypt->set_active (true);
              switch_sign->set_active (true);
            } else if (switch_encrypt->get_active () && switch_sign->get_active ()) {
              switch_encrypt->set_active (true);
              switch_sign->set_active (false);
            } else  {
              switch_encrypt->set_active (false);
              switch_sign->set_active (false);
            }
          }

          return true;
        });

    // }}}
  } // }}}

  EditMessage::~EditMessage () {
    LOG (debug) << "em: deconstruct.";

    if (status_icon_visible) {
      main_window->notebook.remove_widget (&message_sending_status_icon);
    }

    if (editor->started ()) {
      editor->stop ();
    }

    if (is_regular_file (tmpfile_path)) {
      boost::filesystem::remove (tmpfile_path);
    }
  }

  void EditMessage::close (bool force) {
    if (sending_in_progress.load ()) {
      /* block closing the window while sending */
    } else if (!force && !message_sent && !draft_saved) {
      ask_yes_no ("Do you want to close this message? (unsaved changes will be lost)", [&](bool yes){ if (yes) { Mode::close (force); } });
    } else if (force && !message_sent && !draft_saved && save_draft_on_force_quit) {
      LOG (warn) << "em: force quit, trying to save draft..";
      bool r = save_draft ();
      if (!r) {
        LOG (error) << "em: cannot save draft! check account config. changes will be lost.";
      }
      Mode::close (force);
    } else {
      // message has been sent successfully, no need to complain.
      Mode::close (force);
    }
  }

  /* drafts */
  bool EditMessage::save_draft () {
    LOG (info) << "em: saving draft..";
    draft_saved = false;
    ComposeMessage * c = make_message ();
    ustring fname;

    bool add_to_notmuch = false;

    if (!draft_msg) {
      /* make new message */

      path ddir = path(c->account->save_drafts_to.c_str ());
      if (!is_directory(ddir)) {
        LOG (error) << "em: no draft directory specified!";
        warning_str = "draft could not be saved, no suitable draft directory for account specified.";
        /* move to key handler:
         * warning_str = "draft could not be saved!"; */
        /* on_tv_ready (); */
        return false;

      } else {
        /* msg_id might come from external client or server */
        ddir = ddir / path(Utils::safe_fname (msg_id));
        fname = ddir.c_str ();

        add_to_notmuch = true;
      }
    } else {
      fname = draft_msg->fname; // overwrite
    }

    c->write (fname);

    LOG (info) << "em: saved draft to: " << fname;

    if (add_to_notmuch) {
      astroid->actions->doit (refptr<Action> (
            new AddDraftMessage (fname)));
    }

    draft_saved = true;
    return true;
  }

  void EditMessage::delete_draft () {
    if (draft_msg) {

      delete_draft (draft_msg);

      draft_msg = refptr<Message> ();
      draft_saved = true; /* avoid saving on exit */

    } else {
      LOG (warn) << "em: not a draft, not deleting.";
    }
  }

  void EditMessage::delete_draft (refptr<Message> draft_msg) {
    LOG (info) << "em: deleting draft.";
    path fname = path(draft_msg->fname.c_str());

    if (is_regular_file (fname)) {
      boost::filesystem::remove (fname);

      /* first remove tag in case it has been sent */
      astroid->actions->doit (refptr<Action>(
            new OnMessageAction (draft_msg->mid, draft_msg->tid,

              [fname] (Db * db, notmuch_message_t * msg) {
                for (ustring t : Db::draft_tags) {
                  notmuch_message_remove_tag (msg,
                      t.c_str ());
                }

                bool persists = !db->remove_message (fname.c_str ());

                if (persists && db->maildir_synchronize_flags) {
                  /* sync in case there are other copies of the message */
                  notmuch_message_tags_to_maildir_flags (msg);
                }
              })));
    }

  }

  /* edit / read message cycling {{{ */
  void EditMessage::on_from_combo_changed () {
    prepare_message ();
    read_edited_message ();
  }

  void EditMessage::prepare_message () {
    LOG (debug) << "em: preparing message from fields..";

    auto iter = from_combo->get_active ();
    if (!iter) {
      LOG (warn) << "em: error: no from account selected.";
      return;
    }

    auto row = *iter;
    Account * a = row[from_columns.account];
    auto from_ia = internet_address_mailbox_new (a->name.c_str(), a->email.c_str());
    ustring from = internet_address_to_string (from_ia, true);

    tmpfile.open (tmpfile_path.c_str(), std::fstream::out);
    tmpfile << "From: " << from << endl;
    tmpfile << "To: " << to << endl;
    tmpfile << "Cc: " << cc << endl;
    if (bcc.size() > 0) {
      tmpfile << "Bcc: " << bcc << endl;
    }
    tmpfile << "Subject: " << subject << endl;
    tmpfile << endl;
    tmpfile << body.raw();

    tmpfile.close ();
    LOG (debug) << "em: prepare message done.";
  }

  void EditMessage::set_from (Address a) {
    if (!accounts->is_me (a)) {
      LOG (error) << "em: from address is not a defined account.";
    }

    set_from (accounts->get_account_for_address (a));
  }

  void EditMessage::set_from (Account * a) {
    int rn = from_combo->get_active_row_number ();

    for (Gtk::TreeRow row : from_store->children ()) {
      if (row[from_columns.account] == a) {
        from_combo->set_active (row);
        break;
      }
    }

    bool same_account = (rn == from_combo->get_active_row_number ());
    LOG (debug) << "same account: " << same_account;
    if (!same_account) {
      reset_signature ();
    }
  }

  void EditMessage::reset_signature () {
    /* should not be run unless the account has been changed */
    auto it = from_combo->get_active ();
    Account * a = (*it)[from_columns.account];

    switch_signature->set_sensitive (a->has_signature);
    switch_signature->set_state (a->has_signature && a->signature_default_on);

    encryption_revealer->set_reveal_child (a->has_gpg);
    switch_sign->set_sensitive (a->has_gpg);
    switch_encrypt->set_sensitive (a->has_gpg);

    switch_encrypt->set_state (false);
    switch_sign->set_state (a->has_gpg && a->always_gpg_sign);
  }


  void EditMessage::switch_signature_set () {
    LOG (debug) << "got sig: " << switch_signature->get_active ();
    prepare_message ();
    read_edited_message ();
  }

  void EditMessage::read_edited_message () {
    /* make message */
    draft_saved = false; // we expect changes to have been made
    warning_str = "";
    if (astroid->hint_level () < 1) {
      info_str = "Edit message with 'Enter'.";
    } else {
      info_str = "";
    }

    ComposeMessage * c = make_message ();

    if (c == NULL) {
      LOG (error) << "err: could not make message.";
      return;
    }

    if (c->encrypt || c->sign) {
      if (!c->encryption_success) {
        warning_str = "Failed encrypting: " + UstringUtils::replace (c->encryption_error, "\n", "<br />");
      }
    }

    /* set account selector to from address email */
    set_from (c->account);

    to = c->to;
    cc = c->cc;
    bcc = c->bcc;
    references = c->references;
    inreplyto = c->inreplyto;
    subject = c->subject;
    body = ustring(c->body.str());

    ustring tmpf = c->write_tmp ();

    auto msgt = refptr<MessageThread>(new MessageThread());
    msgt->add_message (tmpf);
    thread_view->load_message_thread (msgt);

    delete c;

    unlink (tmpf.c_str());
  }

  /* }}} */

  void EditMessage::on_tv_ready () {
    LOG (debug) << "em: got tv ready.";

    if (warning_str.length() > 0)
      thread_view->set_warning (thread_view->focused_message, warning_str);
    else
      thread_view->hide_warning (thread_view->focused_message);

    if (info_str.length () > 0)
      thread_view->set_info (thread_view->focused_message, info_str);
    else
      thread_view->hide_info (thread_view->focused_message);
  }

  /* swapping between edit and read mode {{{ */
  void EditMessage::fields_hide () {
    fields_revealer->set_reveal_child (false);
  }

  void EditMessage::fields_show () {
   fields_revealer->set_reveal_child (true);
  }

  /* turn on or off the editor or set up for the editor */
  void EditMessage::editor_toggle (bool on) {
    LOG (debug) << "em: editor toggle: " << on;

    if (on) {
      prepare_message ();

      editor_active = true;

      /*
      editor_rev->set_reveal_child (true);
      thread_rev->set_reveal_child (false);
      */

      editor->show ();
      thread_view->hide ();

      gtk_box_set_child_packing (editor_box->gobj (), GTK_WIDGET(editor->gobj ()), true, true, 5, GTK_PACK_START);
      gtk_box_set_child_packing (editor_box->gobj (), GTK_WIDGET(thread_view->gobj ()), false, false, 5, GTK_PACK_START);

      /* future Gtk
      editor_box->set_child_packing (editor_rev, true, true, 2);
      editor_box->set_child_packing (thread_rev, false, false, 2);
      */

      fields_hide ();

      editor->start ();

    } else {

      editor_active = false;

      if (editor->started ()) {
        editor->stop ();
      }

      /*
      editor_rev->set_reveal_child (false);
      thread_rev->set_reveal_child (true);
      */
      editor->hide ();
      thread_view->show ();

      gtk_box_set_child_packing (editor_box->gobj (), GTK_WIDGET(editor->gobj ()), false, false, 5, GTK_PACK_START);
      gtk_box_set_child_packing (editor_box->gobj (), GTK_WIDGET(thread_view->gobj ()), true, true, 5, GTK_PACK_START);

      /* future Gtk
      editor_box->set_child_packing (editor_rev, false, false, 2);
      editor_box->set_child_packing (thread_rev, true, true, 2);
      */

      fields_show ();

      read_edited_message ();

      grab_modal ();
      thread_view->grab_focus ();
    }
  }

  void EditMessage::activate_editor () {
    LOG (debug) << "em: activate editor.";

    editor_active = true;

    LOG (debug) << "em: activate editor.";

    if (!editor->ready ()) {
      LOG (warn) << "em: activate editor: not ready.";
      return;
    }

    /*
     * https://bugzilla.gnome.org/show_bug.cgi?id=729248
     */

    release_modal ();

    if (editor->ready ()) {

      editor->focus ();

    } else {

      LOG (warn) << "em: activate editor, editor not yet started!";

    }
  }

  /* }}} */

  void EditMessage::on_element_action (int id, ThreadView::ElementAction action) {

    if (sending_in_progress.load ()) return;

    if (action == ThreadView::ElementAction::EDelete) {
      /* delete attachment */
      LOG (info) << "em: remove attachment: " << id;

      /* TODO: this will not always correspond to the attachment number! */
      attachments.erase (attachments.begin() + (id-1));

      prepare_message ();
      read_edited_message ();

    }
  }

  /* }}} */

  /* send message {{{ */
  bool EditMessage::check_fields () {
    /* TODO: check fields.. */
    return true;
  }

  bool EditMessage::send_message () {
    LOG (info) << "em: sending message..";

    info_str = "sending message..";
    warning_str = "";
    on_tv_ready ();

    if (!check_fields ()) {
      LOG (error) << "em: error problem with some of the input fields..";
      return false;
    }

    /* load body */
    editor_toggle (false); // resets warning and info

    info_str = "sending message..";
    on_tv_ready ();

    ComposeMessage * c = make_message ();

    if (c == NULL) return false;

    if (c->encrypt || c->sign) {
      if (!c->encryption_success) {
        warning_str = "Cannot send, failed encrypting: " + UstringUtils::replace (c->encryption_error, "\n", "<br />");
        return false;
      }
    }

    c->message_sent().connect (
        sigc::mem_fun (this, &EditMessage::send_message_finished));

    fields_hide ();
    sending_in_progress.store (true);

    /* set message sending icon */
    status_icon_visible = true;
    Glib::RefPtr<Gtk::IconTheme> theme = Gtk::IconTheme::get_default();
    Glib::RefPtr<Gdk::Pixbuf> pixbuf = theme->load_icon (
        "mail-send",
        Notebook::icon_size,
        Gtk::ICON_LOOKUP_USE_BUILTIN | Gtk::ICON_LOOKUP_FORCE_SIZE);
    message_sending_status_icon.set (pixbuf);

    main_window->notebook.add_widget (&message_sending_status_icon);

    c->send_threaded ();

    sending_message = c;

    return true;
  }

  void EditMessage::send_message_finished (bool result_from_sender) {
    LOG (info) << "em: message sending done.";
    status_icon_visible = true;

    Glib::RefPtr<Gtk::IconTheme> theme = Gtk::IconTheme::get_default();
    Glib::RefPtr<Gdk::Pixbuf> pixbuf;

    if (result_from_sender) {
      info_str = "message sent successfully!";
      warning_str = "";
      lock_message_after_send ();

      pixbuf = theme->load_icon (
          "gtk-apply",
          Notebook::icon_size,
          Gtk::ICON_LOOKUP_USE_BUILTIN | Gtk::ICON_LOOKUP_FORCE_SIZE);

      /* delete draft */
      if (draft_msg) {
        LOG (info) << "em: deleting draft: " << draft_msg->fname;
        delete_draft ();
      }

    } else {
      warning_str = "message could not be sent!";
      info_str = "";
      fields_show ();

      pixbuf = theme->load_icon (
         "dialog-error",
          Notebook::icon_size,
          Gtk::ICON_LOOKUP_USE_BUILTIN | Gtk::ICON_LOOKUP_FORCE_SIZE);
    }

    message_sending_status_icon.set (pixbuf);
    sending_in_progress.store (false);

    delete sending_message;

    on_tv_ready ();

    emit_message_sent_attempt (result_from_sender);
  }

  void EditMessage::lock_message_after_send () {
    message_sent = true;

    fields_hide ();
  }

  ComposeMessage * EditMessage::make_message () {

    if (!check_fields ()) {
      LOG (error) << "em: error, problem with some of the input fields..";
      return NULL;
    }

    ComposeMessage * c = new ComposeMessage ();

    c->load_message (msg_id, tmpfile_path.c_str());

    c->set_references (references);
    c->set_inreplyto (inreplyto);

    for (shared_ptr<ComposeMessage::Attachment> a : attachments) {
      c->add_attachment (a);
    }

    if (c->account->has_signature && switch_signature->get_active ()) {
      c->include_signature = true;
    } else {
      c->include_signature = false;
    }

    if (c->account->has_gpg) {
      c->encrypt = switch_encrypt->get_active ();
      c->sign    = switch_sign->get_active ();
    }

    c->build ();
    c->finalize ();

    return c;
  }

  /* }}} */

  void EditMessage::make_tmpfile () {
    tmpfile_path = tmpfile_path / path(msg_id);

    LOG (info) << "em: tmpfile: " << tmpfile_path;

    if (is_regular_file (tmpfile_path)) {
      LOG (error) << "em: error: tmpfile already exists!";
      throw runtime_error ("em: tmpfile already exists!");
    }

    tmpfile.open (tmpfile_path.c_str(), std::fstream::out);

    if (tmpfile.fail()) {
      LOG (error) << "em: error: could not create tmpfile!";
      throw runtime_error ("em: coult not create tmpfile!");
    }

    tmpfile.close ();
  }

  void EditMessage::attach_file () {
    LOG (info) << "em: attach file..";

    if (message_sent) {
      LOG (debug) << "em: message already sent.";
      return;
    }

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
              LOG (error) << "em: attach: file is not regular: " << p.c_str();
            } else {
              LOG (info) << "em: attaching file: " << p.c_str();
              add_attachment (new ComposeMessage::Attachment (p));
            }
          }

          prepare_message ();
          read_edited_message ();

          break;
        }

      default:
        {
          LOG (debug) << "em: attach: cancelled.";
        }
    }
  }

  void EditMessage::add_attachment (ComposeMessage::Attachment * a) {
    if (a->valid) {
      attachments.push_back (shared_ptr<ComposeMessage::Attachment> (a));
    } else {
      LOG (error) << "em: invalid attachment, not adding: " << a->name;
      delete a;
    }
  }

  void EditMessage::grab_modal () {
    if (!editor_active) add_modal_grab ();
  }

  void EditMessage::release_modal () {
    remove_modal_grab ();
  }

  /* message sent attempt signal */
  EditMessage::type_message_sent_attempt
    EditMessage::message_sent_attempt ()
  {
    return m_message_sent_attempt;
  }

  void EditMessage::emit_message_sent_attempt (bool res) {
    m_message_sent_attempt.emit (res);
  }

  /* from combo box {{{ */
  /*
  bool EditMessage::on_from_combo_key_press (GdkEventKey * event) {

    switch (event->keyval) {
      case GDK_KEY_j:
        {
          from_combo->set_active (from_combo->get_active_row_number()+1);
          return true;
        }
    }

    return false;
  }
  */

  // }}}

}

