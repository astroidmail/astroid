# pragma once

/* forward declarations of classes and structs 'n stuff */
namespace Astroid {

  class Astroid;
  class Db;
  class NotmuchThread;
  class Config;
  class AccountManager;
  class Account;
  class Contacts;

  /* message and thread */
  class Message;
  class MessageThread;
  class Chunk;

  /* composing */
  class ComposeMessage;

  /* UI */
  class MainWindow;
  class CommandBar;

  /* actions */
  class ActionManager;
  class Action;
  class TagAction;
  class ArchiveAction;
  class StarAction;

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

}

