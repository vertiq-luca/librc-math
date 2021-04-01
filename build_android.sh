# !/bin/bash

set -e

# Setup the android NDK
export NDK=~/Android/Sdk/ndk-bundle/

# Where we will be installing the android build
BUILD_INSTALL_DIR=./android_build/

####################################################################################
## Parse the command line arguments
####################################################################################
POSITIONAL=()
while [[ $# -gt 0 ]]
do
key="$1"

case $key in
    -b|--build_dir)
    BUILD_DIR_INPUT="$2"
    shift # past argument
    shift # past value
    ;;
    -n|--ndk)
    NDK_INPUT="$2"
    shift # past argument
    shift # past value
    ;;
    -h|--help)
    HELP_INPUT="wasSet"
    shift # past argument
    ;;
    *)    # unknown option
    POSITIONAL+=("$1") # save it in an array for later
    shift # past argument
    ;;
esac
done
set -- "${POSITIONAL[@]}" # restore positional parameters



# Check if the help was selected and if it was then print it
if [[ -v HELP_INPUT ]]; 
then 
    echo "Build librc_math for Android."
    echo "This will also build all the needed dependencies."
    echo
    echo "Usage: ./build_android.sh -n <ndk_install_location> -b <output_build_dir>"
    echo "Options:"
    echo "      -n,--ndk        : The location that the NDK is installed."
    echo "                        Default is \"~/Android/Sdk/ndk-bundle/\""
    echo "                        The argument is optional"
    echo "      -b,--build_dir  : The location to put the build output"
    echo "                        Default is \"./android_build/\""
    echo "                        The argument is optional"
    echo "      -h,--help       : Display this usage help"
    exit
fi


# Check if the build dir is set and take appropriate action
if [ -z ${BUILD_DIR_INPUT+x} ]; 
then 
	# Make it absolute
	CWD=$(pwd)
	BUILD_INSTALL_DIR=${CWD}/${BUILD_INSTALL_DIR}
	echo "Build dir is not set. Using ${BUILD_INSTALL_DIR}"
else 
	echo "Build directory was manually set to ${BUILD_DIR_INPUT}"
	BUILD_INSTALL_DIR=${BUILD_DIR_INPUT}
fi

# Check if the NDK is set and take appropriate action
if [ -z ${NDK_INPUT+x} ]; 
then 
	echo "NDK is not set. Using ${NDK}"
else 
	echo "NDK was manually set to ${NDK_INPUT}"
	export NDK=${NDK_INPUT}
fi

####################################################################################
## Set the output dirs
####################################################################################

# Where the output files will go
LIB_DIR=${BUILD_INSTALL_DIR}/lib/
INCLUDE_DIR=${BUILD_INSTALL_DIR}/include/

# Make all the needed output directories
mkdir -p ${LIB_DIR}
mkdir -p ${INCLUDE_DIR}


####################################################################################
## Build the Library
####################################################################################


# The directory where we build android from
ANDROID_BUILD_DIR=./android/

# The location of the lib and include 
LIB_BUILD_FILE=${ANDROID_BUILD_DIR}/libs/arm64-v8a/librc_math.so
INCLUDE_BUILD_DIR=${ANDROID_BUILD_DIR}../library/include/

# Clean the Android build artifacts
rm -rf ${ANDROID_BUILD_DIR}/obj
rm -rf ${ANDROID_BUILD_DIR}/libs

# Make the android libs
CWD=$(pwd)
cd ${ANDROID_BUILD_DIR}/jni
$NDK/ndk-build -e ALL_BUILD_DIR=${BUILD_INSTALL_DIR} -e APP_ABI=arm64-v8a
cd ${CWD}


####################################################################################
## Install the library
####################################################################################

# Move the files to the Install directory 
cp ${LIB_BUILD_FILE} ${LIB_DIR}
cp -rp ${INCLUDE_BUILD_DIR}/* ${INCLUDE_DIR}



