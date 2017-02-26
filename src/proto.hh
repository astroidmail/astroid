# pragma once

# include <glibmm.h>

/* forward declarations of classes and structs 'n stuff */
namespace Astroid {

  /* aliases for often used types  */
  typedef Glib::ustring ustring;
  typedef Glib::ustring::size_type ustring_sz;
  # define refptr Glib::RefPtr

  /* core and database */
  class Astroid;
  class Db;
  class NotmuchItem;
  class NotmuchMessage;
  class NotmuchThread;
  class Config;
  struct StandardPaths;
  struct RuntimePaths;
  class AccountManager;
  class Account;
  //class Contacts;
  class Poll;
  class PluginManager;

  /* message and thread */
  class Message;
  class MessageThread;
  class Chunk;

  /* composing */
  class ComposeMessage;

  /* actions */
  class ActionManager;
  class Action;
  class TagAction;
  class ToggleAction;
  class SpamAction;
  class MuteAction;

  /* user interface */
  class MainWindow;
  class CommandBar;
  class LogView;

  /* modes */
  class Mode;
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

