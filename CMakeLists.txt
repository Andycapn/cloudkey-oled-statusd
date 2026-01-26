cmake_minimum_required(VERSION 3.10)
project(cloudkey_oled_statusd C)

set(CMAKE_C_STANDARD 11)

include_directories(assets)
include_directories(fb)
include_directories(stats)
include_directories(ui)

add_library(fb
        fb/bitmap.c
        fb/bitmap.h
        fb/bitmap_loader.c
        fb/bitmap_loader.h
        fb/draw.c
        fb/draw.h
        fb/fb.c
        fb/fb.h
        fb/text.c
        fb/text.h)

add_library(ui
        ui/render_network.c
        ui/render_network.h
        ui/render_state.c
        ui/render_state.h
        ui/state_icons.c
        ui/state_icons.h
        ui/ui_manager.c
        ui/ui_manager.h
        ui/screens/status.c
        ui/screens/network.c
        ui/screens/hello.c
        ui/screens/splash.c
        ui/screens.h)

if(SIMULATOR)
    target_compile_definitions(fb PUBLIC SIMULATOR)
    target_compile_definitions(ui PRIVATE ICON_PATH="${CMAKE_SOURCE_DIR}/assets/icons/")
    target_link_libraries(fb PUBLIC X11)
endif()

target_link_libraries(ui fb)

add_executable(oled-statusd
        main.c)

add_library(stats
        stats/host.c
        stats/host.h
        stats/ip.c
        stats/ip.h
        stats/network.c
        stats/network.h
        stats/state.c
        stats/state.h
        stats/system.c
        stats/system.h)

target_link_libraries(oled-statusd ui stats)

install(TARGETS oled-statusd DESTINATION bin)
install(DIRECTORY assets/icons/ DESTINATION share/oled-status/icons)
