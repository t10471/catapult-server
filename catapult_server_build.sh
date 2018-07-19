#!/bin/bash
##########################################################################
# This is the catapult-server automated install script for Linux and Mac OS.
# This file was downloaded from https://github.com/nemtech/catapult-server

# Copyright (c) 2016-present,
# Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
#
# This file is part of Catapult.
#
# Catapult is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Catapult is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with Catapult. If not, see <http://www.gnu.org/licenses/>.
#
# https://github.com/nemtech/catapult-server/blob/master/LICENSE.txt
##########################################################################

set -e

unset TMPFILE

atexit() {
  [[ -n ${tmpfile-} ]] && rm -f "$tmpfile"
}

trap atexit EXIT
trap 'rc=$?; trap - EXIT; atexit; exit $?' INT PIPE TERM



CWD="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
if [ "${CWD}" != "${PWD}" ]; then
  printf "\\n\\tPlease cd into directory %s to run this script.\\n \\tExiting now.\\n\\n" "${CWD}"
  exit 1
fi
ARCH=$( uname )
: ${BUILD_DIR:=${PWD}/_build}
: ${DEPENDENCIES_DIR:=${PWD}/_build_dependencies}
TMP_DIR=$(mktemp -d "/tmp/${0##*/}.tmp.XXXXXX")

printf "\\n\\ttcatapult-server buid start\\n\\n" $BUILD_DIR
printf "\\tBUILD_DIR = %s \\n" $BUILD_DIR
printf "\\tDEPENDENCIES_DIR = %s \\n" $DEPENDENCIES_DIR
printf "\\tTMP_DIR = %s \\n\\n" $TMP_DIR

function darwin_install_dependencies () {
  mkdir -p $DEPENDENCIES_DIR
  darwin_install_googletest
  darwin_install_boost
  darwin_install_mongodb
  darwin_install_mongo_c_driver
  darwin_install_mongo_cxx_driver
  darwin_install_libzmq
  darwin_install_cppzmq
  mkdir $BUILD_DIR
  cd $BUILD_DIR
  cmake -DCMAKE_BUILD_TYPE=RelWithDebugInfo \
    -DMONGOC_LIB=${MONGO_C_DRIVER_DIR}/lib/libmongoc-1.0.dylib \
    -DBSONC_LIB=${LIBBSON_DIR}/lib/libbson-1.0.dylib \
    -DMONGOCXX_LIB=${MONGO_CXX_DRIVER_INSTALL_DIR}/lib/libmongocxx.dylib \
    -DBSONCXX_LIB=${MONGO_CXX_DRIVER_INSTALL_DIR}/lib/libbsoncxx.dylib \
    -DCMAKE_C_FLAGS="-lpthread" \
    -DCMAKE_MODULE_LINKER_FLAGS="-lpthread" \
    -DCMAKE_SHARED_LINKER_FLAGS="-lpthread" ..
  make publish
  make
}


function darwin_install_googletest () {
  GOOGLETEST_VERSION=1.8.0
  GOOGLETEST_INSTALL_DIR=${DEPENDENCIES_DIR}/google-test/${GOOGLETEST_VERSION}
  if [ -z "$GTEST_ROOT" ]; then
    printf "\\tInstall GoogleTest %s.\\n" GOOGLETEST_VERSION

    mkdir -p ${GOOGLETEST_INSTALL_DIR}
    cd $TMP_DIR
    git clone git@github.com:google/googletest.git
    cd googletest
    git checkout -b release-1.8.0
    mkdir cmake-build
    cd cmake-build
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=${GOOGLETEST_INSTALL_DIR} ..
    make
    make install
    export GTEST_ROOT=${GOOGLETEST_INSTALL_DIR}

    printf "\\tSuccessfully installed GoogleTest %s.\\n" GOOGLETEST_VERSION
  else
    printf "\\tGoogleTest already installed.\\n"
  fi
}

