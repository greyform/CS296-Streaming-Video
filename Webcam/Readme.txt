Install OpenCV on Ubuntu

INSTALL OPENCV FROM THE OFFICIAL SITE

To install the latest version of OpenCV be sure that you have removed the library from the repository with sudo apt-get autoremove libopencv-dev python-opencv and follow the steps below.

Download the installation script install-opencv.sh, open your terminal and execute:

bash install-opencv.sh

Type your sudo password and you will have installed OpenCV. This operation may take a long time due to the packages to be installed and the compilation process.

Download cmake from the official website:
https://cmake.org/download/

If there is no existing CMake installation, a bootstrap script is provided to build CMake:
  ./bootstrap
  make
  make install
