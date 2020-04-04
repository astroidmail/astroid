# define BOOST_TEST_DYN_LINK
# define BOOST_TEST_MODULE TestCompose
# include <boost/test/unit_test.hpp>

# include <string>
# include "test_common.hh"
# include "compose_message.hh"
# include "message_thread.hh"
# include "chunk.hh"
# include "account_manager.hh"
# include "utils/address.hh"
# include "utils/ustring_utils.hh"

BOOST_AUTO_TEST_SUITE(Markdown)

  BOOST_AUTO_TEST_CASE(compose_read_test)
  {
    using Astroid::ComposeMessage;
    using Astroid::Message;
    setup ();

    ComposeMessage * c = new ComposeMessage ();

    ustring bdy = "# This is a test";

    c->body << bdy;
    c->markdown = true;

    c->build ();
    c->finalize ();

    BOOST_CHECK_MESSAGE (c->markdown_success, c->markdown_error);

    ustring fn = c->write_tmp ();

    Message m (fn);

    /* check plain text part */
    ustring pbdy = m.plain_text (false);
    BOOST_CHECK_MESSAGE (pbdy == bdy, "plain text matches plain text");

    /* check html part */
    BOOST_CHECK_MESSAGE (g_mime_content_type_is_type (m.root->content_type, "multipart", "alternative"), "main message part is multipart/alternative");

    auto plain = m.root->kids[0];
    auto html  = m.root->kids[1];

    BOOST_CHECK_MESSAGE (g_mime_content_type_is_type(plain->content_type, "text", "plain"), "first part is text/plain");
    BOOST_CHECK_MESSAGE (g_mime_content_type_is_type(html->content_type, "text", "html"), "second part is text/html");

    ustring _html = html->viewable_text (false);
    Astroid::UstringUtils::trim(_html);

    LOG (trace) << "markdown html: '" << _html << "'";

    BOOST_CHECK_MESSAGE ("<h1>This is a test</h1>" == _html, "html part is correctly constructed");

    unlink (fn.c_str ());
    teardown ();
  }
  BOOST_AUTO_TEST_CASE(regression_markdown_partial_write)
  {
    using Astroid::ComposeMessage;
    using Astroid::Message;
    setup ();

    ComposeMessage * c = new ComposeMessage ();

    ustring bdy = R"(# This is a test
      Ὁ δὲ Σαῦλος ἔτι ἐμπνέων ἀπειλῆς καὶ φόνου εἰς τοὺς μαθητὰς τοῦ κυρίου, προσελθὼν τῷ ἀρχιερεῖ 2 ᾐτήσατο παρʼ αὐτοῦ ἐπιστολὰς εἰς Δαμασκὸν πρὸς τὰς συναγωγάς, ὅπως ἐάν τινας εὕρῃ τῆς ὁδοῦ ὄντας, ἄνδρας τε καὶ γυναῖκας, δεδεμένους ἀγάγῃ εἰς Ἰερουσαλήμ. 3 ἐν δὲ τῷ πορεύεσθαι ἐγένετο αὐτὸν ἐγγίζειν τῇ Δαμασκῷ, ⸂ἐξαίφνης τε αὐτὸν περιήστραψεν⸃ φῶς ⸀ἐκ τοῦ οὐρανοῦ, 4 καὶ πεσὼν ἐπὶ τὴν γῆν ἤκουσεν φωνὴν λέγουσαν αὐτῷ Σαοὺλ Σαούλ, τί με διώκεις; 5 εἶπεν δέ· Τίς εἶ, κύριε; ὁ ⸀δέ· Ἐγώ εἰμι Ἰησοῦς ὃν σὺ διώκεις· 6 ἀλλὰ ἀνάστηθι καὶ εἴσελθε εἰς τὴν πόλιν, καὶ λαληθήσεταί σοι ⸂ὅ τί⸃ σε δεῖ ποιεῖν. 7 οἱ δὲ ἄνδρες οἱ συνοδεύοντες αὐτῷ εἱστήκεισαν ἐνεοί, ἀκούοντες μὲν τῆς φωνῆς μηδένα δὲ θεωροῦντες. 8 ἠγέρθη δὲ ⸀Σαῦλος ἀπὸ τῆς γῆς, ἀνεῳγμένων ⸀δὲ τῶν ὀφθαλμῶν αὐτοῦ ⸀οὐδὲν ἔβλεπεν· χειραγωγοῦντες δὲ αὐτὸν εἰσήγαγον εἰς Δαμασκόν. 9 καὶ ἦν ἡμέρας τρεῖς μὴ βλέπων, καὶ οὐκ ἔφαγεν οὐδὲ ἔπιεν. 10 Ἦν δέ τις μαθητὴς ἐν Δαμασκῷ ὀνόματι Ἁνανίας, καὶ εἶπεν πρὸς αὐτὸν ⸂ἐν ὁράματι ὁ κύριος⸃· Ἁνανία. ὁ δὲ εἶπεν· Ἰδοὺ ἐγώ, κύριε. 11 ὁ δὲ κύριος πρὸς αὐτόν· ⸀Ἀναστὰς πορεύθητι ἐπὶ τὴν ῥύμην τὴν καλουμένην Εὐθεῖαν καὶ ζήτησον ἐν οἰκίᾳ Ἰούδα Σαῦλον ὀνόματι Ταρσέα, ἰδοὺ γὰρ προσεύχεται, 12 καὶ εἶδεν ⸂ἄνδρα ἐν ὁράματι Ἁνανίαν ὀνόματι⸃ εἰσελθόντα καὶ ἐπιθέντα αὐτῷ ⸀χεῖρας ὅπως ἀναβλέψῃ. 13 ἀπεκρίθη δὲ Ἁνανίας· Κύριε, ⸀ἤκουσα ἀπὸ πολλῶν περὶ τοῦ ἀνδρὸς τούτου, ὅσα κακὰ ⸂τοῖς ἁγίοις σου ἐποίησεν⸃ ἐν Ἰερουσαλήμ· 14 καὶ ὧδε ἔχει ἐξουσίαν παρὰ τῶν ἀρχιερέων δῆσαι πάντας τοὺς ἐπικαλουμένους τὸ ὄνομά σου. 15 εἶπεν δὲ πρὸς αὐτὸν ὁ κύριος· Πορεύου, ὅτι σκεῦος ἐκλογῆς ⸂ἐστίν μοι⸃ οὗτος τοῦ βαστάσαι τὸ ὄνομά μου ἐνώπιον ⸀ἐθνῶν ⸀τε καὶ βασιλέων υἱῶν τε Ἰσραήλ, 16 ἐγὼ γὰρ ὑποδείξω αὐτῷ ὅσα δεῖ αὐτὸν ὑπὲρ τοῦ ὀνόματός μου παθεῖν. 17 ἀπῆλθεν δὲ Ἁνανίας καὶ εἰσῆλθεν εἰς τὴν οἰκίαν, καὶ ἐπιθεὶς ἐπʼ αὐτὸν τὰς χεῖρας εἶπεν· Σαοὺλ ἀδελφέ, ὁ κύριος ἀπέσταλκέν με, ⸀Ἰησοῦς ὁ ὀφθείς σοι ἐν τῇ ὁδῷ ᾗ ἤρχου, ὅπως ἀναβλέψῃς καὶ πλησθῇς πνεύματος ἁγίου. 18 καὶ εὐθέως ἀπέπεσαν ⸂αὐτοῦ ἀπὸ τῶν ὀφθαλμῶν⸃ ⸀ὡς λεπίδες, ἀνέβλεψέν τε καὶ ἀναστὰς ἐβαπτίσθη, 19 καὶ λαβὼν τροφὴν ⸀ἐνίσχυσεν.Ὁ δὲ Σαῦλος ἔτι ἐμπνέων ἀπειλῆς καὶ φόνου εἰς τοὺς μαθητὰς τοῦ κυρίου, προσελθὼν τῷ ἀρχιερεῖ 2 ᾐτήσατο παρʼ αὐτοῦ ἐπιστολὰς εἰς Δαμασκὸν πρὸς τὰς συναγωγάς, ὅπως ἐάν τινας εὕρῃ τῆς ὁδοῦ ὄντας, ἄνδρας τε καὶ γυναῖκας, δεδεμένους ἀγάγῃ εἰς Ἰερουσαλήμ. 3 ἐν δὲ τῷ πορεύεσθαι ἐγένετο αὐτὸν ἐγγίζειν τῇ Δαμασκῷ, ⸂ἐξαίφνης τε αὐτὸν περιήστραψεν⸃ φῶς ⸀ἐκ τοῦ οὐρανοῦ, 4 καὶ πεσὼν ἐπὶ τὴν γῆν ἤκουσεν φωνὴν λέγουσαν αὐτῷ Σαοὺλ Σαούλ, τί με διώκεις; 5 εἶπεν δέ· Τίς εἶ, κύριε; ὁ ⸀δέ· Ἐγώ εἰμι Ἰησοῦς ὃν σὺ διώκεις· 6 ἀλλὰ ἀνάστηθι καὶ εἴσελθε εἰς τὴν πόλιν, καὶ λαληθήσεταί σοι ⸂ὅ τί⸃ σε δεῖ ποιεῖν. 7 οἱ δὲ ἄνδρες οἱ συνοδεύοντες αὐτῷ εἱστήκεισαν ἐνεοί, ἀκούοντες μὲν τῆς φωνῆς μηδένα δὲ θεωροῦντες. 8 ἠγέρθη δὲ ⸀Σαῦλος ἀπὸ τῆς γῆς, ἀνεῳγμένων ⸀δὲ τῶν ὀφθαλμῶν αὐτοῦ ⸀οὐδὲν ἔβλεπεν· χειραγωγοῦντες δὲ αὐτὸν εἰσήγαγον εἰς Δαμασκόν. 9 καὶ ἦν ἡμέρας τρεῖς μὴ βλέπων, καὶ οὐκ ἔφαγεν οὐδὲ ἔπιεν. 10 Ἦν δέ τις μαθητὴς ἐν Δαμασκῷ ὀνόματι Ἁνανίας, καὶ εἶπεν πρὸς αὐτὸν ⸂ἐν ὁράματι ὁ κύριος⸃· Ἁνανία. ὁ δὲ εἶπεν· Ἰδοὺ ἐγώ, κύριε. 11 ὁ δὲ κύριος πρὸς αὐτόν· ⸀Ἀναστὰς πορεύθητι ἐπὶ τὴν ῥύμην τὴν καλουμένην Εὐθεῖαν καὶ ζήτησον ἐν οἰκίᾳ Ἰούδα Σαῦλον ὀνόματι Ταρσέα, ἰδοὺ γὰρ προσεύχεται, 12 καὶ εἶδεν ⸂ἄνδρα ἐν ὁράματι Ἁνανίαν ὀνόματι⸃ εἰσελθόντα καὶ ἐπιθέντα αὐτῷ ⸀χεῖρας ὅπως ἀναβλέψῃ. 13 ἀπεκρίθη δὲ Ἁνανίας· Κύριε, ⸀ἤκουσα ἀπὸ πολλῶν περὶ τοῦ ἀνδρὸς τούτου, ὅσα κακὰ ⸂τοῖς ἁγίοις σου ἐποίησεν⸃ ἐν Ἰερουσαλήμ· 14 καὶ ὧδε ἔχει ἐξουσίαν παρὰ τῶν ἀρχιερέων δῆσαι πάντας τοὺς ἐπικαλουμένους τὸ ὄνομά σου. 15 εἶπεν δὲ πρὸς αὐτὸν ὁ κύριος· Πορεύου, ὅτι σκεῦος ἐκλογῆς ⸂ἐστίν μοι⸃ οὗτος τοῦ βαστάσαι τὸ ὄνομά μου ἐνώπιον ⸀ἐθνῶν ⸀τε καὶ βασιλέων υἱῶν τε Ἰσραήλ, 16 ἐγὼ γὰρ ὑποδείξω αὐτῷ ὅσα δεῖ αὐτὸν ὑπὲρ τοῦ ὀνόματός μου παθεῖν. 17 ἀπῆλθεν δὲ Ἁνανίας καὶ εἰσῆλθεν εἰς τὴν οἰκίαν, καὶ ἐπιθεὶς ἐπʼ αὐτὸν τὰς χεῖρας εἶπεν· Σαοὺλ ἀδελφέ, ὁ κύριος ἀπέσταλκέν με, ⸀Ἰησοῦς ὁ ὀφθείς σοι ἐν τῇ ὁδῷ ᾗ ἤρχου, ὅπως ἀναβλέψῃς καὶ πλησθῇς πνεύματος ἁγίου. 18 καὶ εὐθέως ἀπέπεσαν ⸂αὐτοῦ ἀπὸ τῶν ὀφθαλμῶν⸃ ⸀ὡς λεπίδες, ἀνέβλεψέν τε καὶ ἀναστὰς ἐβαπτίσθη, 19 καὶ λαβὼν τροφὴν ⸀ἐνίσχυσεν.
      )";

    c->body << bdy;
    c->markdown = true;

    c->build ();
    c->finalize ();

    BOOST_CHECK_MESSAGE (c->markdown_success, c->markdown_error);

    ustring fn = c->write_tmp ();

    Message m (fn);

    /* check plain text part */
    ustring pbdy = m.plain_text (false);
    BOOST_CHECK_MESSAGE (pbdy == bdy, "plain text matches plain text");

    /* check html part */
    BOOST_CHECK_MESSAGE (g_mime_content_type_is_type (m.root->content_type, "multipart", "alternative"), "main message part is multipart/alternative");

    auto plain = m.root->kids[0];
    auto html  = m.root->kids[1];

    BOOST_CHECK_MESSAGE (g_mime_content_type_is_type(plain->content_type, "text", "plain"), "first part is text/plain");
    BOOST_CHECK_MESSAGE (g_mime_content_type_is_type(html->content_type, "text", "html"), "second part is text/html");

    ustring _html = html->viewable_text (false);
    Astroid::UstringUtils::trim(_html);

    LOG (trace) << "markdown html: '" << _html << "'";


    BOOST_CHECK_MESSAGE (_html.find ("</pre>") != std::string::npos, "html part is correctly constructed");

    unlink (fn.c_str ());
    teardown ();
  }

BOOST_AUTO_TEST_SUITE_END()