function darwin_install_boost () {
  BOOST_VERSION=1.65.1
  BOOST_INSTALL_DIR=${DEPENDENCIES_DIR}/boost/${BOOST_VERSION}
  if [ -z "$BOOST_ROOT" ] && [ -z "$Boost_INCLUDE_DIR" ] && [ -z "$Boost_INCLUDE_DIR" ]; then
    printf "\\tInstall Boost %s.\\n" BOOST_VERSION

    mkdir -p ${BOOST_INSTALL_DIR}
    cd $TMP_DIR
    wget https://dl.bintray.com/boostorg/release/1.65.1/source/boost_1_65_1.tar.gz
    tar xfz boost_1_65_1.tar.gz
    cd boost_1_65_1
    ./b2 install -j4 -–prefix=${BOOST_INSTALL_DIR} --exec-prefix==${BOOST_INSTALL_DIR} --libdir=${BOOST_INSTALL_DIR}/lib --includedir=${BOOST_INSTALL_DIR}/include
    export BOOST_ROOT=$BOOST_INSTALL_DIR
    export Boost_INCLUDE_DIR=${BOOST_INSTALL_DIR}/include
    export BOOST_INCLUDE_DIR=${BOOST_INSTALL_DIR}/include

    printf "\\tSuccessfully installed Boost %s.\\n" BOOST_VERSION
  else
    printf "\\tBoost already installed.\\n"
  fi
}

function darwin_install_mongodb () {
  if [ $( brew list | grep mongodb ) ]; then
    printf "\\tMongodb already installed.\\n"
    retrun
  fi
  if [ ! $(brew install mongodb) ]; then
    printf "\\tHomebrew returned an error installing mongodb.\\n"
    printf "\\tExiting now.\\n\\n"
    exit 1;
  fi
}

function darwin_install_mongo_c_driver () {
  MONGO_C_DRIVER_VERSION=1.4.2
  LIBBSON_INSTALL_DIR=${DEPENDENCIES_DIR}/libbson/${MONGO_C_DRIVER_VERSION}
  MONGO_C_DRIVER_INSTALL_DIR=${DEPENDENCIES_DIR}/mongo-c-driver/${MONGO_C_DRIVER_VERSION}
  if [ -z "$LIBBSON_DIR" ] && [ -z "$MONGO_C_DRIVER_DIR" ]; then
    printf "\\tInstall mongo-c-driver %s.\\n" LIBBSON_VERSION

    mkdir -p ${LIBBSON_INSTALL_DIR}
    cd $TMP_DIR
    wget https://github.com/mongodb/mongo-c-driver/releases/download/1.4.2/mongo-c-driver-1.4.2.tar.gz
    cd mongo-c-driver-1.4.2
    cd src/libbson
    mkdir cmake-build
    cd cmake-build
    cmake -DCMAKE_INSTALL_PREFIX=${LIBBSON_INSTALL_DIR} ..
    make
    make install
    cd ../../../

    mkdir -p ${MONGO_C_DRIVER_INSTALL_DIR}
    mkdir cmake-build
    cd cmake-build
    cmake -DBSON_ROOT_DIR=${LIBBSON_INSTALL_DIR} -DCMAKE_INSTALL_PREFIX=${MONGO_C_DRIVER_INSTALL_DIR} -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF ..
    make
    make install

    export LIBBSON_DIR=$LIBBSON_INSTALL_DIR
    export MONGO_C_DRIVER_DIR=$MONGO_C_DRIVER_INSTALL_DIR

    printf "\\tSuccessfully installed mongo-c-driver %s.\\n" LIBBSON_VERSION
  else
    printf "\\tmongo-c-driver already installed.\\n"
  fi
}

