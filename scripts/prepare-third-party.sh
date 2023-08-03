#!/bin/bash

set -e

# Setup the versions of linux kernel and busybox to use
kernel_version=5.4.251
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

    local config_target="noneconfig"
    case "$ARCH" in
    arm)
        config_target=vexpress_defconfig
        ;;
    riscv)
        config_target=defconfig
        ;;
    esac

    ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} make O=build-${ARCH} ${config_target}
    echo "CONFIG_DEBUG_INFO=y" >>build-${ARCH}/.config
    echo "CONFIG_GDB_SCRIPTS=y" >>build-${ARCH}/.config
    ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} make O=build-${ARCH} -j$(nproc)
    popd
}

build_busybox() {
    pushd ${third_party_dir}/busybox-${busybox_version}
    mkdir -p build-${ARCH}
    ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} make O=build-${ARCH} defconfig
    echo "CONFIG_STATIC=y" >>build-${ARCH}/.config
    popd
}

build_rootfs() {
    pushd ${third_party_dir}/busybox-${busybox_version}
    ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} make O=build-${ARCH} install
    dd if=/dev/zero of=rootfs.ext4 bs=1M count=64
    mkfs.ext4 rootfs.ext4
    mkdir -p mnt
    guestmount -a rootfs.ext4 -m /dev/sda mnt

    cp -r build-${ARCH}/_install/* mnt
    mkdir -p mnt/mnt
    mkdir -p mnt/dev
    mkdir -p mnt/sys
    mkdir -p mnt/proc

    touch mnt/bin/init.sh
    chmod +x mnt/bin/init.sh

    cat <<EOF >mnt/bin/init.sh
#!/bin/sh
mount -t devtmpfs none /dev
mount -t sysfs none /sys
mount -t proc none /proc
mount -t 9p lddt /mnt

/bin/sh
EOF
    sync
    guestunmount mnt
    mv rootfs.ext4 ..
    popd
}

download_and_extract_tarballs
build_kernel
build_busybox
build_rootfs
