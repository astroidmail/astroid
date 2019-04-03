# include <iostream>
# include <stdlib.h>
# include <functional>

# include <boost/filesystem.hpp>
# include <boost/filesystem/operations.hpp>
# include <boost/property_tree/ptree.hpp>
# include <boost/property_tree/json_parser.hpp>
# include <boost/property_tree/ini_parser.hpp>

# include "config.hh"
# include "poll.hh"
# include "utils/utils.hh"
# include "utils/resource.hh"

using namespace std;
using namespace boost::filesystem;
using boost::property_tree::ptree;

namespace Astroid {
  Config::Config (bool _test, bool no_load) {
    if (_test) {
      LOG (info) << "cf: loading test config.";
    }

    test = _test;

    load_dirs ();

    std_paths.config_file = std_paths.config_dir / path("config");

    if (!no_load)
      load_config (); // re-sets config_dir to parent of fname
  }

  Config::Config (const char * fname, bool no_load) {
    test = false;

    load_dirs ();
    std_paths.config_file = path(fname);
    LOG (info) << "cf: loading config: " << fname;
    if (!no_load)
      load_config (); // re-sets config_dir to parent of fname
  }

  void Config::load_dirs () {

    if (test) {
      /* using $PWD/test/test_home */

      path cur_path (current_path() );

      std_paths.home = cur_path / path("tests/test_home");

      LOG (debug) << "cf: using home and config_dir directory: " << std_paths.home.c_str ();

    } else {
      char * home_c = getenv ("HOME");
      LOG (debug) << "HOME: " << home_c;

      if (home_c == NULL) {
        LOG (error) << "cf: HOME environment variable not set.";
        exit (1);
      }

      std_paths.home = path(home_c);
    }

    /* default config */
    if (test) {
      std_paths.config_dir = std_paths.home;
    } else {
      char * config_home = getenv ("XDG_CONFIG_HOME");
      if (config_home == NULL) {
        std_paths.config_dir = std_paths.home / path(".config/astroid");
      } else {
        std_paths.config_dir = path(config_home) / path("astroid");
      }
    }

    /* searches file */
    std_paths.searches_file = std_paths.config_dir / path("searches");

    /* plugin dir */
    std_paths.plugin_dir = std_paths.config_dir / path ("plugins");

    /* default data */
    char * data = getenv ("XDG_DATA_HOME");
    if (data == NULL) {
      std_paths.data_dir = std_paths.home / path(".local/share/astroid");
    } else {
      std_paths.data_dir = path(data) / path("astroid");
    }

    /* default cache */
    char * cache = getenv ("XDG_CACHE_HOME");
    if (cache == NULL) {
      std_paths.cache_dir = std_paths.home / path(".cache/astroid");
    } else {
      std_paths.cache_dir = path(cache) / path("astroid");
    }

    /* default runtime */
    char * runtime = getenv ("XDG_RUNTIME_HOME");
    if (runtime == NULL) {
      std_paths.runtime_dir = std_paths.cache_dir;
    } else {
      std_paths.runtime_dir = path(runtime) / path("astroid");
    }

    /* save to and attach from directory */
    std_paths.save_dir   = std_paths.home;
    std_paths.attach_dir = std_paths.home;
  }

  void  Config::setup_default_initial_config (ptree &conf, bool accounts, bool startup) {
    /* initial default options - these are only set when the sections are
     * completely missing.
     */

    if (accounts) {
      /* default example account:
       *
       * note: AccountManager will complain if there are no accounts defined.
       *
       */
      conf.put ("accounts.charlie.name", "Charlie Root");
      conf.put ("accounts.charlie.email", "root@localhost");
      conf.put ("accounts.charlie.gpgkey", "");
      conf.put ("accounts.charlie.always_gpg_sign", false);
      conf.put ("accounts.charlie.sendmail", "msmtp -i -t");
      conf.put ("accounts.charlie.default", true);
      conf.put ("accounts.charlie.save_sent", false);
      conf.put ("accounts.charlie.save_sent_to",
          "/home/root/Mail/sent/cur/");
      conf.put ("accounts.charlie.additional_sent_tags", "");

      conf.put ("accounts.charlie.save_drafts_to",
          "/home/root/Mail/drafts/");

      conf.put ("accounts.charlie.signature_separate", false);
      conf.put ("accounts.charlie.signature_file", "");
      conf.put ("accounts.charlie.signature_file_markdown", "");
      conf.put ("accounts.charlie.signature_default_on", true);
      conf.put ("accounts.charlie.signature_attach", false);

      conf.put ("accounts.charlie.select_query", "");
    }

    if (startup) {
      /* default searches, also only set if initial */
      conf.put("startup.queries.inbox", "tag:inbox");
    }
  }

