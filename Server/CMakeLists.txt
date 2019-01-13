#Server
set(Server_SRC Server.cpp)
add_executable(RUDP-Server Server.cpp ServerSocket.h ServerSocket.cpp ServerSegment.cpp ServerSegment.h SendUnit.cpp SendUnit.h config.h Congestion.cpp Congestion.h Sliding.cpp Sliding.h ServerService.h ServerService.cpp IpAddress.cpp IpAddress.h)