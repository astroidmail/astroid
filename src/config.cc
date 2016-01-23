# include <iostream>
# include <stdlib.h>
# include <functional>

# include <boost/filesystem.hpp>
# include <boost/filesystem/operations.hpp>
# include <boost/property_tree/ptree.hpp>
# include <boost/property_tree/json_parser.hpp>
# include <boost/property_tree/ini_parser.hpp>

# include "config.hh"
# include "log.hh"
# include "poll.hh"

using namespace std;
using namespace boost::filesystem;
using boost::property_tree::ptree;

namespace Astroid {
  Config::Config (bool _test, bool no_load) {
    if (_test) {
      log << info << "cf: loading test config." << endl;
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
    log << info << "cf: loading config: " << fname << endl;
    if (!no_load)
      load_config (); // re-sets config_dir to parent of fname
  }

  void Config::load_dirs () {

    if (test) {
      /* using $PWD/test/test_home */

      path cur_path (current_path() );

      std_paths.home = cur_path / path("test/test_home");

      log << LogLevel::test << "cf: using home directory: " << std_paths.home.c_str () << endl;

    } else {
      char * home_c = getenv ("HOME");
      log << debug << "HOME: " << home_c << endl;

      if (home_c == NULL) {
        log << error << "cf: HOME environment variable not set." << endl;
        exit (1);
      }

      std_paths.home = path(home_c);
    }

    /* default config */
    char * config_home = getenv ("XDG_CONFIG_HOME");
    if (config_home == NULL) {
      std_paths.config_dir = std_paths.home / path(".config/astroid");
    } else {
      std_paths.config_dir = path(config_home) / path("astroid");
    }

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
  }

  ptree Config::setup_default_config (bool initial) {
    ptree default_config;
    default_config.put ("astroid.config.version", CONFIG_VERSION);
    std::string nm_cfg = path(std_paths.home / path (".notmuch-config")).string();
    default_config.put ("astroid.notmuch_config" , nm_cfg);
    default_config.put ("astroid.notmuch.db", "~/.mail");
    default_config.put ("astroid.notmuch.excluded_tags", "muted,spam,deleted");
    default_config.put ("astroid.notmuch.sent_tags", "sent");

    default_config.put ("astroid.debug.dryrun_sending", false);

    if (initial) {
      /* initial default options - these are only set when a new
       * configuration file is created. */

      /* default example account:
       *
       * note: AccountManager will complain if there are no accounts defined.
       *
       */
      default_config.put ("accounts.charlie.name", "Charlie Root");
      default_config.put ("accounts.charlie.email", "root@localhost");
      default_config.put ("accounts.charlie.gpgkey", "");
      default_config.put ("accounts.charlie.sendmail", "msmtp -t");
      default_config.put ("accounts.charlie.default", true);
      default_config.put ("accounts.charlie.save_sent", false);
      default_config.put ("accounts.charlie.save_sent_to",
          "/home/root/Mail/sent/cur/");
      default_config.put ("accounts.charlie.additional_sent_tags", "");

      default_config.put ("accounts.charlie.save_drafts_to",
          "/home/root/Mail/drafts/");

      /* default searches, also only set if initial */
      default_config.put("startup.queries.inbox", "tag:inbox");
    }

    /* ui behaviour */
    default_config.put ("thread_index.page_jump_rows", 6);
    default_config.put ("thread_index.sort_order", "newest");

    default_config.put ("general.time.clock_format", "local"); // or 24h, 12h
    default_config.put ("general.time.same_year", "%b %-e");
    default_config.put ("general.time.diff_year", "%x");

    /* thread index cell theme */
    default_config.put ("thread_index.cell.font_description", "default");
    default_config.put ("thread_index.cell.line_spacing", 2);
    default_config.put ("thread_index.cell.date_length", 10);
    default_config.put ("thread_index.cell.message_count_length", 4);
    default_config.put ("thread_index.cell.authors_length", 20);
    default_config.put ("thread_index.cell.tags_length", 80);

    default_config.put ("thread_index.cell.subject_color", "#807d74");
    default_config.put ("thread_index.cell.subject_color_selected", "#000000");

    default_config.put ("thread_index.cell.tags_color", "#31587a");
    default_config.put ("thread_index.cell.hidden_tags", "attachment,flagged,unread");


    /* editor */
    default_config.put ("editor.gvim.cmd", "gvim");
    default_config.put ("editor.gvim.args", "-f -c 'set ft=mail' '+set fileencoding=utf-8' '+set ff=unix' '+set enc=utf-8'"); //  '+/^\\s*\\n/' '+nohl'
    default_config.put ("editor.charset", "utf-8");

    /* mail composition */
    default_config.put ("mail.reply.quote_line", "Excerpts from %1's message of %2:"); // %1 = author, %2 = pretty_verbose_date
    default_config.put ("mail.forward.quote_line", "Forwarding %1's message of %2:"); // %1 = author, %2 = pretty_verbose_date
    default_config.put ("mail.forward.disposition", "inline");

    /* contacts (not in use)
    default_config.put ("contacts.lbdb.cmd", "lbdb");
    default_config.put ("contacts.lbdb.enable", false);
    default_config.put ("contacts.recent.load", 100);
    default_config.put ("contacts.recent.query", "not tag:spam");
    */

    /* polling */
    default_config.put ("poll.interval", Poll::DEFAULT_POLL_INTERVAL); // seconds

    /* attachments
     *
     *   a chunk is saved and opened with the this command */
    default_config.put ("attachment.external_open_cmd", "xdg-open");

    /* thread view
     *
     *   if true; chunks (parts) that are not viewed initially are opened
     *            externally when this is set. the part is opened with
     *            'attachment.external_open_cmd'. */
    default_config.put ("thread_view.open_html_part_external", true);

    /*   if a link is clicked (html, ftp, etc..) it is executed with this
     *   command. */
    default_config.put ("thread_view.open_external_link", "xdg-open");

    default_config.put ("thread_view.indent_messages", true);

    /* mathjax */
    default_config.put ("thread_view.mathjax.enable", true);

    // MathJax.js will be added to this uri when the script is loaded
    default_config.put ("thread_view.mathjax.uri_prefix", "https://cdn.mathjax.org/mathjax/latest/");

    // a comma-separated list of tags which mathjax is enabled for, if empty,
    // allow for all messages.
    default_config.put ("thread_view.mathjax.for_tags", "");

    /* code prettify */
    default_config.put ("thread_view.code_prettify.enable", true);

    // run_prettify.js will be added to this uri when the script is loaded
    default_config.put ("thread_view.code_prettify.uri_prefix", "https://google-code-prettify.googlecode.com/svn/loader/");

    // a comma-separeted list of tags which code_prettify is enabled for, if empty,
    // allow for all messages.
    default_config.put ("thread_view.code_prettify.for_tags", "");

    // the tag enclosing code
    default_config.put ("thread_view.code_prettify.code_tag", "```");
    default_config.put ("thread_view.code_prettify.enable_for_patches", true);

    /* crypto */
    default_config.put ("crypto.gpg.path", "gpg");

    return default_config;
  }

  void Config::write_back_config () {
    log << warn << "cf: writing back config to: " << std_paths.config_file << endl;

    write_json (std_paths.config_file.c_str (), config);
  }

  void Config::populate_notmuch_config (const std::string& nm_cfg) {
    boost::property_tree::read_ini( nm_cfg, notmuch_config );
  } 

  void Config::load_config (bool initial) {
    if (test) {
      log << info << "cf: test config, loading defaults." << endl;
      config = setup_default_config (true);
      config.put ("poll.interval", 0);
      config.put ("astroid.notmuch.db", "test/mail/test_mail");
      populate_notmuch_config(config.get<std::string> ("astroid.notmuch_config"));
      return;
    }

    log << info << "cf: loading: " << std_paths.config_file << endl;

    std_paths.config_dir = absolute(std_paths.config_file.parent_path());
    if (!is_directory(std_paths.config_dir)) {
      log << warn << "cf: making config dir.." << endl;
      create_directories (std_paths.config_dir);
    }


    if (!is_regular_file (std_paths.config_file)) {
      if (!initial) {
        log << warn << "cf: no config, using defaults." << endl;
      }
      config = setup_default_config (true);
      write_back_config ();
    } else {

      /* loading config file */
      ptree new_config;
      config = setup_default_config (false);
      read_json (std_paths.config_file.c_str(), new_config);
      log << info << "cf: version: " << config.get<int>("astroid.config.version") << endl;

      merge_ptree (new_config);

      if (!check_config (new_config)) {
        write_back_config ();
      }
    }
    populate_notmuch_config (config.get<std::string> ("astroid.notmuch_config") );
  }

  bool Config::check_config (ptree new_config) {
    bool changed = false;

    if (new_config != config) {
      changed = true;
    }

    int version = config.get<int>("astroid.config.version");

    if (version < 1) {
      /* check accounts for save_draft */
      ptree apt = config.get_child ("accounts");

      for (auto &kv : apt) {
        try {

          ustring sto = kv.second.get<string> ("save_drafts_to");

        } catch (const boost::property_tree::ptree_bad_path &ex) {

          log << warn << "config: setting default save draft path for account: " << kv.first << ", please update!" << endl;
          ustring key = ustring::compose ("accounts.%1.save_drafts_to", kv.first);
          config.put (key.c_str (), "/home/root/Mail/drafts/");

        }
      }

      changed = true;
    }

    if (version < 2) {
      /* check accounts additional_sent_tags */
      ptree apt = config.get_child ("accounts");

      for (auto &kv : apt) {
        try {

          ustring sto = kv.second.get<string> ("additional_sent_tags");

        } catch (const boost::property_tree::ptree_bad_path &ex) {

          ustring key = ustring::compose ("accounts.%1.additional_sent_tags", kv.first);
          config.put (key.c_str (), "");
        }
      }

      changed = true;
    }

    if (version < CONFIG_VERSION) {
      config.put ("astroid.config.version", CONFIG_VERSION);
      changed = true;
    }

    if (changed) {
      log << warn << "cf: missing values in config have been updated with defaults (old version: " << version << ", new: " << CONFIG_VERSION << ")" << endl;
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

  void Config::merge(const ptree &parent,
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

