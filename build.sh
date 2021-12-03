#!/bin/bash
################################################################################
# Copyright 2021 ModalAI Inc.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# 4. The Software is used solely in conjunction with devices provided by
#    ModalAI Inc.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
################################################################################

# exit with error immediately on any failure
set -e

TOOLCHAIN32="/opt/cross_toolchain/arm-gnueabi-4.9.toolchain.cmake"
TOOLCHAIN64="/opt/cross_toolchain/aarch64-gnu-4.9.toolchain.cmake"

print_usage(){
	echo "                                                                                "
	echo " Build the current project in one of 4 modes based on build environment."
	echo " "
	echo ""
	echo " Usage:"
	echo "  ./build.sh native"
	echo "        Build with the native gcc/g++ compilers."
	echo ""
	echo "  ./build.sh 32"
	echo "        Build with an arm-gnueabi 32-bit cross compiler."
	echo ""
	echo "  ./build.sh 64"
	echo "        Build with an aarch64 64-bit cross compiler."
	echo ""
	echo "  ./build.sh both"
	echo "        Build both 32 and 64-bit binaries."
	echo ""
}


if ! [ $# -eq 1 ]; then
	print_usage
	exit 1
fi

case "$1" in
	native)
		mkdir -p build
		cd build
		cmake ../
		make -j$(nproc)
		cd ../
		;;

	32)
		mkdir -p build32
		cd build32
		cmake -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN32} ../
		make -j$(nproc)
		cd ../
		;;

	64)
		mkdir -p build64
		cd build64
		cmake -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN64} ../
		make -j$(nproc)
		cd ../
		;;

	both)
		mkdir -p build32
		cd build32
		cmake -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN32} ../
		make -j$(nproc)
		cd ../
		mkdir -p build64
		cd build64
		cmake -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN64} ../
		make -j$(nproc)
		cd ../
		;;
	*)
		print_usage
		exit 1
		;;
esac

echo ""
echo "DONE BUILDING"


