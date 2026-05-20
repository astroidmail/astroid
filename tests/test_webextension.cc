# define BOOST_TEST_DYN_LINK
# define BOOST_TEST_MODULE TestWebExtension
# include <boost/test/unit_test.hpp>
# include <boost/filesystem.hpp>

# include <webkit2/webkit2.h>

# include "test_common.hh"

# include "utils/ustring_utils.hh"

namespace bfs = boost::filesystem;

class TestClient : public sigc::trackable
{
public:
    WebKitWebView *webview;
    WebKitSettings *websettings;
    WebKitWebContext *context;
    refptr<Gio::SocketListener> srv;
    refptr<Gio::UnixConnection> ext;
    gulong extension_connect_id;
    GMainLoop *main_loop;

    void run_loop ()
    {
        LOG (info) << "test: start main loop";
        g_main_loop_run (main_loop);
    }

    void quit_loop ()
    {
        LOG (info) << "test: quit main loop";
        g_main_loop_quit (main_loop);
    }

    void extension_connect (refptr<Gio::AsyncResult> &res)
    {
        LOG (info) << "test: got web extension connect";
        ext = refptr<Gio::UnixConnection>::cast_dynamic (srv->accept_finish (res));
    }

    void init_web_extensions (WebKitWebContext * context)
    {
        bfs::path test_dir = bfs::system_complete (bfs::path ("."));
        bfs::path build_dir = test_dir.parent_path ();
        BOOST_CHECK (exists (build_dir / bfs::path ("libtvextension.so")));

        webkit_web_context_set_web_extensions_directory (context, build_dir.c_str ());

        ustring socket_addr = ustring::compose ("%1/astroid.%2",
                                                test_dir.c_str (),
                                                Astroid::UstringUtils::random_alphanumeric (30));
        refptr<Gio::UnixSocketAddress> addr;
        if (Gio::UnixSocketAddress::abstract_names_supported ())
            addr = Gio::UnixSocketAddress::create (socket_addr,
                                                   Gio::UNIX_SOCKET_ADDRESS_ABSTRACT);
        else
            addr = Gio::UnixSocketAddress::create (socket_addr,
                                                   Gio::UNIX_SOCKET_ADDRESS_PATH);

        mode_t p = umask (0077);
        srv = Gio::SocketListener::create ();
        refptr<Gio::SocketAddress> eaddr;

        srv->add_address (addr, Gio::SocketType::SOCKET_TYPE_STREAM,
                          Gio::SocketProtocol::SOCKET_PROTOCOL_DEFAULT,
                          eaddr);

        srv->accept_async (sigc::mem_fun (this, &TestClient::extension_connect));
        umask (p);

        GVariant *gaddr = g_variant_new_string (addr->get_path ().c_str ());
        webkit_web_context_set_web_extensions_initialization_user_data (context, gaddr);
    }

    static void on_web_extensions (WebKitWebContext *context, GVariant *user_data)
    {
        LOG (info) << "test: called initialize-web-extensions";
        ((TestClient *) user_data)->init_web_extensions (context);
    }

    static void on_load_changed (WebKitWebView *webview,
                                 WebKitLoadEvent load_event,
                                 TestClient *test)
    {
        LOG (info) << "test: called load-changed";
        if (load_event != WEBKIT_LOAD_FINISHED)
            return;
        g_signal_handlers_disconnect_by_func (webview,
                                              (gpointer) TestClient::on_load_changed,
                                              test);
        test->quit_loop ();
    }

    static gboolean wait_failed (gpointer user_data)
    {
        bool wait_timed_out = false;
        ((TestClient *) user_data)->quit_loop ();
        BOOST_CHECK (wait_timed_out);
        return G_SOURCE_REMOVE;
    }

    void wait (double seconds)
    {
        g_timeout_add (seconds * 1000, TestClient::wait_failed, this);
        g_signal_connect (webview, "load-changed",
                          G_CALLBACK (TestClient::on_load_changed),
                          (gpointer) this);
        run_loop ();
    }

    TestClient ()
    {
        context = webkit_web_context_new_ephemeral ();
        extension_connect_id = g_signal_connect (context,
                                                 "initialize-web-extensions",
                                                 G_CALLBACK (TestClient::on_web_extensions),
                                                 (gpointer) this);
        main_loop = g_main_loop_new (nullptr, true);

        websettings = WEBKIT_SETTINGS (webkit_settings_new_with_settings (NULL));

        webview = WEBKIT_WEB_VIEW (g_object_new (WEBKIT_TYPE_WEB_VIEW,
                                                 "web-context", context,
                                                 "settings", websettings,
                                                 NULL));

        if (g_object_is_floating (webview))
            g_object_ref_sink (webview);

        webkit_web_view_load_html (webview,
                                   "<html><head></head><body>WebExtension</body></html>",
                                   Astroid::UstringUtils::random_alphanumeric (30).c_str ());
    }

    ~TestClient ()
    {
        g_signal_handler_disconnect (context, extension_connect_id);
        if (ext) ext->close ();
        if (srv) srv->close ();
        g_object_unref (context);
        g_object_unref (websettings);
        g_object_unref (webview);
    }
};

BOOST_AUTO_TEST_SUITE(WebExtension)

    BOOST_AUTO_TEST_CASE(page_client)
    {
        setup ();

        auto client = new TestClient ();
        client->wait (10);
        delete client;

        teardown ();
    }

BOOST_AUTO_TEST_SUITE_END()
