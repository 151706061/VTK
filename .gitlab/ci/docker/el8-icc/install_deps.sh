#!/bin/sh

set -e

rpm --import https://repo.almalinux.org/almalinux/RPM-GPG-KEY-AlmaLinux

# Install EPEL
dnf install -y --setopt=install_weak_deps=False \
    epel-release

# Install extra dependencies for ParaView
dnf install -y --setopt=install_weak_deps=False \
    bzip2 patch git-core git-lfs

# Mesa dependencies
dnf install -y --setopt=install_weak_deps=False \
    mesa-libOSMesa-devel mesa-libOSMesa mesa-dri-drivers

# External dependencies
dnf install -y --setopt=install_weak_deps=False \
    libXcursor-devel utf8cpp-devel pugixml-devel libtiff-devel \
    eigen3-devel double-conversion-devel lz4-devel expat-devel glew-devel \
    hdf5-devel hdf5-mpich-devel hdf5-openmpi-devel hdf5-devel netcdf-devel \
    netcdf-mpich-devel netcdf-openmpi-devel libogg-devel libtheora-devel \
    jsoncpp-devel gl2ps-devel protobuf-devel boost-devel gdal-devel PDAL-devel \
    cgnslib-devel

# Python dependencies
dnf install -y --setopt=install_weak_deps=False \
    python3.11 python3.11-devel python3.11-numpy \
    python3.11-pip

python3.11 -m venv /opt/python311/venv
/opt/python311/venv/bin/pip install 'matplotlib<=3.6.3'
# wslink will bring aiohttp>=3.7.4
/opt/python311/venv/bin/pip install 'wslink>=1.0.4'
/opt/python311/venv/bin/pip install xarray cftime netcdf4

# Upgrade libarchive (for CMake)
dnf upgrade -y --setopt=install_weak_deps=False \
    libarchive

dnf clean all
