Building cads on Nano:
It is an ad-hoc process for now.

1) Install needed libraries and programs
	sudo apt-get install libboost-all-dev
	sudo apt-get install libsqlite3-dev
	sudo apt-get install sqlite3
	sudo apt-get install ninja
	sudo apt-get install libspdlog-dev	
	sudo apt-get install libssl-dev
  sudo apt-get install libcurl4 libcurl4-openssl-dev

	## Nano ##
  Compiles with GPU enabled
	. install_opencv4.5.0_Jetson.sh
	cd workspace; sudo make install

	## Flat Buffers ##
	git clone --depth=1 https://github.com/google/flatbuffers.git
	cd flatbuffers; mkdir build; cd build; cmake .. ; cmake --build; sudo make install

	## Newer GCC ##
	sudo add-apt-repository ppa:ubuntu-toolchain-r/test
	sudo apt-get update
	sudo apt install gcc-10
	sudo apt install g++-10
	export CXX=`which g++-10`
	export CC=`which gcc-10`

	## Newer cmake ##
	sudo apt remove --purge cmake
	sudo snap install cmake --classic

  ## Load newer gcc in Env ##
  . ./env.sh	

  ## Compile with Meson ##
  This will statically link the gocator api into cads
  meson setup debug --buildtype debug
  cd builddir
  ninja

  ### Release
  meson setup release --buildtype release
  cd builddir
  ninja



