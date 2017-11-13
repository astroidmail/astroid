# Copyright (C) 2010, Pino Toscano, <pino@kde.org>
#           (C) 2017, Alexander Adolf <alexander.adolf@condition-alpha.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
# Sample use:
#
# include (FindGObjectIntrospection)   # this performs a pkg_check_modules()
# if(INTROSPECTION_FOUND)
#   include (GObjectIntrospectionMacros)
#   set (GIR_NAME "${PROJECT_NAME}-${PROJECT_VERSION}.gir")
#   set (${GIR_NAME}_NAMESPACE      ${PROJECT_NAME})
#   set (${GIR_NAME}_VERSION        ${PROJECT_VERSION})
#   set (${GIR_NAME}_INCLUDEPKGS    GObject-2.0)   # add more as needed
#   set (${GIR_NAME}_PROGRAM        ${CMAKE_CURRENT_BINARY_DIR}/gir_main)
#   set (${GIR_NAME}_FILES
#     #
#     # your source files here
#     #
#     )
#   list ( APPEND INTROSPECTION_GIRS ${GIR_NAME} )
#   set (INTROSPECTION_SCANNER_ARGS --warn-all)
#   gir_add_introspections (INTROSPECTION_GIRS)
#   #
#   # define your gir_main target here
#   #
#   # must build gir_main first in order to be able to infer typelibs
#   add_dependencies ( gir-typelibs gir_main )
# endif()
#
#

# add_custom_target(gir-girs)
add_custom_target(gir-typelibs)
# add_dependencies(gir-typelibs gir-girs)

set(_gir_specific_var_suffixes
  CFLAGS
  EXPORT_PACKAGES
  INCLUDEPKGS
  INCLUDES
  LIBS
  NAMESPACE
  PACKAGES
  PROGRAM
  VERSION
  )

macro(_gir_prefix_list_elements _outvar _listvar _prefix)
  set(${_outvar})
  foreach(_item IN LISTS ${_listvar})
    list(APPEND ${_outvar} ${_prefix}${_item})
  endforeach()
endmacro(_gir_prefix_list_elements)

macro(_gir_mangle_as_variable_name _name)
  # Transform a string to something which can be referenced through a variable
  # without automake/make complaining, eg Gtk-2.0.gir -> Gtk_2_0_gir
  string(REPLACE "-" "_" ${_name} "${${_name}}")
  string(REPLACE "." "_" ${_name} "${${_name}}")
endmacro(_gir_mangle_as_variable_name)

macro(_gir_add_introspections girs_list)
  # set(_gir_girs)
  # set(_gir_typelibs)

  foreach(_gir IN LISTS ${girs_list})

    set(_gir_name ${_gir})
    _gir_mangle_as_variable_name(_gir_name)

    # Namespace and Version is either fetched from the gir filename
    # or the _NAMESPACE/_VERSION variable combo
    set(_gir_namespace "${${_gir_name}_NAMESPACE}")
    if (_gir_namespace STREQUAL "")
      string(REGEX REPLACE "([^-]+)-.*" "\\1" _gir_namespace "${_gir}")
    endif ()
    set(_gir_version "${${_gir_name}_VERSION}")
    if (_gir_version STREQUAL "")
      string(REGEX REPLACE ".*-([^-]+).gir" "\\1" _gir_version "${_gir}")
    endif ()

    # _PROGRAM is an optional variable which needs it's own --program argument
    set(_gir_program "${${_gir_name}_PROGRAM}")
    if (NOT _gir_program STREQUAL "")
      set(_gir_program "--program=${_gir_program}")
    endif ()

    # Variables which provide lists of things
    _gir_prefix_list_elements(_gir_libraries ${_gir_name}_LIBS "--library=")
    _gir_prefix_list_elements(_gir_packages ${_gir_name}_PACKAGES "--pkg=")
    _gir_prefix_list_elements(_gir_includepkgs ${_gir_name}_INCLUDEPKGS "--include=")
    _gir_prefix_list_elements(_gir_includes ${_gir_name}_INCLUDES "-I")
    _gir_prefix_list_elements(_gir_export_packages ${_gir_name}_EXPORT_PACKAGES "--pkg-export=")

    # FIXME: reuse the LIBTOOL variable from automake if it's set
    set(_gir_libtool "--no-libtool")

    add_custom_command(
      COMMAND ${INTROSPECTION_SCANNER}
              ${INTROSPECTION_SCANNER_ARGS}
              --namespace=${_gir_namespace}
              --nsversion=${_gir_version}
    	      --add-include-path=${CMAKE_CURRENT_BINARY_DIR}
              --output ${CMAKE_CURRENT_BINARY_DIR}/${_gir}
              ${_gir_libtool}
              ${_gir_program}
              ${_gir_libraries}
              ${_gir_packages}
              ${_gir_includepkgs}
              ${_gir_includes}
              ${_gir_export_packages}
              ${${_gir_name}_SCANNERFLAGS}
              ${${_gir_name}_CFLAGS}
              ${${_gir_name}_FILES}
      DEPENDS ${${_gir_name}_FILES}
              ${${_gir_name}_LIBS}
	      ${${_gir_name}_PROGRAM}
      OUTPUT  ${_gir}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      VERBATIM
      )
    add_custom_target(${_gir_name}_gir DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${_gir})
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${_gir} DESTINATION ${SHARE_INSTALL_DIR}/gir-1.0)

    string(REPLACE ".gir" ".typelib" _typelib "${_gir}")
    add_custom_command(
      COMMAND ${INTROSPECTION_COMPILER}
              ${INTROSPECTION_COMPILER_ARGS}
              --includedir=.
              ${CMAKE_CURRENT_BINARY_DIR}/${_gir}
              -o ${CMAKE_CURRENT_BINARY_DIR}/${_typelib}
      DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${_gir}
      OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/${_typelib}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      )
    add_custom_target(${_gir_name}_typelib DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${_typelib})
    add_dependencies(${_gir_name}_typelib ${_gir_name}_gir)
    add_dependencies(gir-typelibs ${_gir_name}_typelib)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${_typelib} DESTINATION lib${LIB_SUFFIX}/girepository-1.0)

  endforeach()
endmacro(_gir_add_introspections)

macro(gir_add_introspections introspection_girs)
  foreach(gir IN LISTS ${introspection_girs})
    set(_name ${gir})
    _gir_mangle_as_variable_name(_name)
    _gir_prefix_list_elements(${_name}_FILES ${gir}_FILES "${CMAKE_CURRENT_SOURCE_DIR}/")
    foreach(_gir_var_suffix IN LISTS _gir_specific_var_suffixes)
      set(${_name}_${_gir_var_suffix} ${${gir}_${_gir_var_suffix}})
    endforeach()
  endforeach()
  _gir_add_introspections(${introspection_girs})
endmacro(gir_add_introspections)
