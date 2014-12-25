# pragma once

# include <glibmm.h>

/* forward declarations of classes and structs 'n stuff */
namespace Astroid {

  /* aliases for often used types  */
  typedef Glib::ustring ustring;
  typedef Glib::ustring::size_type ustring_sz;
  # define refptr Glib::RefPtr

  class Astroid;
  class Db;
  class NotmuchThread;
  class Config;
  class AccountManager;
  class Account;
  //class Contacts;
  class Log;
  class Poll;

  /* message and thread */
  class Message;
  class MessageThread;
  class Chunk;

  /* composing */
  class ComposeMessage;

  /* UI */
  class MainWindow;
  class CommandBar;
  class LogView;

  /* actions */
  class GlobalActions;
  class ActionManager;
  class Action;
  class TagAction;
  class ToggleAction;
  class SpamAction;
  class MuteAction;

  /* modes */
  class Mode;
  struct ModeHelpInfo;
  class PanedMode;
  class ThreadIndex;
  class ThreadIndexScrolled;
  class ThreadIndexListStore;
  class ThreadIndexListCellRenderer;
  class ThreadIndexListView;
  class ThreadView;
  class HelpMode;
  class EditMessage;
  class ReplyMessage;
  class ForwardMessage;
  class RawMessage;

}

