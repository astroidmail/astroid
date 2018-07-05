# shamelessly inspired by https://github.com/bro/cmake/blob/master/InstallSymlink.cmake
macro(install_symlink filepath sympath)
  get_filename_component(symdir ${sympath} DIRECTORY)
  install(DIRECTORY DESTINATION ${symdir})
  install(CODE "
     if (\"\$ENV{DESTDIR}\" STREQUAL \"\")
        execute_process(COMMAND \"${CMAKE_COMMAND}\" -E create_symlink ${filepath} ${sympath})
     else ()
        execute_process(COMMAND \"${CMAKE_COMMAND}\" -E create_symlink ${filepath} \$ENV{DESTDIR}/${sympath})
     endif ()
   ")
  install(CODE "message(\"-- Symlinking: ${sympath} -> ${filepath}\")")
endmacro(install_symlink)
