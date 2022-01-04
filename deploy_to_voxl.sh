#!/bin/bash
################################################################################
# Copyright (c) 2022 ModalAI, Inc. All rights reserved.
#
# Installs the ipk or deb package on target.
# Requires the package to be built and an adb connection.
################################################################################
set -e

PACKAGE=$(cat pkg/control/control | grep "Package" | cut -d' ' -f 2)

# count ipk files in current directory
NUM_IPK=$(ls -1q $PACKAGE*.ipk | wc -l)
NUM_DEB=$(ls -1q $PACKAGE*.deb | wc -l)

if [[ $NUM_IPK -eq "0" && $NUM_DEB -eq "0" ]]; then
	echo "ERROR: no ipk or deb file found"
	echo "run build.sh and make_package.sh first"
	exit 1
elif [[ $NUM_IPK -gt "1" || $NUM_DEB -gt "1" ]]; then
	echo "ERROR: more than 1 ipk or deb file found"
	echo "make sure there is at most one of each in the current directory"
	exit 1
fi

# now we know only one ipk file exists
FILE=$(ls -1q $PACKAGE*.ipk)

if [ "$1" == "ssh" ]; then
	if [ -f /usr/bin/sshpass ];then
		if [ -z ${VOXL_IP+x} ]; then
			echo "Did not find a VOXL_IP env variable,"
			echo ""
			echo "If you would like to push over ssh automatically,"
			echo "please export VOXL_IP in your bashrc"
			echo ""
			read -p "Please enter an IP to push to:" SEND_IP

		else
			SEND_IP="${VOXL_IP}"
		fi

		echo "Pushing File to $SEND_IP"
		sshpass -p "oelinux123" scp -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null ./$FILE root@$SEND_IP:/home/root/ipk/$FILE 2>/dev/null > /dev/null \
		&& echo "File pushed, Installing" \
		&& sshpass -p "oelinux123" ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null root@$SEND_IP "opkg install --force-reinstall --force-downgrade --force-depends /home/root/ipk/$FILE" 2>/dev/null
	else
		echo ""
		echo "You do not have sshpass installed"
		echo "Please install sshpass to use the install via ssh feature"
		echo ""
	fi

else
	if [ ! -f /usr/bin/adb ];then
		echo ""
		echo "You do not have adb installed"
		echo "Please install adb to use the install via adb feature"
		echo ""
		exit 1
	fi

	echo "searching for ADB device"
	adb wait-for-device
	echo "checking VOXL for dpkg/opkg"

	RESULT=`adb shell 'ls /usr/bin/dpkg 2> /dev/null | grep pkg -q; echo $?'`
	RESULT="${RESULT//[$'\t\r\n ']}" ## remove the newline from adb
	if [ "$RESULT" == "0" ] ; then
		echo "dpkg detected";
		MODE="dpkg"
	else
		RESULT=`adb shell 'ls /usr/bin/opkg 2> /dev/null | grep pkg -q; echo $?'`
		RESULT="${RESULT//[$'\t\r\n ']}" ## remove the newline from adb
		if [[ ${RESULT} == "0" ]] ; then
			echo "opkg detected";
			MODE="opkg"
		else
			echo "ERROR neither dpkg nor opkg found on VOXL"
			exit 1
		fi
	fi

	if [ $MODE == dpkg ]; then
		FILE=$(ls -1q $PACKAGE*.deb)
		adb push $FILE /data/$FILE
		adb shell "dpkg -i --force-downgrade --force-depends /data/$FILE"
	else
		FILE=$(ls -1q $PACKAGE*.ipk)
		adb push $FILE /data/$FILE
		adb shell "opkg install --force-reinstall --force-downgrade --force-depends /data/$FILE"
	fi

	echo "DONE"
fi