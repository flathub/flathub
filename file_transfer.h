#ifndef FILE_TRANSFER_H
#define FILE_TRANSFER_H

#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

#include "constants.h"

#ifdef _WIN32
void send_file_tcp(const std::string& serverIp, const std::string& filePath);
#else
void send_file_tcp_linux(const std::string& serverIp, const std::string& filePath);
#endif // _WIN32

#endif // FILE_TRANSFER_H
