This project aims to build a surveillance cam system, where images are grabbed from camera on one machine and transfered to another machine via TCP, resulting in a live stream of video feed. To avoid latency, every frame is individually encoded to jpeg format by OpenCV to drastically reduce the bandwidth consumption.



Install OpenCV 

To install the latest version of OpenCV be sure that you have removed the library from the repository with sudo apt-get autoremove libopencv-dev python-opencv and follow the steps below.

Download the installation script install-opencv.sh, open your terminal and execute:

bash install-opencv.sh

Type your sudo password and you will have installed OpenCV. This operation may take a long time due to the packages to be installed and the compilation process.


Install CMAKE

Download cmake from the official website:
https://cmake.org/download/

If there is no existing CMake installation, a bootstrap script is provided to build CMake:
  ./bootstrap
  make
  make install


Setup

Run the following code to set up server/client on your computer: (CMake and OpenCV required)

git clone https://github.com/greyform/CS296-Streaming-Video.git

cd CS296-Streaming-Video/bin/

cmake . && make

Usage
Run the following command to see stream your camera through localhost:

./server <port> [device number] [resize factor]   
// The device number is set to 0 as default. To change input source, change the argument accordingly. 
// The resize factor is the fraction of the original window that will be reduced. 


./client [server address] [port]

//For demo purpose, use 127.0.0.1 as server address to run client on the same host as server.
//You should see two windows, one with the original grabbed image (before encoding) as "local webcam" and one ///with received live-stream image (after encoding/decoding) as "TCP stream webcam".
