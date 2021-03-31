qt5_add_resources(kmailquick_SRCS resources.qrc)

add_executable(kmailquick
    main.cpp
    quickmail.cpp
    mailmodel.cpp
    ${kmailquick_SRCS}
)
    
target_link_libraries(kmailquick
    Qt5::Core
    Qt5::Gui
    Qt5::Qml
    Qt5::Quick
    Qt5::QuickControls2
    Qt5::Svg
    KF5::I18n
    KF5::ItemModels
    KF5::Mime
    KF5::AkonadiMime
    KF5::AkonadiCore
    KF5::AkonadiAgentBase
    KF5::AkonadiWidgets
    KF5::AkonadiXml
)
install(TARGETS kmailquick ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})
