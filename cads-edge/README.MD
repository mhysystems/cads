Building cads on Nano:
It is an ad-hoc process for now.
exports are in env.sh

Update all Packages
sudo apt update && sudo apt upgrade -y


1) Install needed libraries and programs
	sudo apt-get install libsqlite3-dev sqlite3 ninja-build libssl-dev libcurl4 libcurl4-openssl-dev libtbb-dev python3-pip libprotobuf-c-dev libboost-all-dev libgsl-dev libgflags-dev

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
valgrind --tool=memcheck --track-origins=yes --leak-check=full --show-leak-kinds=all --log-file="val.log" ./cads -c ../config.json 

GCC Address Sanitzer
Uncomment meson.build

# Windows Build
Install vcpkg
VCPKG_ROOT=C:\Users\me\scoop\apps\vcpkg\current
CMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
PKG_CONFIG=C:\Users\me\scoop\apps\vcpkg\current\installed\x64-windows\tools\pkgconf\pkgconf.exe
PKG_CONFIG_PATH=C:\Users\me\scoop\apps\vcpkg\current\installed\x64-windows\debug\lib\pkgconfig
VCPKG_DEFAULT_TRIPLET=x64-windows

maybe cross file
meson setup --cross-file meson-vcpkg.txt win

#Cuda on nano to stop the driver killing the app
echo N > /sys/kernel/debug/gpu.0/timeouts_enabled

matrix profile with length of 31323 and window = 333 on nano takes about 1sec.

#Jetson Nano upgrade issue
https://forums.developer.nvidia.com/t/solution-dpkg-error-processing-package-nvidia-l4t-bootloader-configure/208627
sudo mv /var/lib/dpkg/info/ /var/lib/dpkg/backup/
sudo mkdir /var/lib/dpkg/info/
sudo apt-get update
sudo apt-get -f install
sudo mv /var/lib/dpkg/info/* /var/lib/dpkg/backup/
sudo rm -rf /var/lib/dpkg/info
sudo mv /var/lib/dpkg/backup/ /var/lib/dpkg/info/

#Upgrading python
Use pyenv
https://github.com/pyenv/pyenv




