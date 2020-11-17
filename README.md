# librc_math

This is a small collection of basic math and control system routines focused on robotics applications. It is meant to be easier to use, tweak, and modify than Eigen due to its simplicity. Howevever, it doesn't claim to be as vast or optimized as Eigen.

This is a fork of github.com/strawsondesign/librobotcontrol which is stripped down to only the math functions.

This branch is intended for use on ModalAI VOXL platforms, but it can be built and run on any platform (x86, BeagleBone, RPi, etc).

## Build Instructions

1) prerequisite: your own GCC of choice or the voxl-cross docker image

Follow the instructions here to build and install the voxl-cross docker image:

https://gitlab.com/voxl-public/voxl-docker


2) Launch the voxl-cross docker and make sure this project directory is mounted inside the Docker.

```bash
~/git/libmodal_json$ voxl-docker -i voxl-cross
root@silverstone:/home/user#
root@silverstone:/home/user# ls
CHANGELOG       LICENSE    build.sh  conf     install_on_voxl.sh  library          readers
CMakeLists.txt  README.md  clean.sh  example  ipk                 make_package.sh
root@silverstone:/home/user#
```

3) The build script takes one of these 4 options to set the architecture this should build for:

* `./build.sh native` Builds using whatever native gcc lives at /usr/bin/gcc
* `./build.sh 32`     Builds using the 32-bit (armv7) arm-linux-gnueabi cross-compiler that's in the voxl-cross docker
* `./build.sh 64`     Builds using the 64-bit (armv8) aarch64-gnu cross-compiler that's in the voxl-cross docker
* `./build.sh both`   Builds both 32 and 64 cross-compiled binaries. This is what's used to build the VOXL IPK packages

```bash
bash-4.3$ ./build.sh both
```

4) Make an ipk package while still inside the docker.

```bash
~/git/libmodal_json$ ./make_ipk.sh

Package Name:  librc_math
version Number:  x.x.x
ar: creating librc_math_x.x.x.ipk

DONE
```

This will make a new libmodal_json_x.x.x.ipk file in your working directory. The name and version number came from the ipk/control/control file. If you are updating the package version, edit it there.

It will install whatever was built in the previous step based on your chosen compiler. For the "both" option it will install 32 and 64 bit versions of the lib, one copy of the headers, and just the 64-bit example programs.


## Deploy to VOXL

You can now push the ipk package to the VOXL and install with opkg however you like. To do this over ADB, you may use the included helper script: install_on_voxl.sh. Do this outside of docker as your docker image probably doesn't have usb permissions for ADB.

```bash
$ ./install_on_voxl.sh
searching for ADB device
adb device found
Setting up directory structure
pushing librc_math_1.0.0.ipk to target
librc_math_1.0.0.ipk: 1 file pushed. 2.1 MB/s (441536 bytes in 0.201s)
Configuring librc_math.
DONE
```
