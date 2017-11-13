##
# Tests
find_package ( Boost REQUIRED
  COMPONENTS
  unit_test_framework
  )

function (add_astroid_test name target source)

  add_executable (
    ${target}

    ${source}
    )

  target_link_libraries (
    ${target}

    astr
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    )

  add_test (NAME ${name} COMMAND run_test.sh ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${target})

endfunction()

configure_file (run_test.sh run_test.sh COPYONLY)


# Add the tests

add_astroid_test (generic   test_generic          test_generic.cc)
add_astroid_test (compose   test_composed_message test_composed_message.cc)
add_astroid_test (markdown  test_markdown         test_markdown.cc)
add_astroid_test (non_existant test_non_existant_file test_non_existant_file.cc)
add_astroid_test (open_db   test_open_db          test_open_db.cc)
add_astroid_test (convert_error test_convert_error test_convert_error.cc)
add_astroid_test (no_newline test_no_newline_msg  test_no_newline_msg.cc)
add_astroid_test (mime_message test_mime_message  test_mime_message.cc)
add_astroid_test (theme     test_theme            test_theme.cc)
add_astroid_test (keybindings test_keybindings    test_keybindings.cc)
add_astroid_test (bad_content_id test_bad_content_id test_bad_content_id.cc)
add_astroid_test (notmuch   test_notmuch          test_notmuch.cc)
add_astroid_test (notmuch_standalone test_notmuch_standalone test_notmuch_standalone.cc)
add_astroid_test (address   test_address          test_address.cc)
add_astroid_test (dates     test_dates            test_dates.cc)
add_astroid_test (crypto    test_crypto           test_crypto.cc)
add_astroid_test (gmime_version test_gmime_version test_gmime_version.cc)




