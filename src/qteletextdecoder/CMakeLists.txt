file (GLOB SOURCES *.cpp)

add_library(qteletextdecoder ${SOURCES} teletextfonts.qrc)

target_include_directories(qteletextdecoder PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(qteletextdecoder Qt::Widgets)
