cmake_minimum_required(VERSION 3.16)
project(ComicChatNG VERSION 2.5.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Gui)
find_package(ZLIB REQUIRED)

set(CCNG_SOURCES
    src/main.cpp
    src/avbstream.cpp
    src/avbimage.cpp
    src/avatar.cpp
    src/backdrop.cpp
    src/artmanager.cpp
    src/emotions.cpp
    src/rules.cpp
    src/balloon.cpp
    src/panel.cpp
    src/pageview.cpp
    src/emotionpicker.cpp
    src/ircclient.cpp
    src/appsettings.cpp
    src/connectdialog.cpp
    src/setupdialog.cpp
    src/roomwidget.cpp
    src/mainwindow.cpp
    src/icons.cpp
)

set(CCNG_RESOURCES res/icons.qrc)

set(CCNG_HEADERS
    include/avbformat.h
    include/avbstream.h
    include/avbimage.h
    include/avatar.h
    include/backdrop.h
    include/artmanager.h
    include/emotions.h
    include/rules.h
    include/balloon.h
    include/panel.h
    include/pageview.h
    include/emotionpicker.h
    include/ircclient.h
    include/appsettings.h
    include/connectdialog.h
    include/setupdialog.h
    include/roomwidget.h
    include/mainwindow.h
    include/icons.h
)

add_executable(ComicChatNG ${CCNG_SOURCES} ${CCNG_HEADERS} ${CCNG_RESOURCES})

target_include_directories(ComicChatNG PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(ComicChatNG PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::Gui
    ZLIB::ZLIB
)

# Default art next to the modern tree (overridable at runtime).
target_compile_definitions(ComicChatNG PRIVATE
    CCNG_DEFAULT_ART_DIR="${CMAKE_SOURCE_DIR}/v2.5-beta-1-modern/comicart"
)

install(TARGETS ComicChatNG RUNTIME DESTINATION bin)
