#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="diy2"
readonly ownership="Diy2 Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/diy2.git"
readonly tag="for/vtk-20250313-e5f2edf6f"
readonly paths="
.gitattributes
CMakeLists.vtk.txt
include
LEGAL.txt
LICENSE.txt
README.md
README.kitware.md
"

extract_source () {
    git_archive
    pushd "$extractdir/$name-reduced"
    mv -v include/diy include/vtkdiy2
    mv -v CMakeLists.vtk.txt CMakeLists.txt
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
