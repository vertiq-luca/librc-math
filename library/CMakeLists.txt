cmake_minimum_required(VERSION 3.5)


# Build lib from all source files
file(GLOB all_src_files src/*.c)
add_library(${LIBNAME} SHARED ${all_src_files})
target_link_libraries(${LIBNAME} LINK_PUBLIC m)
target_include_directories(${LIBNAME} PUBLIC include )

# make the include directory public for install
file(GLOB LIB_HEADERS include/*.h)
set_target_properties(${LIBNAME} PROPERTIES PUBLIC_HEADER "${LIB_HEADERS}")
set_target_properties(${LIBNAME} PROPERTIES SOVERSION 1)

# make sure everything is installed where we want
# LIB_INSTALL_DIR comes from the parent cmake file
install(
	TARGETS			${LIBNAME}
	LIBRARY			DESTINATION ${LIB_INSTALL_DIR}
	RUNTIME			DESTINATION /usr/bin
	PUBLIC_HEADER	DESTINATION /usr/include
)

# install the headers manually since they live in a folder in /usr/include
# and I can't get the PUBLIC_HEADER property to recognize this
file(GLOB MORE_HEADERS include/rc_math/*.h)
install( FILES ${MORE_HEADERS} DESTINATION /usr/include/rc_math/)