  ptree Config::setup_default_config (bool initial) {
    ptree default_config;
    default_config.put ("astroid.config.version", CONFIG_VERSION);

    std::string nm_cfg = path(std_paths.home / path (".notmuch-config")).string();
    char* nm_env = getenv("NOTMUCH_CONFIG");
    if (nm_env != NULL) {
      nm_cfg.assign(nm_env, strlen(nm_env));
    }
    default_config.put ("astroid.notmuch_config" , nm_cfg);

    default_config.put ("astroid.debug.dryrun_sending", false);

    /* only show hints with a level higher than this */
    default_config.put ("astroid.hints.level", 0);

    default_config.put ("astroid.log.syslog", false);
    default_config.put ("astroid.log.stdout", true);

# if DEBUG
    default_config.put ("astroid.log.level", "debug"); // (trace, debug, info, warning, error, fatal)
# else
    default_config.put ("astroid.log.level", "info"); // (trace, debug, info, warning, error, fatal)
# endif

    if (initial) {
      setup_default_initial_config (default_config);
    }

    /* terminal */
    default_config.put ("terminal.height", 10);
    default_config.put ("terminal.font_description", "default"); // https://developer.gnome.org/pango/stable/pango-Fonts.html#pango-font-description-from-string

    /* thread index */
    default_config.put ("thread_index.page_jump_rows", 6);
    default_config.put ("thread_index.sort_order", "newest");

    default_config.put ("general.time.clock_format", "local"); // or 24h, 12h
    default_config.put ("general.time.same_year", "%b %-e");
    default_config.put ("general.time.diff_year", "%x");

    default_config.put ("general.tagbar_move", "tag");

    /* thread index cell theme */
    default_config.put ("thread_index.cell.font_description", "default");
    default_config.put ("thread_index.cell.line_spacing", 2);
    default_config.put ("thread_index.cell.date_length", 10);
    default_config.put ("thread_index.cell.message_count_length", 4);
    default_config.put ("thread_index.cell.authors_length", 20);

    default_config.put ("thread_index.cell.subject_color", "#807d74");
    default_config.put ("thread_index.cell.subject_color_selected", "#000000");
    default_config.put ("thread_index.cell.background_color_selected", "");
    default_config.put ("thread_index.cell.background_color_marked", "#fff584");
    default_config.put ("thread_index.cell.background_color_marked_selected", "#bcb559");

    default_config.put ("thread_index.cell.tags_length", 80);
    default_config.put ("thread_index.cell.tags_upper_color", "#e5e5e5");
    default_config.put ("thread_index.cell.tags_lower_color", "#333333");
    default_config.put ("thread_index.cell.tags_alpha", "0.5");
    default_config.put ("thread_index.cell.hidden_tags", "attachment,flagged,unread");


    /* editor */
    // also useful: '+/^\\s*\\n/' '+nohl'
# ifndef DISABLE_EMBEDDED
    default_config.put ("editor.cmd", "gvim -geom 10x10 --servername %2 --socketid %3 -f -c 'set ft=mail' '+set fileencoding=utf-8' '+set ff=unix' '+set enc=utf-8' '+set fo+=w' %1");
    default_config.put ("editor.external_editor", false); // should be true on Wayland
# else
    default_config.put ("editor.cmd", "gvim -f -c 'set ft=mail' '+set fileencoding=utf-8' '+set enc=utf-8' '+set ff=unix' '+set fo+=w' %1");
    default_config.put ("editor.external_editor", true); // should be true on Wayland
# endif

    default_config.put ("editor.charset", "utf-8");
    default_config.put ("editor.save_draft_on_force_quit", true);

    default_config.put ("editor.attachment_words", "attach");
    default_config.put ("editor.attachment_directory", "~");

    default_config.put ("editor.markdown_processor", "cmark");
    default_config.put ("editor.markdown_on", false); // default

    default_config.put ("mail.reply.quote_processor", "w3m -dump -T text/html"); // e.g. lynx -dump

    /* mail composition */
    default_config.put ("mail.reply.quote_line", "Excerpts from %1's message of %2:"); // %1 = author, %2 = pretty_verbose_date
    default_config.put ("mail.reply.mailinglist_reply_to_sender", true);
    default_config.put ("mail.forward.quote_line", "Forwarding %1's message of %2:"); // %1 = author, %2 = pretty_verbose_date
    default_config.put ("mail.forward.disposition", "inline");
    default_config.put ("mail.sent_tags", "sent");
    default_config.put ("mail.message_id_fqdn", ""); // custom fqdn for the message id: default: local hostname
    default_config.put ("mail.message_id_user", ""); // custom user for the message id: default: 'astroid'
    default_config.put ("mail.user_agent", "default");
    default_config.put ("mail.send_delay", 2); // wait seconds before sending, allowing to cancel
    default_config.put ("mail.close_on_success", false); // close page automatically on succesful sending of message
    default_config.put ("mail.format_flowed", false); // mail sent with astroid can be reformatted using format_flowed

    /* polling */
    default_config.put ("poll.interval", Poll::DEFAULT_POLL_INTERVAL); // seconds
    default_config.put ("poll.always_full_refresh", false); // always do full refresh after poll, slow.

    /* attachments
     *
     *   a chunk is saved and opened with this command */
    default_config.put ("attachment.external_open_cmd", "xdg-open");

    /* thread view
     *
     *   if true; chunks (parts) that are not viewed initially are opened
     *            externally when this is set. the part is opened with
     *            'attachment.external_open_cmd'. */
    default_config.put ("thread_view.open_html_part_external", false);
    default_config.put ("thread_view.preferred_type", "plain");
    default_config.put ("thread_view.preferred_html_only", false);

    default_config.put ("thread_view.allow_remote_when_encrypted", false);

    /*   if a link is clicked (html, ftp, etc..) it is executed with this
     *   command. */
    default_config.put ("thread_view.open_external_link", "xdg-open");

    default_config.put ("thread_view.default_save_directory", "~");

    default_config.put ("thread_view.indent_messages", false);

    /* gravatar */
    default_config.put ("thread_view.gravatar.enable", true);

    /* mark unread */
    default_config.put ("thread_view.mark_unread_delay", .5);

    /* expand flagged messages by default */
    default_config.put ("thread_view.expand_flagged", true);

    /* crypto */
    default_config.put ("crypto.gpg.path", "gpg2");
    default_config.put ("crypto.gpg.always_trust", true);
    default_config.put ("crypto.gpg.enabled", true);

    /* saved searches */
    default_config.put ("saved_searches.show_on_startup", false);
    default_config.put ("saved_searches.save_history", true);
    default_config.put ("saved_searches.history_lines_to_show", 15); /* -1 is all */
    default_config.put ("saved_searches.history_lines", 1000); /* number of history lines to store */

    return default_config;
  }

