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

    echo "Start downloading tarballs..."
    if [ ! -e linux-${kernel_version}.tar.xz ]; then
        wget $kernel_url
    fi
    if [ ! -e busybox-${busybox_version}.tar.bz2 ]; then
        wget $busybox_url
    fi
    echo "End downloading tarballs"

    local kernel_tarball=$(find . -name linux*.tar.xz)
    local busybox_tarball=$(find . -name busybox*.tar.bz2)

    echo "Start extracting tarballs..."
    if [ ! -e ../linux-${kernel_version} ]; then
        tar -xf $kernel_tarball -C ..
    fi

    if [ ! -e ../busybox-${busybox_version} ]; then
        tar -xf $busybox_tarball -C ..
    fi
    echo "End extracting tarballs"

    popd
}

build_kernel() {
    pushd ${third_party_dir}/linux-${kernel_version}
    ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} make O=build defconfig
    cp build/.config build/.config.old
    echo "CONFIG_DEBUG_INFO=y" >>build/.config
    echo "CONFIG_GDB_SCRIPTS=y" >>build/.config
    ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} make O=build -j$(nproc)
    popd
}

build_busybox() {
    pushd ${third_party_dir}/busybox-${busybox_version}
    ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} make defconfig
    cp .config .config.old
    echo "CONFIG_STATIC=y" >>.config
    popd
}

build_rootfs() {
    pushd ${third_party_dir}/busybox-${busybox_version}
    ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} make install
    dd if=/dev/zero of=rootfs.ext4 bs=4M count=32
    mkfs.ext4 rootfs.ext4
    mkdir -p mnt
    guestmount -a rootfs.ext4 -m /dev/sda mnt

    cp -r _install/* mnt
    mkdir -p mnt/mnt
    mkdir -p /dev
    mkdir -p /sys
    mkdir -p /proc

    touch mnt/bin/init.sh
    chmod +x mnt/bin/init.sh

    cat <<EOF >mnt/bin/init.sh
#!/bin/sh
mount -t sysfs none /sys
mount -t proc none /proc
mount -t 9p lddtsrc /mnt

/bin/sh
EOF
    guestunmount mnt
    mv rootfs.ext4 ..
    popd
}

download_and_extract_tarballs
build_kernel
build_busybox
build_rootfs
