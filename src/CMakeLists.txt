cmake_minimum_required(VERSION 2.8.11...3.20)
project(lamiae)

# link_deps([FATAL] DEPS <deps>..)
# Calls link_directories for <deps>. <deps> is a list of dependencies.
# If FATAL is specified then it exits on failure. Otherwise it returns
# and sets link_deps_failed.
function(link_deps_dirs)
    cmake_parse_arguments(link_deps_dirs "FATAL" "TARGET" "DEPS" "${ARGN}")
    foreach(dep IN LISTS link_deps_dirs_DEPS)
        if(${link_deps_dirs_FATAL})
            pkg_check_modules(${dep} REQUIRED ${dep})
        else()
            pkg_check_modules(${dep} ${dep})
        endif()
        if(NOT ${dep}_FOUND)
            set(link_deps_dirs_failed true PARENT_SCOPE)
            return()
        endif()

        # tell the compiler where to find the libraries' binaries
        foreach(lib_dir IN LISTS ${dep}_LIBRARY_DIRS)
            message("library dir: ${lib_dir}")
            link_directories(${lib_dir})
        endforeach(lib_dir)
    endforeach(dep)
endfunction(link_deps_dirs)

# configure_deps(TARGET <target> [FATAL] DEPS <deps>...)
# Configures dependencies for <target>. <deps> is a list of dependencies.
# If FATAL is specified then it exits on failure. Otherwise it returns
# and sets configure_deps_failed.
function(configure_deps)
    cmake_parse_arguments(configure_deps "FATAL" "TARGET" "DEPS" "${ARGN}")
    foreach(dep IN LISTS configure_deps_DEPS)
        if(${configure_deps_FATAL})
            pkg_check_modules(${dep} REQUIRED ${dep})
        else()
            pkg_check_modules(${dep} ${dep})
        endif()
        if(NOT ${dep}_FOUND)
            set(configure_deps_failed true PARENT_SCOPE)
            return()
        endif()

        # add the necessary includes
        foreach(include_dir IN LISTS ${dep}_INCLUDE_DIRS)
            include_directories(${include_dir})
        endforeach(include_dir)

        # link to the libraries
        foreach(lib IN LISTS ${dep}_LIBRARIES)
            target_link_libraries(${configure_deps_TARGET} ${lib})
        endforeach(lib)
    endforeach(dep)
endfunction(configure_deps)

# set default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type (release or debug)" FORCE)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# set compile options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fsigned-char -ffast-math -fno-exceptions -fno-rtti -Wno-invalid-offsetof -std=c++11")

set(BIN_SUFFIX "64.bin")
set(PLATFORM_PATH "bin/unk")

# install to ../bin/
if(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "i[3-6]86")
	set(ARCHITECTURE "32")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "x86_64")
	set(ARCHITECTURE "64")
else()
    set(ARCHITECTURE "64") #The original Makefile assumes 64bit if in doubt
endif()

# Use pkg-config to configure dependencies later
find_package(PkgConfig REQUIRED)
# the dependencies required by all platforms
set(client_deps zlib sdl2 SDL2_image SDL2_mixer gl)
set(server_deps zlib)
set(tessfont_deps zlib freetype2)

# the client depends on almost all the source files
file(GLOB client_sources
    engine/*.cpp
    rpggame/*.cpp
    shared/*.cpp
)

# tessfont is a very simple application
file(GLOB tessfont_sources
    shared/tessfont.c
)

# neither server nor client need tessfont.cpp - to avoid warnings about duplicate main()s, it has to be removed from their source lists
file(GLOB tessfont_cpp_path shared/tessfont.c)
list(REMOVE_ITEM client_sources ${tessfont_cpp_path})

# platform specific code
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    list(APPEND client_deps x11)
    set(link_libs rt)
    link_libraries(${link_libs})

    #TODO docs talk about generator stuff
    if(CMAKE_BUILD_TYPE MATCHES DEBUG)
        set(BIN_SUFFIX "${ARCHITECTURE}.dbg")
    else()
        set(BIN_SUFFIX "${ARCHITECTURE}.bin")
    endif()
    set(PLATFORM_PATH "bin/unix")
elseif(APPLE)
    # build OS X specific Objective-C code
    file(GLOB mac_client_sources
        xcode/main.m xcode/macutils.mm
        xcode/SDLmain.m xcode/ConsoleView.m
    )
    file(GLOB mac_server_sources
        xcode/macutils.mm
    )
    list(APPEND client_sources ${mac_client_sources})
    list(APPEND server_sources ${mac_server_sources})

    #todo???
    set(OUTPUT_PATH_EXTENSION "_osx")
elseif(MINGW)
    link_libraries(ws2_32 winmm)
    set(BIN_SUFFIX ".exe")
    set(PLATFORM_PATH "bin/win${ARCHITECTURE}")
else()
    message(STATUS "Platform was not recognised, binaries will go to ${PLATFORM_PATH}")
endif()

# set up library directories
link_deps_dirs(FATAL DEPS ${client_deps})

# add the client executable and link it to enet
add_executable(lamiae${BIN_SUFFIX} ${client_sources})
target_link_libraries(lamiae${BIN_SUFFIX} enet)

if(APPLE)
    # include framework required in xcode/ code
    find_library(COCOA_LIBRARY Cocoa)
    target_link_libraries(lamiae${BIN_SUFFIX} ${COCOA_LIBRARY})
elseif(MINGW)
    #TODO link in resources and manifest
else()
    add_executable(tessfont${BIN_SUFFIX} ${tessfont_sources})
    set_target_properties(tessfont${BIN_SUFFIX} PROPERTIES
        COMPILE_FLAGS "-DSTANDALONE"
    )
    target_link_libraries(tessfont${BIN_SUFFIX} m)
endif()

# configure dependencies
configure_deps(TARGET lamiae${BIN_SUFFIX} FATAL DEPS ${client_deps})
configure_deps(TARGET tessfont${BIN_SUFFIX} FATAL DEPS ${tessfont_deps})

# configure enet
set(ENET_SOURCE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/enet)
add_subdirectory(${ENET_SOURCE_DIRECTORY})

# configure local includes
include_directories(
    ${ENET_SOURCE_DIRECTORY}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/rpggame
    ${CMAKE_CURRENT_SOURCE_DIR}/shared
    ${CMAKE_CURRENT_SOURCE_DIR}/support
)

# include the headers for the libraries bundled in ../bin
if(MINGW)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
endif()

set(targets lamiae${BIN_SUFFIX} tessfont${BIN_SUFFIX})

foreach(target IN LISTS targets)
    install(
        TARGETS ${target}
        DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/../${PLATFORM_PATH}/${ARCHITECTURE}/
    )
endforeach()