  void Config::write_back_config () {
    LOG (warn) << "cf: writing back config to: " << std_paths.config_file;

    write_json (std_paths.config_file.c_str (), config);
  }

  void Config::load_config (bool initial) {
    if (test) {
      LOG (info) << "cf: test config, loading defaults.";
      config = setup_default_config (true);
      config.put ("poll.interval", 0);
      config.put ("accounts.charlie.gpgkey", "gaute@astroidmail.bar");
      config.put ("mail.send_delay", 0);
      std::string test_nmcfg_path;
      if (getenv ("ASTROID_BUILD_DIR")) {
        test_nmcfg_path = (current_path () / path ("tests/mail/test_config")).string();
      } else {
        test_nmcfg_path = (Resource::get_exe_dir () / path ("tests/mail/test_config")).string();
      }
      boost::property_tree::read_ini (test_nmcfg_path, notmuch_config);
      has_notmuch_config = true;
      return;
    }

    LOG (info) << "cf: loading: " << std_paths.config_file;

    std_paths.config_dir = absolute(std_paths.config_file.parent_path());
    if (!is_directory(std_paths.config_dir)) {
      LOG (warn) << "cf: making config dir..";
      create_directories (std_paths.config_dir);
    }

    if (!is_directory(std_paths.runtime_dir)) {
      LOG (warn) << "cf: making runtime dir..";
      create_directories (std_paths.runtime_dir);
    }

    if (!is_regular_file (std_paths.config_file)) {
      if (!initial) {
        LOG (warn) << "cf: no config, using defaults.";
      }
      config = setup_default_config (true);
    } else {

      /* loading config file */
      ptree new_config;
      config = setup_default_config (false);
      read_json (std_paths.config_file.c_str(), new_config);
      LOG (info) << "cf: version: " << config.get<int>("astroid.config.version");

      merge_ptree (new_config);

      check_config (new_config);
    }

    /* load save_dir */
    std_paths.save_dir = Utils::expand(bfs::path (config.get<string>("thread_view.default_save_directory")));
    run_paths.save_dir = std_paths.save_dir;

    /* load attach_dir */
    std_paths.attach_dir = Utils::expand(bfs::path (config.get<string>("editor.attachment_directory")));
    run_paths.attach_dir = std_paths.attach_dir;

    /* read notmuch config */
    bfs::path notmuch_config_path;
    char * notmuch_config_env = getenv ("NOTMUCH_CONFIG");
    if (notmuch_config_env) {
      notmuch_config_path = Utils::expand(bfs::path (notmuch_config_env));
    } else {
      notmuch_config_path = Utils::expand(bfs::path (config.get<string>("astroid.notmuch_config")));
    }

    if (is_regular_file (notmuch_config_path)) {
      boost::property_tree::read_ini (
        notmuch_config_path.c_str(),
        notmuch_config);
      has_notmuch_config = true;
    } else {
      has_notmuch_config = false;
    }
  }


