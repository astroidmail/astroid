##
# Tests
find_package ( Boost REQUIRED
  COMPONENTS
  unit_test_framework
  )

add_executable ( 
  test_generic

  test_generic.cc
  )

target_link_libraries (
  test_generic

  astr
  ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
  )

add_test (NAME generic COMMAND test_generic)



