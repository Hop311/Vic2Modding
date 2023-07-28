cmake_minimum_required (VERSION 3.20)

project(Vic2Modding)

include_directories("source" "source/database" "lodepng")
set(SRC "source/winmain.c" "source/win32_tools.c" "source/render.c" "source/maths.c" "source/memory_opt.c" "source/string_wrapper.c" "source/file.c"
		   "source/parser.c" "source/lexer.c" "source/database/database_types.c" "source/database/database_lists.c" "source/database/database_parsing.c" "source/database/database_parsing_common.c"
		   "source/database/database_parsing_map.c" "source/database/database_parsing_units.c" "source/database/database_parsing_history.c" "lodepng/lodepng.c")
#set(SOURCE "source/pixel_draw.c")

# Executables
add_executable(Vic2Modding WIN32 ${SRC})
set_target_properties(Vic2Modding PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED True)
if(MSVC)
	#target_compile_options(Vic2Modding PRIVATE "/W4;/WX;$<$<CONFIG:RELEASE>:/O2>")
	target_link_options(Vic2Modding PRIVATE "/SUBSYSTEM:WINDOWS" "/ENTRY:WinMainCRTStartup")
else()
	#target_compile_options(Vic2Modding PRIVATE "-Wall;-Wextra;-Werror;$<$<CONFIG:RELEASE>:-O3>")
	target_link_options(Vic2Modding PRIVATE "-mwindows")
endif()

