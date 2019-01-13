#Client
set(Client_SRC Client.cpp)
add_executable(RUDP-Client Client.cpp ClientSocket.cpp ClientSocket.h ClientSegment.cpp ClientSegment.h config.h ClientService.h ClientService.cpp ReceiverBuffer.h ReceiverBuffer.cpp)