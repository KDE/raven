# Copyright 2021 Carl Schwan <carl@carlschwan.eu>
# Copyright 2022 Devin Lin <devin@kde.org>
# SPDX-License-Identifier: GPL-2.0-or-later

cmake_minimum_required(VERSION 3.16)

project(raven)

set (CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT_VERSION "0.1")
set(QT_MIN_VERSION "5.15.0")
set(KF5_MIN_VERSION "5.101.0")
set(LIBKDEPIM_VERSION "5.19.0")

include(FeatureSummary)

################# set KDE specific information #################
find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(ECMPoQmTools)
include(KDEClangFormat)
include(ECMQmlModule)

ecm_setup_version(${PROJECT_VERSION}
    VARIABLE_PREFIX RAVEN
    VERSION_HEADER ${CMAKE_CURRENT_BINARY_DIR}/version.h
)

################# Find dependencies #################
find_package(Qt5 ${QT_MIN_VERSION} REQUIRED COMPONENTS 
    Core 
    Gui 
    Qml 
    QuickControls2 
    Svg 
    Sql 
    WebEngineWidgets
)

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS 
    Kirigami2 
    I18n 
    CoreAddons 
    Declarative 
    ItemModels
    KCMUtils
    KIO
    Wallet
    DBusAddons
)

find_package(KF5 ${LIBKDEPIM_VERSION} REQUIRED COMPONENTS
    Mime
)

find_package(QCoro${QT_MAJOR_VERSION} COMPONENTS Core DBus REQUIRED)
qcoro_enable_coroutines()

find_package(KF5KirigamiAddons 0.6 REQUIRED)

# Dependencies of MailCore and co.
find_package(OpenSSL REQUIRED)
find_package(ICU COMPONENTS i18n uc REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(LibLZMA REQUIRED)
find_package(ZLIB REQUIRED)
find_library(RESOLV_LIB resolv)
find_library(CTEMPLATE_LIB ctemplate)
find_library(ETPAN_LIB etpan)
find_library(TIDY_LIB tidy)
find_package(PkgConfig REQUIRED)
pkg_search_module(UUID uuid REQUIRED IMPORTED_TARGET)
pkg_search_module(GLIB glib-2.0 REQUIRED IMPORTED_TARGET)

find_package(MailCore REQUIRED)

################# build and install #################

add_definitions(-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII -DQT_NO_URL_CAST_FROM_STRING)
add_definitions(-DQT_USE_QSTRINGBUILDER)
add_definitions(-DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT)
add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x050d00)
add_definitions(-DQT_STRICT_ITERATORS)

add_subdirectory(src)

install(PROGRAMS org.kde.raven.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES org.kde.raven.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
install(FILES org.kde.raven.svg DESTINATION ${KDE_INSTALL_FULL_ICONDIR}/hicolor/scalable/apps/)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES src/*.cpp src/*.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
        
