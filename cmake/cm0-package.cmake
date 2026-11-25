#
# SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
#
# SPDX-License-Identifier: MIT
# Packaging and install layout for CardputerZero Debian builds.

include(GNUInstallDirs)

set(APP_DISPLAY_NAME "TemplateApp" CACHE STRING "Human-readable application name used by launchers and package filename")
set(APP_DEBIAN_REVISION "rev1" CACHE STRING "Debian package revision/vendor suffix")
set(APP_DEBIAN_ARCHITECTURE "arm64" CACHE STRING "Debian package architecture")
# Deliberately a placeholder: the store rejects packages whose Maintainer is
# still a template/M5Stack identity, so set your own name and email here.
set(APP_MAINTAINER "TODO Your Name <todo@example.invalid>" CACHE STRING "Debian package maintainer")
set(APP_PACKAGE_DESCRIPTION "CardputerZero LVGL template application" CACHE STRING "Debian package summary")
set(APP_INSTALL_SYSTEMD_SERVICE ON CACHE BOOL "Install a systemd service file for embedded deployments")

set(APP_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/package")
configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/templates/app.desktop.in"
    "${APP_GENERATED_DIR}/${PROJECT_NAME}.desktop"
    @ONLY
)

if(APP_INSTALL_SYSTEMD_SERVICE)
    configure_file(
        "${CMAKE_CURRENT_LIST_DIR}/templates/app.service.in"
        "${APP_GENERATED_DIR}/${PROJECT_NAME}.service"
        @ONLY
    )
endif()

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/config/template-app.conf"
    DESTINATION "/etc"
)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets/audio/"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/${APP_NAME}/audio"
    PATTERN ".DS_Store" EXCLUDE
)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/${APP_NAME}/fonts"
    PATTERN ".DS_Store" EXCLUDE
)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets/images/"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/${APP_NAME}/images"
    PATTERN ".DS_Store" EXCLUDE
)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets/images/"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/APPLaunch/share/images"
    FILES_MATCHING
    PATTERN "template*.png"
)

install(FILES "${APP_GENERATED_DIR}/${PROJECT_NAME}.desktop"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/APPLaunch/applications"
)

if(APP_INSTALL_SYSTEMD_SERVICE)
    install(FILES "${APP_GENERATED_DIR}/${PROJECT_NAME}.service"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/systemd/system"
    )
endif()

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
    DESTINATION "${CMAKE_INSTALL_DOCDIR}"
)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/LICENSE.txt"
    DESTINATION "${CMAKE_INSTALL_DOCDIR}"
    RENAME "third-party-assets-license.txt"
)

set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
set(CPACK_OUTPUT_FILE_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/dist")
set(CPACK_PACKAGE_NAME "${APP_DISPLAY_NAME}")
set(CPACK_PACKAGE_VENDOR "${APP_MAINTAINER}")
set(CPACK_PACKAGE_CONTACT "${APP_MAINTAINER}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${APP_PACKAGE_DESCRIPTION}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_FILE_NAME "${APP_DISPLAY_NAME}_${PROJECT_VERSION}_${APP_DEBIAN_REVISION}_${APP_DEBIAN_ARCHITECTURE}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/LICENSE.txt")

string(TOLOWER "${APP_DISPLAY_NAME}" APP_DEBIAN_PACKAGE_NAME)
string(REGEX REPLACE "[^a-z0-9+.-]" "-" APP_DEBIAN_PACKAGE_NAME "${APP_DEBIAN_PACKAGE_NAME}")
set(CPACK_DEBIAN_PACKAGE_NAME "${APP_DEBIAN_PACKAGE_NAME}")
set(CPACK_DEBIAN_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_DEBIAN_PACKAGE_RELEASE "${APP_DEBIAN_REVISION}")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "${APP_DEBIAN_ARCHITECTURE}")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${APP_MAINTAINER}")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(APP_DEBIAN_DEPENDS "libc6, libstdc++6, libgcc-s1, libfreetype6, libpng16-16, libjpeg62-turbo, zlib1g")
if(FMT_INCLUDE_DIR AND FMT_LIBRARY)
    string(APPEND APP_DEBIAN_DEPENDS ", libfmt10")
endif()
# Generate Depends from the binary where we can, and keep the list above only
# as the fallback.
#
# The hand-written list cannot track what the binary actually links. On this
# template as it stands it already over-declares: it names libjpeg62-turbo and
# zlib1g, which the built executable does not link. Over-declaring only pulls
# extra packages, so it is harmless and easy not to notice -- but the same
# mechanism under-declares the moment anyone links something new, and that
# ships a package dpkg will happily install onto a system missing the library,
# where it dies at the dynamic linker saying nothing useful.
#
# dpkg-shlibdeps reads the ELF, so it cannot drift, and it adds the minimum
# versions a hand-written list has no way to know.
#
# It needs the "file" utility, and CPackDeb fails the whole package rather
# than warning when it is absent, so this checks first and falls back to the
# hand-written list instead of breaking builds in environments that lack it.
find_program(APP_FILE_UTILITY file)
if(APP_FILE_UTILITY)
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    # Left empty on purpose: merging the two produced duplicate entries, and
    # the generated list is a superset of the useful part of the manual one.
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "")
else()
    message(WARNING
        "file(1) not found: falling back to the hand-written dependency list, "
        "which does not track what the binary links. Install the 'file' "
        "package for generated dependencies.")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "${APP_DEBIAN_DEPENDS}")
endif()
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION TRUE)

configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/templates/conffiles.in"
    "${APP_GENERATED_DIR}/conffiles"
    @ONLY
)
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${APP_GENERATED_DIR}/conffiles")

include(CPack)
