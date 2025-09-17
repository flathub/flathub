#include "file_transfer.h"
#include <cstring> // For memset
#include <thread> // For usleep on Linux

#ifdef _WIN32
void send_file_tcp(const std::string& serverIp, const std::string& filePath) {
    SOCKET fileSocket;
    sockaddr_in serverAddr;

    fileSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fileSocket == INVALID_SOCKET) {
        std::cerr << "TCP file socket creation failed: " << WSAGetLastError() << std::endl;
        return;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(Constants::Network::TCP_PORT);
    if (inet_pton(AF_INET, serverIp.c_str(), &serverAddr.sin_addr) != 1) {
        std::cerr << "Invalid server IP address for file transfer: " << serverIp << std::endl;
        closesocket(fileSocket);
        return;
    }

    if (connect(fileSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "TCP file connect failed: " << WSAGetLastError() << std::endl;
        closesocket(fileSocket);
        return;
    }
    std::cout << "Connected to server for file transfer." << std::endl;

    std::filesystem::path p(filePath);
    std::string filename = p.filename().string();

    // Send filename
    if (send(fileSocket, filename.c_str(), filename.length(), 0) == SOCKET_ERROR) {
        std::cerr << "Failed to send filename: " << WSAGetLastError() << std::endl;
        closesocket(fileSocket);
        return;
    }
    // Add a small delay or confirmation if needed to ensure server is ready for file content
    Sleep(100); 

    std::ifstream inFile(filePath, std::ios::binary);
    if (!inFile.is_open()) {
        std::cerr << "Failed to open file for reading: " << filePath << std::endl;
        closesocket(fileSocket);
        return;
    }

    char buffer[Constants::Network::BUFFER_SIZE];
    while (!inFile.eof()) {
        inFile.read(buffer, Constants::Network::BUFFER_SIZE);
        int bytesRead = inFile.gcount();
        if (bytesRead > 0) {
            if (send(fileSocket, buffer, bytesRead, 0) == SOCKET_ERROR) {
                std::cerr << "Failed to send file data: " << WSAGetLastError() << std::endl;
                break;
            }
        }
    }

    if (inFile.eof()) {
        std::cout << "File sent successfully: " << filePath << std::endl;
    } else {
        std::cerr << "Error reading file: " << filePath << std::endl;
    }

    inFile.close();
    closesocket(fileSocket);
}

#else // Linux X11 specific
void send_file_tcp_linux(const std::string& serverIp, const std::string& filePath) {
    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "TCP file socket creation error" << std::endl;
        return;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(Constants::Network::TCP_PORT);

    if (inet_pton(AF_INET, serverIp.c_str(), &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid server IP address for file transfer: " << serverIp << std::endl;
        close(sock);
        return;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("TCP file connect failed");
        close(sock);
        return;
    }
    std::cout << "Connected to server for file transfer." << std::endl;

    std::filesystem::path p(filePath);
    std::string filename = p.filename().string();

    // Send filename
    if (send(sock, filename.c_str(), filename.length(), 0) == -1) {
        perror("Failed to send filename");
        close(sock);
        return;
    }
    usleep(100000); // 100ms delay

    std::ifstream inFile(filePath, std::ios::binary);
    if (!inFile.is_open()) {
        std::cerr << "Failed to open file for reading: " << filePath << std::endl;
        close(sock);
        return;
    }

    char buffer[Constants::Network::BUFFER_SIZE];
    while (!inFile.eof()) {
        inFile.read(buffer, Constants::Network::BUFFER_SIZE);
        ssize_t bytesRead = inFile.gcount();
        if (bytesRead > 0) {
            if (send(sock, buffer, bytesRead, 0) == -1) {
                perror("Failed to send file data");
                break;
            }
        }
    }

    if (inFile.eof()) {
        std::cout << "File sent successfully: " << filePath << std::endl;
    } else {
        std::cerr << "Error reading file: " << filePath << std::endl;
    }

    inFile.close();
    close(sock);
}
#endif // _WIN32
