# include <iostream>
# include <stdlib.h>
# include <functional>

# include <boost/filesystem.hpp>
# include <boost/property_tree/ptree.hpp>
# include <boost/property_tree/json_parser.hpp>

# include "config.hh"
# include "log.hh"

using namespace std;
using namespace boost::filesystem;
using boost::property_tree::ptree;

namespace Astroid {
  Config::Config (bool _test, bool no_load) {
    load_dirs ();

    test = _test;

    config_file = config_dir / path("config");

    if (!no_load)
      load_config (); // re-sets config_dir to parent of fname
  }

  Config::Config (const char * fname, bool no_load) {
    load_dirs ();
    config_file = path(fname);
    if (!no_load)
      load_config (); // re-sets config_dir to parent of fname
  }

  void Config::load_dirs () {
    char * home_c      = getenv ("HOME");

    if (home_c == NULL) {
      log << error << "cf: HOME environment variable not set." << endl;
      exit (1);
    }

    home = path(home_c);

    /* default config */
    char * config_home = getenv ("XDG_CONFIG_HOME");
    if (config_home == NULL) {
      config_dir = home / path(".config/astroid");
    } else {
      config_dir = path(config_home) / path("astroid");
    }

    /* default data */
    char * data = getenv ("XDG_DATA_HOME");
    if (data == NULL) {
      data_dir = home / path(".local/share/astroid");
    } else {
      data_dir = path(data) / path("astroid");
    }

    /* default cache */
    char * cache = getenv ("XDG_CACHE_HOME");
    if (cache == NULL) {
      cache_dir = home / path(".cache/astroid");
    } else {
      cache_dir = path(cache) / path("astroid");
    }

    /* default runtime */
    char * runtime = getenv ("XDG_RUNTIME_HOME");
    if (runtime == NULL) {
      runtime_dir = cache_dir;
    } else {
      runtime_dir = path(runtime) / path("astroid");
    }
  }

  void Config::setup_default_config (bool initial) {
    default_config.put ("astroid.config.version", CONFIG_VERSION);
    default_config.put ("astroid.notmuch.db", "~/.mail");
    default_config.put ("astroid.notmuch.excluded_tags", "muted,spam,deleted");

    /* TODO: eventually these should be removed */
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


      /* default searches, also only set if initial */
      default_config.put("startup.queries.inbox", "tag:inbox");
    }

    /* ui behaviour */
    default_config.put ("thread_index.open_default_paned", false);
    default_config.put ("thread_index.page_jump_rows", 6);
    default_config.put ("general.time.clock_format", "local"); // or 24h, 12h
    default_config.put ("general.time.same_year", "%b %-e");
    default_config.put ("general.time.diff_year", "%x");

    /* editor */
    default_config.put ("editor.gvim.cmd", "gvim");
    default_config.put ("editor.gvim.args", "-f -c 'set ft=mail' '+set fileencoding=utf-8' '+set ff=unix' '+set enc=utf-8'"); //  '+/^\\s*\\n/' '+nohl'
    default_config.put ("editor.charset", "utf-8");

    /* mail composition */
    default_config.put ("mail.reply.quote_line", "Excerpts from %1's message of %2:"); // %1 = author, %2 = pretty_verbose_date
    default_config.put ("mail.forward.quote_line", "Forwarding %1's message of %2:"); // %1 = author, %2 = pretty_verbose_date

    /* contacts (not in use)
    default_config.put ("contacts.lbdb.cmd", "lbdb");
    default_config.put ("contacts.lbdb.enable", false);
    default_config.put ("contacts.recent.load", 100);
    default_config.put ("contacts.recent.query", "not tag:spam");
    */

    /* polling */
    default_config.put ("poll.interval", 60); // seconds

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

    // a ; delimited list of tags which mathjax is enabled for, if empty,
    // allow for all messages.
    default_config.put ("thread_view.mathjax.for_tags", "");

    /* code prettify */
    default_config.put ("thread_view.code_prettify.enable", true);

    // run_prettify.js will be added to this uri when the script is loaded
    default_config.put ("thread_view.code_prettify.uri_prefix", "https://google-code-prettify.googlecode.com/svn/loader/");

    // a ; delimited list of tags which code_prettify is enabled for, if empty,
    // allow for all messages.
    default_config.put ("thread_view.code_prettify.for_tags", "");

    // the tag enclosing code
    default_config.put ("thread_view.code_prettify.code_tag", "```");
    default_config.put ("thread_view.code_prettify.enable_for_patches", true);
  }

  void Config::write_back_config () {
    log << warn << "cf: writing back config to: " << config_file << endl;

    write_json (config_file.c_str (), config);
  }

  void Config::load_config (bool initial) {
    if (test) {
      log << info << "cf: test config, loading defaults." << endl;
      setup_default_config (true);
      config = default_config;
      config.put ("poll.interval", 0);
      config.put ("astroid.notmuch.db", "test/mail/test_mail");
      return;
    }

    log << info << "cf: loading: " << config_file << endl;

    config_dir = absolute(config_file.parent_path());
    if (!is_directory(config_dir)) {
      log << warn << "cf: making config dir.." << endl;
      create_directories (config_dir);
    }


    if (!is_regular_file (config_file)) {
      if (!initial) {
        log << warn << "cf: no config, using defaults." << endl;
      }
      setup_default_config (true);
      config = default_config;
      write_back_config ();
    } else {
      ptree new_config;
      setup_default_config (false);
      config = default_config;
      read_json (config_file.c_str(), new_config);

      merge_ptree (new_config);

      if (new_config != config) {
        log << warn << "cf: missing values in config have been updated with defaults." << endl;
        write_back_config ();
      }
    }
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

