cmake_minimum_required(VERSION 3.0) 
project(fcserver)

set(EXECUTABLE_NAME "fcserver")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

# TODO: Make these work to turn OFF
option(USE_BUILTIN_WS "Use the included version of libwebsockets. Otherwise search the system" ON)
option(USE_BUILTIN_LIBUSB "Use the built-in libusb" ON)
option(APPEND_PLATFORM "Append the platform to the executable name" OFF)
option(WITH_INSTALL_TARGETS "Generate install targets used by make install and CPack for example" ON)
option(WITH_SYSTEMD_SERVICE "Creates an install target for a SystemD service" ON)
option(WITH_SYSTEMD_USER "Run the SystemD service using a special user. Name of the user can be changed using -DFCSERVER_USER=username" OFF)
set(FCSERVER_USER "fcserver" CACHE STRING "The user that is created after a debian package installation if WITH_SYSTEMD_USER is enabled")

# TODO: Enable installing init daemon instead

if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(LINUX ON)
endif()

if (WITH_SYSTEMD_SERVICE)
    if (NOT LINUX)
        message("Turning off WITH_SYSTEMD_SERVICE on non-Linux system")
        set(WITH_SYSTEMD_SERVICE OFF)
    endif()
endif()

# TODO: Make sure git submodules are loaded

#
# Set version based on git tag
#
find_package(Git)

set(FCSERVER_VERSION_MAJOR 0)
set(FCSERVER_VERSION_MINOR 0)
set(FCSERVER_VERSION_PATCH 0)
set(FCSERVER_VERSION_STR "unknown")
set(FCSERVER_RAW_VERSION_STR "unknown")

