cmake_minimum_required (VERSION 3.11)
 
# --- Fetch FTXUI --------------------------------------------------------------
include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)
set(FETCHCONTENT_QUIET FALSE)

FetchContent_Declare(ftxui
	GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
	GIT_TAG b2853c8f147bfa032688773ea05027f80b317a57 
	GIT_PROGRESS TRUE
	GIT_SHALLOW FALSE 
)
 
FetchContent_GetProperties(ftxui)
if(NOT ftxui_POPULATED)
	FetchContent_Populate(ftxui)
	add_subdirectory(
		${ftxui_SOURCE_DIR}
		${ftxui_BINARY_DIR}
		EXCLUDE_FROM_ALL
	)
endif()
 
# --- Build --------------------------------------------------------------------

project(P14_fileViewer
  LANGUAGES CXX
  VERSION 1.0.0
)
add_executable(PROJ14 
	src/main.cc
)

target_include_directories(PROJ14 PRIVATE src)
target_link_libraries(PROJ14
	PRIVATE ftxui::screen
	PRIVATE ftxui::dom
	PRIVATE ftxui::component
)
set_target_properties(PROJ14 PROPERTIES CXX_STANDARD 20)
