cmake_minimum_required(VERSION 3.5)
project(rc_math)
set(LIBNAME rc_math)

# James' standard list of cmake flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -Wall -Wextra -Wuninitialized \
					-Wunused-variable -Wdouble-promotion -Wmissing-prototypes \
					-Wmissing-declarations -Werror=undef -Wno-unused-function ")

# extra optimization flags to enable NEON/vectorization
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -ffast-math -ftree-vectorize")


# for VOXL, install 64-bit libraries to lib64, 32-bit libs go in /usr/lib
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^aarch64")
	set(LIB_INSTALL_DIR /usr/lib64)
else()
	set(LIB_INSTALL_DIR /usr/lib)
endif()


# add targets from subdirectories
add_subdirectory(library)
add_subdirectory(examples)
