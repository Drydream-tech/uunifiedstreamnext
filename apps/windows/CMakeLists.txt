cmake_minimum_required(VERSION 3.24)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2)

qt_standard_project_setup()

qt_add_executable(UnifiedStreamNext
    main.cpp
    ConnectionBridge.hpp
    ConnectionBridge.cpp
)

qt_add_qml_module(UnifiedStreamNext
    URI UnifiedStreamNext
    VERSION 1.0
    QML_FILES
        qml/Main.qml
)

target_link_libraries(UnifiedStreamNext PRIVATE
    usn_core
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
)
