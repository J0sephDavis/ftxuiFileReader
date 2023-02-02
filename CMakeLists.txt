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

project(ftxuiFileViewer
  LANGUAGES CXX
  VERSION 1.0.0
)
add_executable(ftxuiFileViewer 
	src/main.cc
)

target_include_directories(ftxuiFileViewer PRIVATE src)
target_link_libraries(ftxuiFileViewer
	PRIVATE ftxui::screen
	PRIVATE ftxui::dom
	PRIVATE ftxui::component
)
set_target_properties(ftxuiFileViewer PROPERTIES CXX_STANDARD 20)
