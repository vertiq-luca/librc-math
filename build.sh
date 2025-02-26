#!/bin/bash

## voxl-cross contains the following toolchains
## first two for apq8096, last for qrb5165
TOOLCHAIN_APQ8096_32="/opt/cross_toolchain/arm-gnueabi-4.9.toolchain.cmake"
TOOLCHAIN_APQ8096_64="/opt/cross_toolchain/aarch64-gnu-4.9.toolchain.cmake"
TOOLCHAIN_QRB5165="/opt/cross_toolchain/aarch64-gnu-7.toolchain.cmake"

# placeholder in case more cmake opts need to be added later
EXTRA_OPTS=""

## this list is just for tab-completion
AVAILABLE_PLATFORMS="qrb5165 apq8096 native"


print_usage(){
	echo ""
	echo " Build the current project based on platform target."
	echo ""
	echo " Usage:"
	echo ""
	echo "  ./build.sh apq8096"
	echo "        Build 64-bit binaries for apq8096"
	echo ""
	echo "  ./build.sh qrb5165"
	echo "        Build 64-bit binaries for qrb5165"
	echo ""
	echo "  ./build.sh native"
	echo "        Build with the native gcc/g++ compilers."
	echo ""
	echo ""
}


case "$1" in
	apq8096)
		## multi-arch library, build both 32 and 64 for apq8096
		mkdir -p build32
		cd build32
		cmake -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_APQ8096_32} ${EXTRA_OPTS} ../
		make -j$(nproc)
		cd ../
		mkdir -p build64
		cd build64
		cmake -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_APQ8096_64} ${EXTRA_OPTS} ../
		make -j$(nproc)
		cd ../
		;;
	qrb5165)
		## multi-arch library, build both 32 and 64
		mkdir -p build32
		cd build32
		cmake -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_APQ8096_32} ${EXTRA_OPTS} ../
		make -j$(nproc)
		cd ../
		mkdir -p build64
		cd build64
		cmake -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_QRB5165} ${EXTRA_OPTS} ../
		make -j$(nproc)
		cd ../
		;;

	native)
		mkdir -p build
		cd build
		cmake ${EXTRA_OPTS} ../
		make -j$(nproc)
		cd ../
		;;

	*)
		print_usage
		exit 1
		;;
esac



