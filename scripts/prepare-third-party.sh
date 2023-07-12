#!/bin/bash

set -e

# Setup the versions of linux kernel and busybox to use
kernel_version=5.4.249
busybox_version=1.36.1

kernel_url=https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-${kernel_version}.tar.xz
busybox_url=https://busybox.net/downloads/busybox-${busybox_version}.tar.bz2

# Setup the path
project_dir=$(realpath $(dirname ${BASH_SOURCE[0]})/..)
third_party_dir=$project_dir/third-party

download_and_extract_tarballs() {
    mkdir -p $third_party_dir/.tmp

    pushd $third_party_dir/.tmp

    wget $kernel_url
    wget $busybox_url

    local kenel_tarball=$(find . -name linux*.tar.xz)
    local busybox_tarball=$(find . -name busybox*.tar.bz2)

    tar -xf $kernel_tarball -C ..
    tar -xf $busybox_tarball -C ..

    popd
}

build_kernel() {
    echo pass
}

build_busybox() {
    echo pass
}