  bool Config::check_config (ptree new_config) {
    LOG (debug) << "cf: check config..";
    bool changed = false;

    if (new_config != config) {
      changed = true;
    }

    int version = config.get<int>("astroid.config.version");

    {
      bool hasstartup;
      try {
        ptree strup = config.get_child ("startup");
        hasstartup  = strup.count ("queries") > 0;

        if ( (hasstartup &&
              strup.get_child ("queries").size () == 0) &&
             !config.get<bool> ("saved_searches.show_on_startup")
            )
        {
          LOG (info) << "cf: no startup queries, forcing show saved_searches on startup.";
          config.put ("saved_searches.show_on_startup", true);
        }
      } catch (const boost::property_tree::ptree_bad_path &ex) {
        hasstartup  = false;
      }

      bool hasaccounts;
      try {
        ptree apt   = config.get_child ("accounts");
        hasaccounts = apt.size () > 0;
      } catch (const boost::property_tree::ptree_bad_path &ex) {
        hasaccounts = false;
      }

      if (!hasaccounts || !hasstartup) {
        LOG (warn) << "cf: missing accounts or startup.queries: using defaults";
        setup_default_initial_config (config, !hasaccounts, !hasstartup);
      }
    }

    if (version < CONFIG_VERSION) {
      LOG (error) << "cf: the config file is an old version (" << version << "), the current version is: " << CONFIG_VERSION;
    }

    if (changed) {
      LOG (warn) << "cf: missing values in config have been updated with defaults (old version: " << version << ", new: " << CONFIG_VERSION << ")";
    }

    return !changed;
  }

  /* TODO: split into utils/ somewhere.. */
  /* merge of property trees */

  // from http://stackoverflow.com/questions/8154107/how-do-i-merge-update-a-boostproperty-treeptree
  template<typename T>
    void Config::traverse_recursive(
        const boost::property_tree::ptree &parent,
        const boost::property_tree::ptree::path_type &childPath,
        const boost::property_tree::ptree &child, T &method)
    {
      using boost::property_tree::ptree;

      method(parent, childPath, child);
      for(ptree::const_iterator it=child.begin();
          it!=child.end();
          ++it) {
        ptree::path_type curPath = childPath / ptree::path_type(it->first);
        traverse_recursive(parent, curPath, it->second, method);
      }
    }

  void Config::traverse(const ptree &parent,
            function<void(const ptree &,
            const ptree::path_type &,
            const ptree&)> method)
    {
      traverse_recursive(parent, "", parent, method);
    }

  void Config::merge(const ptree & /* parent */,
             const ptree::path_type &childPath,
             const ptree &child) {

    // overwrites existing default values
    config.put(childPath, child.data());
  }

  void Config::merge_ptree(const ptree &pt)   {
    function<void(const ptree &,
      const ptree::path_type &,
      const ptree&)> method = bind (&Config::merge, this, _1, _2, _3);

    traverse(pt, method);
  }
}

