#!/bin/bash

setup_arch() {
    local arch=$1
    case "$arch" in
    arm)
        export ARCH=$arch
        export CROSS_COMPILE=arm-linux-gnueabihf-
        ;;
    riscv)
        export ARCH=$arch
        export CROSS_COMPILE=riscv64-linux-gnu-
        ;;
    *)
        echo Invalid arch
        return 1
        ;;
    esac
}

while getopts ":a:" option; do
    case "$option" in
    a)
        arch=$OPTARG
        setup_arch $arch
        ;;
    *)
        echo "Usage: envsetup.sh -a ARCH"
        ;;
    esac
done
