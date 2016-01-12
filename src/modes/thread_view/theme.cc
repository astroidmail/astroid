# include "theme.hh"

# include <atomic>
# include <iostream>
# include <fstream>
# include <boost/filesystem.hpp>

# include "astroid.hh"
# include "config.hh"
# include "log.hh"

# include <sass/context.h>

using namespace std;
using namespace boost::filesystem;

namespace Astroid {
  atomic<bool> Theme::theme_loaded (false);
  const char * Theme::thread_view_html_f = "ui/thread-view.html";
  const char * Theme::thread_view_css_f  = "ui/thread-view.css";
  ustring Theme::thread_view_html;
  ustring Theme::thread_view_css;

  Theme::Theme () {
    log << debug << "theme: loading.." << endl;
    /* https://github.com/sass/libsass/blob/master/docs/api-doc.md */

    /*

    context = sass_make_file_context("file.scss")
    options = sass_file_context_get_options(context)
    sass_option_set_precision(options, 1)
    sass_option_set_source_comments(options, true)

    sass_file_context_set_options(context, options)

    compiler = sass_make_file_compiler(sass_context)
    sass_compiler_parse(compiler)
    sass_compiler_execute(compiler)

    output = sass_context_get_output_string(context)
    // Retrieve errors during compilation
    error_status = sass_context_get_error_status(context)
    json_error = sass_context_get_error_json(context)
    // Release memory dedicated to the C compiler
    sass_delete_compiler(compiler)

  */
    /* load css, html and DOM objects */
    if (!theme_loaded) {
      path def_tv_html = path(thread_view_html_f);
      path def_tv_css  = path(thread_view_css_f);
# ifdef PREFIX
      path tv_html = path(PREFIX) / path("share/astroid") / def_tv_html;
      path tv_css  = path(PREFIX) / path("share/astroid") / def_tv_css;
# else
      path tv_html = def_tv_html;
      path tv_css  = def_tv_css;
# endif

      /* check for user modified theme files in config directory */
      path user_tv_html = astroid->config->config_dir / def_tv_html;
      path user_tv_css  = astroid->config->config_dir / def_tv_css;

      if (exists (user_tv_html)) {
          tv_html = user_tv_html;
          log << info << "tv: using user html file: " << tv_html.c_str () << endl;


      } else if (!exists(tv_html)) {
        log << error << "tv: cannot find html theme file: " << tv_html.c_str() << ", using default.." << endl;
        if (!exists(def_tv_html)) {
          log << error << "tv: cannot find default html theme file." << endl;
          exit (1);
        }

        tv_html = def_tv_html;
      }

      if (!check_theme_version (tv_html)) {

        log << error << "tv: html file version does not match!" << endl;

      }

      if (exists (user_tv_css)) {
          tv_css = user_tv_css;
          log << info << "tv: using user css file: " << tv_css.c_str () << endl;


      } else if (!exists(tv_css)) {
        log << error << "tv: cannot find css theme file: " << tv_css.c_str() << ", using default.." << endl;
        if (!exists(def_tv_css)) {
          log << error << "tv: cannot find default css theme file." << endl;
          exit (1);
        }

        tv_css = def_tv_css;
      }

      if (!check_theme_version (tv_css)) {

        log << error << "tv: css file version does not match!" << endl;

      }

      std::ifstream tv_html_f (tv_html.c_str());
      istreambuf_iterator<char> eos; // default is eos
      istreambuf_iterator<char> tv_iit (tv_html_f);

      thread_view_html.append (tv_iit, eos);
      tv_html_f.close ();

      std::ifstream tv_css_f (tv_css.c_str());
      istreambuf_iterator<char> tv_css_iit (tv_css_f);
      thread_view_css.append (tv_css_iit, eos);
      tv_css_f.close ();

      theme_loaded = true;
    }
  }

  bool Theme::check_theme_version (path p) {
    /* check version found in first line in file */

    std::ifstream f (p.c_str ());

    ustring vline;
    int version;
    f >> vline >> vline >> version;

    log << debug << "tv: testing version: " << version << endl;

    f.close ();

    return (version == THEME_VERSION);
  }
}

