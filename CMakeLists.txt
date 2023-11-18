# Serene Programming Language
#
# Copyright (c) 2019-2023 Sameer Rahmani <lxsameer@gnu.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 2.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

find_package(BDWgc 8.2.0 REQUIRED)

# Main Binary =================================================================
add_executable(serene)

set_target_properties(serene PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
  C_INCLUDE_WHAT_YOU_USE ${iwyu}
  CXX_INCLUDE_WHAT_YOU_USE ${iwyu}
  # Warn on unused libs
  LINK_WHAT_YOU_USE TRUE
)

if (CPP_20_SUPPORT)
  target_compile_features(serene PRIVATE cxx_std_20)
else()
  target_compile_features(serene PRIVATE cxx_std_17)
endif()

# Setup header directory and auto generated headers
target_include_directories(serene
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_include_directories(serene SYSTEM PUBLIC
  # We don't want the generated files from table gen
  # to be treated as local since the contain warnings
  ${PROJECT_BINARY_DIR}/serene/include)

target_link_libraries(serene PRIVATE
  LLVMSupport
)

# Autogenerate the `config.h` file
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/serene/config.h.in include/serene/config.h)


target_compile_options(serene
  PRIVATE
  $<$<NOT:$<BOOL:${SERENE_DISABLE_LIBCXX}>>:-stdlib=libc++>

  # LLVM has it's own RTTI
  -fno-rtti
  -fno-builtin-strlen

  # Dedicate a section to each function, so the linker
  # can do a better job on dead code elimination
  -ffunction-sections
  -fdata-sections

  $<$<CONFIG:DEBUG>:-fsanitize=address>
  $<$<CONFIG:DEBUG>:-static-libsan>
  $<$<CONFIG:DEBUG>:-g3>
  $<$<CONFIG:DEBUG>:-O0>
  $<$<CONFIG:DEBUG>:-ggdb>
  # For the sake of debugging
  $<$<CONFIG:DEBUG>:-fno-inline>
  # To make the local ccache happy
  $<$<CONFIG:DEBUG>:-fdebug-prefix-map=${PROJECT_SOURCE_DIR}=.>

  # No tail call elimination on Debug to let asan provide
  # better stacktrackes
  $<$<CONFIG:DEBUG>:-fno-optimize-sibling-calls>

  $<$<CONFIG:DEBUG>:-fno-omit-frame-pointer>
  $<$<CONFIG:RELEASE>:-fomit-frame-pointer>
  $<$<CONFIG:RELEASE>:-O3>
  $<$<CONFIG:RELEASE>:-fmerge-all-constants>
)

target_link_options(serene PRIVATE
  $<$<NOT:$<BOOL:${SERENE_DISABLE_LIBCXX}>>:-stdlib=libc++>
  $<$<NOT:$<BOOL:${SERENE_DISABLE_LIBCXX}>>:-lc++abi>
  $<$<NOT:$<BOOL:${SERENE_DISABLE_COMPILER_RT}>>:--rtlib=compiler-rt>

  -Wl,--gc-sections
  $<$<CONFIG:RELEASE>:-s>

  $<$<CONFIG:DEBUG>:-fsanitize=address>
  $<$<CONFIG:DEBUG>:-static-libsan>
  # Do not link against shared libraries
  --static
)


if(SERENE_ENABLE_BUILDID)
  target_link_options(serene -Wl,--build-id)
endif()

if(SERENE_ENABLE_TIDY)
  set_target_properties(serene PROPERTIES CXX_CLANG_TIDY ${CLANG_TIDY_PATH})
endif()

if(SERENE_ENABLE_THINLTO)
  include(CheckIPOSupported)
  # Optional IPO. Do not use IPO if it's not supported by compiler.
  check_ipo_supported(RESULT result OUTPUT output)
  if(result)
    message(STATUS "IPO is supported and is turned on")
    set_property(TARGET serene PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
  else()
    message(WARNING "IPO is not supported: ${output}")
    message(WARNING "Make sure to use lld")
  endif()
endif()


include(GNUInstallDirs)

install(TARGETS serene EXPORT SereneTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

add_subdirectory(src)
add_subdirectory(include)
