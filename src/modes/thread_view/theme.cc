# include "theme.hh"

# include <atomic>
# include <iostream>
# include <fstream>
# include <boost/filesystem.hpp>

# include "astroid.hh"
# include "config.hh"
# include "log.hh"
# include "utils/resource.hh"

# ifdef SASSCTX_CONTEXT_H
  # include <sass/context.h>
# elif SASSCTX_SASS_CONTEXT_H
  # include <sass_context.h>
# endif


using namespace std;
using namespace boost::filesystem;

namespace Astroid {
  std::atomic<bool> Theme::theme_loaded (false);
  const char * Theme::thread_view_html_f = "ui/thread-view.html";
  const char * Theme::thread_view_scss_f  = "ui/thread-view.scss";
  ustring Theme::thread_view_html;
  ustring Theme::thread_view_css;

  Theme::Theme () {
    using bfs::path;
    using std::endl;
    log << debug << "theme: loading.." << endl;

    /* load css, html and DOM objects */
    if (!theme_loaded) {
      path tv_html = Resource (true, thread_view_html_f).get_path ();
      path tv_scss = Resource (true, thread_view_scss_f).get_path ();

      if (!check_theme_version (tv_html)) {

        log << error << "tv: html file version does not match!" << endl;

      }

      if (!check_theme_version (tv_scss)) {

        log << error << "tv: scss file version does not match!" << endl;

      }

      std::ifstream tv_html_f (tv_html.c_str());
      std::istreambuf_iterator<char> eos; // default is eos
      std::istreambuf_iterator<char> tv_iit (tv_html_f);

      thread_view_html.append (tv_iit, eos);
      tv_html_f.close ();

      thread_view_css = process_scss (tv_scss.c_str ());

      /*
      std::ifstream tv_css_f (tv_css.c_str());
      istreambuf_iterator<char> tv_css_iit (tv_css_f);
      thread_view_css.append (tv_css_iit, eos);
      tv_css_f.close ();
      */

      theme_loaded = true;
    }
  }

  ustring Theme::process_scss (const char * scsspath) {
    /* - https://github.com/sass/libsass/blob/master/docs/api-doc.md
     * - https://github.com/sass/libsass/blob/master/docs/api-context-example.md
     */
    using std::endl;

    log << info << "theme: processing: " << scsspath << endl;

    struct Sass_File_Context* file_ctx = sass_make_file_context(scsspath);
    struct Sass_Options* options = sass_file_context_get_options(file_ctx);
    struct Sass_Context* context = sass_file_context_get_context(file_ctx);
    sass_option_set_precision(options, 1);
    sass_option_set_source_comments(options, true);

    sass_file_context_set_options(file_ctx, options);

    int status = sass_compile_file_context (file_ctx);

    if (status != 0) {
      const char * err = sass_context_get_error_message (context);
      ustring erru (err);
      log << error << "theme: error processing: " << erru << endl;

      throw runtime_error (
          ustring::compose ("theme: could not process scss: %1", erru).c_str ());
    }

    const char * output = sass_context_get_output_string(context);
    ustring output_str(output);
    sass_delete_file_context (file_ctx);

    return output_str;
  }

  bool Theme::check_theme_version (bfs::path p) {
    /* check version found in first line in file */

    std::ifstream f (p.c_str ());

    ustring vline;
    int version;
    f >> vline >> vline >> version;

    log << debug << "tv: testing version: " << version << std::endl;

    f.close ();

    return (version == THEME_VERSION);
  }
}

