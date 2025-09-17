#ifndef FILE_TRANSFER_SERVER_H
#define FILE_TRANSFER_SERVER_H

#include <string>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#endif

namespace FileTransferServer {
#ifdef _WIN32
    void handle_file_transfer_server_windows();
#else
    void handle_file_transfer_server_linux();
#endif
} // namespace FileTransferServer

#endif // FILE_TRANSFER_SERVER_H