if (GIT_FOUND)
    macro (git_log_format FORMAT_CHARS VAR_NAME)
        execute_process(
            COMMAND ${GIT_EXECUTABLE} log -1 --pretty=format:%${FORMAT_CHARS}
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            OUTPUT_VARIABLE ${VAR_NAME}
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    endmacro()

    git_log_format(h GIT_HASH_SHORT)
    git_log_format(H GIT_HASH)
    git_log_format(an GIT_AUTHOR_EMAIL)
    git_log_format(ae GIT_AUTHOR_EMAIL)
    git_log_format(cn GIT_COMMITTER_NAME)
    git_log_format(ce GIT_COMMITTER_EMAIL)
    git_log_format(B GIT_COMMIT_MESSAGE)

    # Get version from tag.
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --match "fcserver-*"
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Get each version part
    string(REGEX REPLACE "^fcserver-?([0-9]+)\\..*" "\\1" FCSERVER_VERSION_MAJOR "${GIT_VERSION}")
    string(REGEX REPLACE "^fcserver-?[0-9]+\\.([0-9]+).*" "\\1" FCSERVER_VERSION_MINOR "${GIT_VERSION}")
    #string(REGEX REPLACE "^fcserver-?[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" FCSERVER_VERSION_PATCH "${GIT_VERSION}")
    set(FCSERVER_RAW_VERSION_STR "${GIT_VERSION}")
    string(REPLACE "fcserver-" ""
        FCSERVER_VERSION_STR "${GIT_VERSION}")
    message("Version: ${FCSERVER_VERSION_STR}")
endif()

# We use the raw git tag version of the string here.
add_definitions(-DFCSERVER_VERSION=${FCSERVER_RAW_VERSION_STR})

#
# Generate HTTP docs at build-time using a Python script
#
find_package(PythonInterp)

if (NOT PYTHONINTERP_FOUND)
    message(FATAL_ERROR "Python required to generate HTTP documentation")
endif()

message("Found Python ${PYTHON_VERSION_STRING}")

file(GLOB HTTP_DOCS_FILES
    "${PROJECT_SOURCE_DIR}/http/*.html"
    "${PROJECT_SOURCE_DIR}/http/js/*.js"
    "${PROJECT_SOURCE_DIR}/http/css/*.css")

message("HTTP DOC Files:\n${HTTP_DOCS_FILES}\n")

add_custom_command(
    OUTPUT "${PROJECT_BINARY_DIR}/httpdocs.cpp"
    COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/http/manifest.py > "${PROJECT_BINARY_DIR}/httpdocs.cpp"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/http/"
    DEPENDS ${HTTP_DOCS_FILES})

add_custom_target(http_docs DEPENDS "${PROJECT_BINARY_DIR}/httpdocs.cpp")

#
# Sources
#
set(SRC
    "${PROJECT_SOURCE_DIR}/src/main.cpp"
    "${PROJECT_SOURCE_DIR}/src/tcpnetserver.cpp"
    "${PROJECT_SOURCE_DIR}/src/usbdevice.cpp"
    "${PROJECT_SOURCE_DIR}/src/fcdevice.cpp"
    "${PROJECT_SOURCE_DIR}/src/enttecdmxdevice.cpp"
    "${PROJECT_SOURCE_DIR}/src/fcserver.cpp"
    "${PROJECT_SOURCE_DIR}/src/version.cpp"
    "${PROJECT_SOURCE_DIR}/src/tinythread.cpp"
    "${PROJECT_SOURCE_DIR}/src/spidevice.cpp"
    "${PROJECT_SOURCE_DIR}/src/apa102spidevice.cpp"
    "${PROJECT_BINARY_DIR}/httpdocs.cpp"
    )

source_group("Fadecandy Sources" FILES ${SRC})

include_directories(
    "${PROJECT_SOURCE_DIR}/src/")

if (UNIX)
    add_definitions("-Wno-strict-aliasing -MMD -Wno-unused-label")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti")
endif()

if (APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++0x -Wno-tautological-constant-out-of-range-compare")
endif()

#
# Pthreads
#
find_package(Threads)

#
# rapidjson (headers only, so "#include rapidjson/rapidjson.h" can be used)
#
include_directories("${PROJECT_SOURCE_DIR}/")

#
# Libwebsockets
#

if (USE_BUILTIN_WS)
    # Turn off unused libwebsockets parts.
    # TODO: Once libwebsockets version is updated these need to change, newer version uses prefix "LWS_*"
    set(WITHOUT_CLIENT ON CACHE BOOL "")
    set(WITHOUT_EXTENSIONS ON CACHE BOOL "")
    set(WITHOUT_TESTAPPS ON CACHE BOOL "")
    set(WITH_SSL OFF CACHE BOOL "")

    # Newer versions of CMake will complain since the libwebockets
    # version we're using is quite old.
    set(CMAKE_POLICY_DEFAULT_CMP0042 OLD)
    set(CMAKE_POLICY_DEFAULT_CMP0046 OLD)
    add_subdirectory("${PROJECT_SOURCE_DIR}/libwebsockets" EXCLUDE_FROM_ALL)
    include_directories("${PROJECT_SOURCE_DIR}/libwebsockets/lib")
    set(CMAKE_POLICY_DEFAULT_CMP0042 NEW)
    set(CMAKE_POLICY_DEFAULT_CMP0046 NEW)
else()
    # TODO: Would require a new version of libwebsockets supporting getting access to the socket. A custom version is used atm.
    message(FATAL_ERROR "Only USE_BUILTIN_WS currently supported")
endif()

#
# Libusb
#

# TODO: Allow using system libusb.

if (USE_BUILTIN_LIBUSB)
    set(LIBUSB_SRC
        "${PROJECT_SOURCE_DIR}/libusbx/libusb/core.c"
        "${PROJECT_SOURCE_DIR}/libusbx/libusb/descriptor.c"
        "${PROJECT_SOURCE_DIR}/libusbx/libusb/hotplug.c"
        "${PROJECT_SOURCE_DIR}/libusbx/libusb/io.c"
        "${PROJECT_SOURCE_DIR}/libusbx/libusb/strerror.c"
        "${PROJECT_SOURCE_DIR}/libusbx/libusb/sync.c")

    if (APPLE)
        list(APPEND LIBUSB_SRC
            "${PROJECT_SOURCE_DIR}/libusbx/libusb/os/darwin_usb.c"
            "${PROJECT_SOURCE_DIR}/libusbx/libusb/os/poll_posix.c"
            "${PROJECT_SOURCE_DIR}/libusbx/libusb/os/threads_posix.c")
    endif()

    if (LINUX)
        list(APPEND LIBUSB_SRC
            "${PROJECT_SOURCE_DIR}/libusbx/libusb/os/linux_usbfs.c"
            "${PROJECT_SOURCE_DIR}/libusbx/libusb/os/linux_netlink.c"
            "${PROJECT_SOURCE_DIR}/libusbx/libusb/os/poll_posix.c"
            "${PROJECT_SOURCE_DIR}/libusbx/libusb/os/threads_posix.c"
            )
    endif()

    include_directories("${PROJECT_SOURCE_DIR}/libusbx/libusb/")

    source_group("LibUSB Sources" FILES ${LIBUSB_SRC})

    list(APPEND SRC ${LIBUSB_SRC})
else()
    message(FATAL_ERROR "Only USE_BUILTIN_LIBUSB works yet")
endif()

add_executable(${EXECUTABLE_NAME} ${SRC})

target_link_libraries(${EXECUTABLE_NAME} stdc++ ${CMAKE_THREAD_LIBS_INIT} websockets)

# TODO: Do system introspection instead of hardcording these...

if (LINUX)
    target_link_libraries(${EXECUTABLE_NAME} rt)

    #
    # Libusb specific
    #
    add_definitions(
        -DOS_LINUX
        -DTHREADS_POSIX
        -DPOLL_NFDS_TYPE=nfds_t
        -DLIBUSB_CALL=
        -DDEFAULT_VISIBILITY=
        -DHAVE_GETTIMEOFDAY
        -DHAVE_POLL_H
        -DHAVE_ASM_TYPES_H
        -DHAVE_SYS_SOCKET_H
        -DHAVE_LINUX_NETLINK_H
        -DHAVE_LINUX_FILTER_H
        )
endif()

if (APPLE)
    add_definitions(-DHAVE_POLL_H)

    #
    # Libusb specific
    #
    add_definitions(-Wno-non-literal-null-conversion)
    target_link_libraries(${EXECUTABLE_NAME} "-framework CoreFoundation" "-framework IOKit" objc)

    add_definitions(
        -DOS_DARWIN
        -DTHREADS_POSIX
        -DPOLL_NFDS_TYPE=nfds_t
        -DLIBUSB_CALL=
        -DDEFAULT_VISIBILITY=
        -DHAVE_GETTIMEOFDAY)
endif()

#
# Install targets
#

if (WITH_INSTALL_TARGETS)
    install(TARGETS ${EXECUTABLE_NAME}
            RUNTIME DESTINATION "bin/" COMPONENT Runtime)

    if (WITH_SYSTEMD_SERVICE)
        # Possible to change at cmake generation if it is different on some system.
        set(SYSTEMD_CONFIG_DIR "/lib/systemd/system" CACHE STRING "Path to the systemd configuration dir")
        set(SYSTEMD_UNIT_NAME "${PROJECT_BINARY_DIR}/${EXECUTABLE_NAME}.service")

        if (WITH_SYSTEMD_USER)
            # TODO: Setup proper udev rules for FCSERVER_USER in the postinst.in script
            # currently this must be done manually which is quite tedious.
            # http://www.reactivated.net/writing_udev_rules.html

            # Run as specified user in FCSERVER_USER.
            configure_file("${PROJECT_SOURCE_DIR}/cmake/fcserver-user.service.in"
                           "${SYSTEMD_UNIT_NAME}")

            # Creates the user we want to run as after the debian package is installed.
            configure_file("${PROJECT_SOURCE_DIR}/cmake/debian/postinst.in"
                           "${PROJECT_BINARY_DIR}/debian/postinst" @ONLY)

            set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${PROJECT_BINARY_DIR}/debian/postinst")

            # TODO: This does not work currently, it will be run when CPack runs as well and create the user at that time.
            # If we are simply doing "make install" we run it at the end using CMake instead:
            #cpack_add_component(CreateUser
            #                    DISPLAY_NAME "Create service user"
            #                    DESCRIPTION "Create user that will run the SystemD service"
            #                    DISABLED)
            #install(CODE "execute_process(\"${PROJECT_BINARY_DIR}/debian/postinst\")"
            #        COMPONENT CreateUser OPTIONAL)
        else()
            # Run as systemd default user.
            configure_file("${PROJECT_SOURCE_DIR}/cmake/fcserver.service.in"
                           "${SYSTEMD_UNIT_NAME}")
        endif()

        install(FILES
            "${SYSTEMD_UNIT_NAME}"
            DESTINATION "${SYSTEMD_CONFIG_DIR}/" COMPONENT Runtime)
    endif()

    #
    # Packaging (currently only focused on Debian packages)
    #

    get_filename_component(PROJECT_ROOT ${PROJECT_SOURCE_DIR} DIRECTORY)

    if (UNIX)
        set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_ROOT}/LICENSE")

        # TODO: Set to proper version based on git describe
        set(CPACK_PACKAGE_VERSION_MAJOR "${FCSERVER_VERSION_MAJOR}")
        set(CPACK_PACKAGE_VERSION_MINOR "${FCSERVER_VERSION_MINOR}")
        set(CPACK_PACKAGE_VERSION_PATCH "${FCSERVER_VERSION_PATCH}")
        set(CPACK_PACKAGE_VENDOR "Micah Elizabeth Scott")
        set(CPACK_PACKAGE_CONTACT "https://github.com/scanlime/fadecandy")

        set(CPACK_GENERATOR DEB;TGZ)
        set(CPACK_PACKAGE_NAME "fcserver")
        set(CPACK_DEBIAN_PACKAGE_MAINTAINER "joakim.soderberg@gmail.com")
        set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "The Fadecandy Server is a background process that handles the USB communications with one or more Fadecandy Controller boards, and exposes those via the Open Pixel Protocol to TCP and Websocket clients")
        set(CPACK_PACKAGE_FILE_NAME "${EXECUTABLE_NAME}-${FCSERVER_VERSION_STR}")

        set(CPACK_SYSTEM_NAME "${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
        set(CPACK_SET_DESTDIR ON)

        # Include the CPU architecture in the package name.
        if (LINUX)
            # TODO: dpkg is debian specific, add support for other platforms.

            # Find the correct package architecture based on the platform we're building on.
            find_program(DPKG_CMD dpkg)
            if(NOT DPKG_CMD)
                message(WARNING "Debian architecture: Can not find dpkg in your path, default to i386.")
                set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE i386)
            else()
                execute_process(COMMAND "${DPKG_CMD}" --print-architecture
                    OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
            endif()

            set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}-${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")
        endif()


        # TODO: If support for using system version of libs add them ones here
        #set(CPACK_DEBIAN_PACKAGE_DEPENDS "")

        set(CPACK_DEBIAN_PACKAGE_SECTION "Miscellaneous")
        set(CPACK_DEBIAN_PACKAGE_PRIORITY "extra")

        # Needs to be last.
        include(CPack)
    endif()
endif()
