#include "file_transfer_server.h"
#include "constants.h"
#include <iostream>
#include <fstream>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#endif

namespace FileTransferServer {

#ifdef _WIN32
void handle_file_transfer_server_windows() {
    SOCKET listenSocket, clientSocket;
    sockaddr_in serverAddr, clientAddr;
    int clientAddrSize = sizeof(clientAddr);

    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        std::cerr << "TCP listen socket creation failed: " << WSAGetLastError() << std::endl;
        return;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(Constants::Network::TCP_PORT);

    if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "TCP bind failed: " << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        return;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "TCP listen failed: " << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        return;
    }

    std::cout << "TCP Server listening for file transfers on port " << Constants::Network::TCP_PORT << std::endl;

    while (true) {
        clientSocket = accept(listenSocket, (sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "TCP accept failed: " << WSAGetLastError() << std::endl;
            continue;
        }
        std::cout << "Client connected for file transfer." << std::endl;

        char filenameBuffer[Constants::Network::BUFFER_SIZE];
        int bytesReceived = recv(clientSocket, filenameBuffer, Constants::Network::BUFFER_SIZE, 0);
        if (bytesReceived <= 0) {
            std::cerr << "Failed to receive filename or client disconnected." << std::endl;
            closesocket(clientSocket);
            continue;
        }
        filenameBuffer[bytesReceived] = '\0';
        std::string filename = filenameBuffer;
        std::cout << "Receiving file: " << filename << std::endl;

        std::ofstream outFile(filename, std::ios::binary);
        if (!outFile.is_open()) {
            std::cerr << "Failed to open file for writing: " << filename << std::endl;
            closesocket(clientSocket);
            continue;
        }

        char fileBuffer[Constants::Network::BUFFER_SIZE];
        while ((bytesReceived = recv(clientSocket, fileBuffer, Constants::Network::BUFFER_SIZE, 0)) > 0) {
            outFile.write(fileBuffer, bytesReceived);
        }

        if (bytesReceived == 0) {
            std::cout << "File received successfully: " << filename << std::endl;
        } else {
            std::cerr << "Error receiving file: " << WSAGetLastError() << std::endl;
        }

        outFile.close();
        closesocket(clientSocket);
    }
    closesocket(listenSocket);
}
#else
void handle_file_transfer_server_linux() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("TCP socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(Constants::Network::TCP_PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("TCP bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("TCP listen");
        exit(EXIT_FAILURE);
    }

    std::cout << "TCP Server listening for file transfers on port " << Constants::Network::TCP_PORT << std::endl;

    while (true) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("TCP accept");
            continue;
        }
        std::cout << "Client connected for file transfer." << std::endl;

        char filenameBuffer[Constants::Network::BUFFER_SIZE];
        ssize_t bytesReceived = recv(new_socket, filenameBuffer, Constants::Network::BUFFER_SIZE, 0);
        if (bytesReceived <= 0) {
            std::cerr << "Failed to receive filename or client disconnected." << std::endl;
            close(new_socket);
            continue;
        }
        filenameBuffer[bytesReceived] = '\0';
        std::string filename = filenameBuffer;
        std::cout << "Receiving file: " << filename << std::endl;

        std::ofstream outFile(filename, std::ios::binary);
        if (!outFile.is_open()) {
            std::cerr << "Failed to open file for writing: " << filename << std::endl;
            close(new_socket);
            continue;
        }

        char fileBuffer[Constants::Network::BUFFER_SIZE];
        while ((bytesReceived = recv(new_socket, fileBuffer, Constants::Network::BUFFER_SIZE, 0)) > 0) {
            outFile.write(fileBuffer, bytesReceived);
        }

        if (bytesReceived == 0) {
            std::cout << "File received successfully: " << filename << std::endl;
        } else {
            perror("Error receiving file");
        }

        outFile.close();
        close(new_socket);
    }
    close(server_fd);
}
#endif

} // namespace FileTransferServer
