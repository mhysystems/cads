#!/bin/sh 

export CXX=`which g++-10`
export CC=`which gcc-10`
export BOOST_LIBRARYDIR=/home/macro/cads/boost/usr/lib
export BOOST_INCLUDEDIR=/home/macro/cads/boost/usr/include/boost

if [ -d "$HOME/.local/bin" ] ; then
	PATH="$HOME/.local/bin:$PATH"
fi
