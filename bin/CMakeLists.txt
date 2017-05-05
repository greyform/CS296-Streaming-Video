cmake_minimum_required(VERSION 2.8)
project( Video_Streaming )
find_package( OpenCV REQUIRED )
add_executable( server TCP_server.c )
target_link_libraries( server ${OpenCV_LIBS} )
add_executable( client TCP_client.c )
target_link_libraries( client ${OpenCV_LIBS} )

