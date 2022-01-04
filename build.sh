#!/bin/bash

TOOLCHAIN32="/opt/cross_toolchain/arm-gnueabi-4.9.toolchain.cmake"
TOOLCHAIN64="/opt/cross_toolchain/aarch64-gnu-4.9.toolchain.cmake"

# placeholder in case more needs to be added later
EXTRA_OPTS=""
MODE="both" ## default mode when no argument is given

print_usage(){
	echo "                                                                                "
	echo " Build the current project in one of 4 modes based on build environment."
	echo " The default behavior is to build BOTH 32 and 64-bit binaries."
	echo ""
	echo " Usage:"
	echo "  ./build.sh"
	echo "        Build for the default package architecture."
	echo ""
	echo "  ./build.sh both"
	echo "        Build both 32 and 64-bit binaries."
	echo ""
	echo "  ./build.sh 32"
	echo "        Build with an arm-gnueabi 32-bit cross compiler."
	echo ""
	echo "  ./build.sh 64"
	echo "        Build with an aarch64 64-bit cross compiler."
	echo ""
	echo "  ./build.sh native"
	echo "        Build with the native gcc/g++ compilers."
	echo ""
	echo ""
}

if [ $# -eq 0 ]; then
	echo "using default mode: $MODE"
elif ! [ $# -eq 1 ]; then
	print_usage
	exit 1
else
	MODE="$1"
fi

case "$MODE" in
	native)
		mkdir -p build
		cd build
		cmake ../ ${EXTRA_OPTS}
		make -j$(nproc)
		cd ../
		;;

	32)
		mkdir -p build32
		cd build32
		cmake -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN32} ${EXTRA_OPTS} ../
		make -j$(nproc)
		cd ../
		;;

	64)
		mkdir -p build64
		cd build64
		cmake -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN64} ${EXTRA_OPTS} ../
		make -j$(nproc)
		cd ../
		;;

	both|BOTH)
		mkdir -p build32
		cd build32
		cmake -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN32} ${EXTRA_OPTS} ../
		make -j$(nproc)
		cd ../
		mkdir -p build64
		cd build64
		cmake -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN64} ${EXTRA_OPTS} ../
		make -j$(nproc)
		cd ../
		;;
	*)
		print_usage
		exit 1
		;;
esac