function darwin_install_mongo_cxx_driver () {
  MONGO_CXX_DRIVER_VERSION=3.0.2
  MONGO_CXX_DRIVER_INSTALL_DIR=${DEPENDENCIES_DIR}/mongo-cxx-driver/${MONGO_CXX_DRIVER_VERSION}
  if [ -z "$LIBMONGOCXX_DIR" ] && [ -z "$libbsoncxx_DIR" ] && [ -z "$LIBBSONCXX_DIR" ]; then
    printf "\\tInstall mongo-cxx-driver %s.\\n" MONGO_CXX_DRIVER_VERSION
    cd $TMP_DIR
    wget https://github.com/mongodb/mongo-cxx-driver/archive/r3.0.2.tar.gz
    cd r3.0.2
    mkdir cmake-build
    cd cmake-build

    cmake -DCMAKE_BUILD_TYPE=Release \
        -DBSONCXX_POLY_USE_BOOST=1 \
        -DCMAKE_INSTALL_PREFIX=${MONGO_CXX_DRIVER_INSTALL_DIR} \
        -DCMAKE_PREFIX_PATH=${MONGO_CXX_DRIVER_INSTALL_DIR} \
        -DLIBBSON_DIR=${LIBBSON_DIR} \
        -DLIBMONGOC_DIR=${MONGO_C_DRIVER_DIR} ..
    make
    make install

    export LIBMONGOCXX_DIR=${MONGO_CXX_DRIVER_INSTALL_DIR}/lib/cmake/libmongocxx-3.0.2/
    export libbsoncxx_DIR=${MONGO_CXX_DRIVER_INSTALL_DIR}/lib/cmake/libbsoncxx-3.0.2/
    export LIBBSONCXX_DIR=${MONGO_CXX_DRIVER_INSTALL_DIR}/lib/cmake/libbsoncxx-3.0.2/

    printf "\\tSuccessfully installed mongo-cxx-driver %s.\\n" MONGO_CXX_DRIVER_VERSION
  else
    printf "\\tmongo-cxx-driver already installed.\\n"
  fi
}
function darwin_install_libzmq () {
  ZEROMQ_VERSION=4.2.5
  ZEROMQ_INSTALL_DIR=${DEPENDENCIES_DIR}/libzmq/${ZEROMQ_VERSION}
  if [ -z "$ZeroMQ_DIR" ]; then
    printf "\\tInstall libzmq %s.\\n" ZEROMQ_VERSION
    cd $TMP_DIR
    git clone git@github.com:zeromq/libzmq.git
    cd libzmq
    git checkout -b v4.2.5
    mkdir cmake-build
    cd cmake-build
    cmake -DWITH_PERF_TOOL=OFF -DZMQ_BUILD_TESTS=OFF -DENABLE_CPACK=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=${ZEROMQ_INSTALL_DIR} ..
    export ZeroMQ_DIR=${ZEROMQ_INSTALL_DIR}/share/cmake/ZeroMQ/

    printf "\\tSuccessfully installed libzmq %s.\\n" MONGO_CXX_DRIVER_VERSION
  else
    printf "\\tlibzmq already installed.\\n"
  fi
}
function darwin_install_cppzmq () {
  CPPZMQ_VERSION=4.2.5
  CPPZMQ_INSTALL_DIR=${DEPENDENCIES_DIR}/cppzmq/${CPPZMQ_VERSION}
  if [ -z "$cppzmq_DIR" ]; then
    printf "\\tInstall cppzmq %s.\\n" CPPZMQ_VERSION
    cd $TMP_DIR
    git clone git@github.com:zeromq/cppzmq.git
    cd cppzmq
    git checkout -b v4.2.3
    mkdir cmake-build
    cd cmake-build
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=${CPPZMQ_INSTALL_DIR} ..
    export cppzmq_DIR=${CPPZMQ_INSTALL_DIR}/share/cmake/cppzmq/

    printf "\\tSuccessfully installed cppzmq %s.\\n" MONGO_CXX_DRIVER_VERSION
  else
    printf "\\tcppzmq already installed.\\n"
  fi
}

function main () {
  if [ "$ARCH" == "Linux" ]; then
    printf "\\tSorry Linux not yet support.\\n"
    exit 1
  fi

  if [ "$ARCH" == "Darwin" ]; then
    darwin_install_dependencies
  fi
}

main
