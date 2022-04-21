Building cads on Nano:
It is an ad-hoc process for now.
exports are in env.sh

Update all Packages
sudo apt update && sudo apt upgrade -y


1) Install needed libraries and programs
	sudo apt-get install libsqlite3-dev sqlite3 ninja-build libssl-dev libcurl4 libcurl4-openssl-dev libtbb-dev python3-pip

	## Nano ##
  	Compiles with GPU enabled
	. install_opencv4.5.0_Jetson.sh
	cd workspace; sudo make install

	## Newer GCC ##
	sudo add-apt-repository ppa:ubuntu-toolchain-r/test
	sudo apt-get update
	sudo apt install gcc-10 g++-10
	export CXX=`which g++-10`
	export CC=`which gcc-10`

	## Newer cmake ##
	sudo apt remove --purge cmake
	sudo snap install cmake --classic

	## Meson Build ##
	pip3 install --user meson
	
	## Boost 1.74 ##
	sudo add-apt-repository ppa:mhier/libboost-latest
	sudo apt-get update
  sudo apt-get install --download-only libboost1.74-dev
  
  ## Extract boost 1.74 for linking via LD_LIBRARY ##
  mkdir boost
  dpkg-deb -R /var/cache/apt/archives/libboost1.74-dev_1.74-0~16~ubuntu18.04.1_arm64.deb boost
  rm -rf boost/DEBIAN
  dpkg-deb -R /var/cache/apt/archives/libboost1.74_1.74-0~16~ubuntu18.04.1_arm64.deb boost
	export BOOST_LIBRARYDIR=$(pwd)/boost/usr/lib
	export BOOST_INCLUDEDIR=$(pwd)/boost/usr/include/boost
  export LD_LIBRARY=$BOOST_LIBRARYDIR
	
	## Meson in PATH add to ~/.bashrc ##
	if [ -d "$HOME/.local/bin" ] ; then
    		PATH="$HOME/.local/bin:$PATH"
	fi

  ## Load newer gcc in Env ##
  . ./env.sh	

  ## Compile with Meson ##
  This will statically link the gocator api into cads
  meson setup debug --buildtype debug
  cd debug
  ninja

  ### Release
  meson setup release --buildtype release
  cd release
  ninja



