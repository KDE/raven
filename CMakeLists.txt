# Copyright 2021 Carl Schwan <carl@carlschwan.eu>
# Copyright 2022 Devin Lin <devin@kde.org>
# SPDX-License-Identifier: GPL-2.0-or-later

cmake_minimum_required(VERSION 3.16)

project(raven)

set (CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT_VERSION "0.1")
set(QT_MIN_VERSION "6.8.0")
set(KF_MIN_VERSION "6.8.0")
set(LIBKDEPIM_VERSION "6.1.0")

include(FeatureSummary)

################# set KDE specific information #################
find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDEClangFormat)
include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(ECMPoQmTools)
include(ECMQmlModule)
include(ECMConfiguredInstall)
include(ECMFindQmlModule)

ecm_setup_version(${PROJECT_VERSION}
    VARIABLE_PREFIX RAVEN
    VERSION_HEADER ${CMAKE_CURRENT_BINARY_DIR}/version.h
)

################# Find dependencies #################
find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS
    Core
    Gui
    Qml
    QuickControls2
    Svg
    Sql
    WebEngineWidgets
)

find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS
    Kirigami
    I18n
    CoreAddons
    Declarative
    ItemModels
    KCMUtils
    KIO
    Wallet
    DBusAddons
    Config
)

find_package(KPim6Mime REQUIRED)

find_package(QCoro${QT_MAJOR_VERSION} COMPONENTS Core DBus REQUIRED)
qcoro_enable_coroutines()

find_package(KF6KirigamiAddons 0.6 REQUIRED)

# For daemon
find_package(Corrosion)
set_package_properties(Corrosion PROPERTIES
    TYPE RECOMMENDED
    PURPOSE "Required to build the daemon"
    DESCRIPTION "CMake scripts to seamlessly build and link to targets using cargo"
    URL https://github.com/AndrewGaspar/corrosion
)

pkg_search_module(UUID uuid REQUIRED IMPORTED_TARGET)

################# build and install #################

add_definitions(-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII -DQT_NO_URL_CAST_FROM_STRING)
add_definitions(-DQT_USE_QSTRINGBUILDER)
add_definitions(-DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT)
add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x050d00)
add_definitions(-DQT_STRICT_ITERATORS)

add_subdirectory(src)

install(PROGRAMS org.kde.raven.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(PROGRAMS org.kde.ravend-autostart.desktop DESTINATION ${KDE_INSTALL_AUTOSTARTDIR})
install(FILES org.kde.raven.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
install(FILES org.kde.raven.svg DESTINATION ${KDE_INSTALL_FULL_ICONDIR}/hicolor/scalable/apps/)

ecm_install_configured_files(INPUT org.kde.ravend.service.in DESTINATION ${KDE_INSTALL_DBUSSERVICEDIR} @ONLY)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES src/*.cpp src/*.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})

