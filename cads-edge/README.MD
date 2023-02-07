Building cads on Nano:
It is an ad-hoc process for now.
exports are in env.sh

Update all Packages
sudo apt update && sudo apt upgrade -y


1) Install needed libraries and programs
	sudo apt-get install libsqlite3-dev sqlite3 ninja-build libssl-dev libcurl4 libcurl4-openssl-dev libtbb-dev python3-pip libprotobuf-c-dev

	## Nano ##
  	Compiles with GPU enabled
	. install_opencv4.5.0_Jetson.sh
	cd workspace; sudo make install

	## Newer GCC ##
	sudo add-apt-repository ppa:ubuntu-toolchain-r/test
	sudo apt-get update
	sudo apt install gcc-11 g++-11
	export CXX=`which g++-11`
	export CC=`which gcc-11`

	## Newer cmake ##
	sudo apt remove --purge cmake
	sudo snap install cmake --classic

	## Meson Build ##
	pip3 install --user meson
	
	## Meson in PATH add to ~/.bashrc ##
	if [ -d "$HOME/.local/bin" ] ; then
    		PATH="$HOME/.local/bin:$PATH"
	fi

  ## Load newer gcc in Env ##
  . ./env.sh	


  ## Compile with Meson ##
  This will statically link the gocator api into cads
  meson setup bin --buildtype debugoptimized
  cd debug
  ninja


# Memory Leaks
Valgrind
valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all --log-file="val.log" ./cads -c ../config.json 

GCC Address Sanitzer
Uncomment meson.build



