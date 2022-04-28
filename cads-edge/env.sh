#!/bin/sh 

export CXX=`which g++-11`
export CC=`which gcc-11`
export BOOST_LIBRARYDIR=/home/macro/cads/boost/usr/lib
export BOOST_INCLUDEDIR=/home/macro/cads/boost/usr/include/boost
LD_LIBRARY_PATH=$BOOST_LIBRARYDIR

if [ -d "$HOME/.local/bin" ] ; then
	PATH="$HOME/.local/bin:$PATH"
fi
