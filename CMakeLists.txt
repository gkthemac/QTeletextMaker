cmake_minimum_required(VERSION 3.16.0)

project(qteletextmaker VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Core Widgets REQUIRED)

add_subdirectory(src/qteletextdecoder)
add_subdirectory(3rdparty/QtGifImage)

file (GLOB SOURCES src/qteletextmaker/*.cpp)
add_executable(qteletextmaker ${SOURCES}
	src/qteletextmaker/actionicons.qrc
	src/qteletextmaker/appicon.qrc
)

target_link_libraries(qteletextmaker PRIVATE QtGifImage::QtGifImage qteletextdecoder Qt::Widgets)

if(WIN32)
	set_target_properties(qteletextmaker PROPERTIES
		WIN32_EXECUTABLE ON
	)
	target_sources(qteletextmaker PUBLIC ${CMAKE_CURRENT_LIST_DIR}/share/windows/appicon.rc)
endif()

if(UNIX)
	include(GNUInstallDirs)

	set(BIN_INSTALL_DIR "${CMAKE_INSTALL_BINDIR}")
	set(DOC_INSTALL_DIR "${CMAKE_INSTALL_DOCDIR}")
	set(EXAMPLES_INSTALL_DIR "${DOC_INSTALL_DIR}/examples")
else()
	set(BIN_INSTALL_DIR ".")
	set(DOC_INSTALL_DIR ".")
	set(EXAMPLES_INSTALL_DIR "./examples")
endif()

install(TARGETS qteletextmaker
	BUNDLE DESTINATION .
	RUNTIME DESTINATION ${BIN_INSTALL_DIR}
)

install(FILES
	${CMAKE_CURRENT_LIST_DIR}/README.md
	${CMAKE_CURRENT_LIST_DIR}/LICENSE
	DESTINATION ${DOC_INSTALL_DIR}
)

install(DIRECTORY
	${CMAKE_CURRENT_LIST_DIR}/examples/
	DESTINATION ${EXAMPLES_INSTALL_DIR}
)

if(UNIX AND NOT APPLE)
	install(FILES
		${CMAKE_CURRENT_LIST_DIR}/share/linux/qteletextmaker.desktop
		DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
	)
	install(DIRECTORY
		${CMAKE_CURRENT_LIST_DIR}/share/linux/icons
		DESTINATION ${CMAKE_INSTALL_DATADIR}
	)
endif()
