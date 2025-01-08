cmake_minimum_required(VERSION 3.5...3.16)
project(QtGifImage)

set(CMAKE_CXX_STANDARD 17)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core Gui)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui)

set(LIB_LIBRARIES Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui)

set(GIF_IMAGE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/gifimage)
set(GIF_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/3rdparty/giflib)

# Next line was "add_library(${PROJECT_NAME} SHARED"
# but it breaks MXE static compilation
add_library(${PROJECT_NAME}
    ${GIF_IMAGE_DIR}/qgifglobal.h ${GIF_IMAGE_DIR}/qgifimage.cpp
    ${GIF_IMAGE_DIR}/qgifimage.h  ${GIF_IMAGE_DIR}/qgifimage_p.h

    ${GIF_LIB_DIR}/dgif_lib.c ${GIF_LIB_DIR}/egif_lib.c
    ${GIF_LIB_DIR}/gif_err.c  ${GIF_LIB_DIR}/gif_font.c
    ${GIF_LIB_DIR}/gif_hash.c ${GIF_LIB_DIR}/gif_hash.h
    ${GIF_LIB_DIR}/gif_lib.h  ${GIF_LIB_DIR}/gif_lib_private.h
    ${GIF_LIB_DIR}/gifalloc.c ${GIF_LIB_DIR}/quantize.c
)

add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/3rdparty)

target_link_libraries(${PROJECT_NAME} PRIVATE ${LIB_LIBRARIES})
