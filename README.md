#librc_math

This is a small collection of basic math and control system routines focused on robotics applications. It is meant to be easier to use, tweak, and modify than Eigen due to its simplicity. Howevever, it doesn't claim to be as vast or optimized as Eigen.

This is a fork of github.com/strawsondesign/librobotcontrol which is stripped down to only the math functions.

This branch is intended for use on ModalAI VOXL platforms, but it can be built and run on any platform (x86, BeagleBone, RPi, etc).

## Build Instructions for VOXL

1) Prerequisite: voxl-emulator docker image. Follow the instructions here:

https://gitlab.com/voxl-public/voxl-docker

2) Launch the voxl-emulator docker image and make sure this project directory is mounted inside the Docker.

```bash
~/git/librc_math$ voxl-docker -i voxl-emulator

bash-4.3$ ls
CHANGELOG  Makefile   build.sh  examples          ipk      make_ipk.sh
LICENSE    README.md  clean.sh  install_on_target.sh  library
```

5) Compile inside the docker

```bash
bash-4.3$ ./build.sh
```

6) Make an ipk package either inside or outside of the docker.

```bash
bash-4.3$ ./make_package.sh

Package Name:  librc_math
version Number:  1.0.0
Library Install Complete
Examples Install complete
/usr/bin/ar: creating librc_math_1.0.0.ipk

DONE
```

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
