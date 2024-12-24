# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/Users/kevinyu/Projects/mobile-ai/app/src/main/cpp/_deps/jsoncpp-src")
  file(MAKE_DIRECTORY "/Users/kevinyu/Projects/mobile-ai/app/src/main/cpp/_deps/jsoncpp-src")
endif()
file(MAKE_DIRECTORY
  "/Users/kevinyu/Projects/mobile-ai/app/src/main/cpp/_deps/jsoncpp-build"
  "/Users/kevinyu/Projects/mobile-ai/app/src/main/cpp/_deps/jsoncpp-subbuild/jsoncpp-populate-prefix"
  "/Users/kevinyu/Projects/mobile-ai/app/src/main/cpp/_deps/jsoncpp-subbuild/jsoncpp-populate-prefix/tmp"
  "/Users/kevinyu/Projects/mobile-ai/app/src/main/cpp/_deps/jsoncpp-subbuild/jsoncpp-populate-prefix/src/jsoncpp-populate-stamp"
  "/Users/kevinyu/Projects/mobile-ai/app/src/main/cpp/_deps/jsoncpp-subbuild/jsoncpp-populate-prefix/src"
  "/Users/kevinyu/Projects/mobile-ai/app/src/main/cpp/_deps/jsoncpp-subbuild/jsoncpp-populate-prefix/src/jsoncpp-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/kevinyu/Projects/mobile-ai/app/src/main/cpp/_deps/jsoncpp-subbuild/jsoncpp-populate-prefix/src/jsoncpp-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/kevinyu/Projects/mobile-ai/app/src/main/cpp/_deps/jsoncpp-subbuild/jsoncpp-populate-prefix/src/jsoncpp-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
