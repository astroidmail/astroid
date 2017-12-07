# include "theme.hh"

# include <atomic>
# include <iostream>
# include <fstream>
# include <boost/filesystem.hpp>

# include "astroid.hh"
# include "config.hh"
# include "utils/resource.hh"

# ifndef DISABLE_LIBSASS

# ifdef SASSCTX_CONTEXT_H
  # include <sass/context.h>
# elif SASSCTX_SASS_CONTEXT_H
  # include <sass_context.h>
# endif

# endif


using namespace std;
using namespace boost::filesystem;

namespace Astroid {
  std::atomic<bool> Theme::theme_loaded (false);
  const char * Theme::thread_view_html_f = "ui/thread-view.html";
# ifndef DISABLE_LIBSASS
  const char * Theme::thread_view_scss_f  = "ui/thread-view.scss";
# else
  const char * Theme::thread_view_css_f  = "ui/thread-view.css";
# endif
  ustring Theme::thread_view_html;
  ustring Theme::thread_view_css;

  Theme::Theme () {
    using bfs::path;
    using std::endl;
    LOG (debug) << "theme: loading..";

    /* load html and css (from scss) */
    if (!theme_loaded) {
      path tv_html = Resource (true, thread_view_html_f).get_path ();

      if (!check_theme_version (tv_html)) {

        LOG (error) << "tv: html file version does not match!";

      }

# ifndef DISABLE_LIBSASS
      path tv_scss = Resource (true, thread_view_scss_f).get_path ();
      if (!check_theme_version (tv_scss)) {

        LOG (error) << "tv: scss file version does not match!";

      }
# else
      path tv_css = Resource (true, thread_view_css_f).get_path ();

      if (!check_theme_version (tv_css)) {

        LOG (error) << "tv: css file version does not match!";

      }
# endif

      {
        std::ifstream tv_html_f (tv_html.c_str());
        std::istreambuf_iterator<char> eos; // default is eos
        std::istreambuf_iterator<char> tv_iit (tv_html_f);

        thread_view_html.append (tv_iit, eos);
        tv_html_f.close ();
      }

# ifndef DISABLE_LIBSASS
      thread_view_css = process_scss (tv_scss.c_str ());
# else
      {
        std::ifstream tv_css_f (tv_css.c_str());
        std::istreambuf_iterator<char> eos; // default is eos
        std::istreambuf_iterator<char> tv_iit (tv_css_f);

        thread_view_css.append (tv_iit, eos);
        tv_css_f.close ();
      }
# endif

      theme_loaded = true;
    }
  }

# ifndef DISABLE_LIBSASS
  ustring Theme::process_scss (const char * scsspath) {
    /* - https://github.com/sass/libsass/blob/master/docs/api-doc.md
     * - https://github.com/sass/libsass/blob/master/docs/api-context-example.md
     */
    using std::endl;

    LOG (info) << "theme: processing: " << scsspath;

    struct Sass_File_Context* file_ctx = sass_make_file_context(scsspath);
    struct Sass_Options* options = sass_file_context_get_options(file_ctx);
    struct Sass_Context* context = sass_file_context_get_context(file_ctx);
    sass_option_set_precision(options, 1);
    sass_option_set_source_comments(options, true);

    int status = sass_compile_file_context (file_ctx);

    if (status != 0) {
      const char * err = sass_context_get_error_message (context);
      ustring erru (err);
      LOG (error) << "theme: error processing: " << erru;

      throw runtime_error (
          ustring::compose ("theme: could not process scss: %1", erru).c_str ());
    }

    const char * output = sass_context_get_output_string(context);
    ustring output_str(output);
    sass_delete_file_context (file_ctx);

    return output_str;
  }
# endif

  bool Theme::check_theme_version (bfs::path p) {
    /* check version found in first line in file */

    std::ifstream f (p.c_str ());

    ustring vline;
    int version;
    f >> vline >> vline >> version;

    LOG (debug) << "tv: testing version: " << version;

    f.close ();

    return (version == THEME_VERSION);
  }
}

