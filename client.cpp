#include <iostream>
#include <string>
#include <thread>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include "constants.h"

#ifdef _WIN32
    #include <winsock2.h>
    #include <windows.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <X11/Xlib.h>
    #include <X11/Xutil.h>
    #include <X11/extensions/XTest.h>
    #include <X11/cursorfont.h>
#endif

bool controlling_local_screen = true;

#ifdef _WIN32
void inject_input_windows(const std::string& message) {
    INPUT input;
    memset(&input, 0, sizeof(INPUT));

    if (message.rfind(Constants::InputMessages::SCREEN_EXIT, 0) == 0) {
        controlling_local_screen = true;
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        std::cout << "Client: Taking control, showing cursor." << std::endl;
    } else if (message.rfind(Constants::InputMessages::SCREEN_ENTER, 0) == 0) {
        controlling_local_screen = false;
        SetCursor(NULL);
        std::cout << "Client: Releasing control, hiding cursor." << std::endl;
    } else if (!controlling_local_screen) {
        if (message.rfind(Constants::InputMessages::KEY_PRESS, 0) == 0) {
            input.type = INPUT_KEYBOARD;
            input.ki.wVk = std::stoi(message.substr(Constants::InputMessages::KEY_PRESS.length()));
            input.ki.dwFlags = 0;
            SendInput(1, &input, sizeof(INPUT));
        } else if (message.rfind(Constants::InputMessages::KEY_RELEASE, 0) == 0) {
            input.type = INPUT_KEYBOARD;
            input.ki.wVk = std::stoi(message.substr(Constants::InputMessages::KEY_RELEASE.length()));
            input.ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(1, &input, sizeof(INPUT));
        } else if (message.rfind(Constants::InputMessages::MOUSE_MOVE, 0) == 0) {
            std::string coords_str = message.substr(Constants::InputMessages::MOUSE_MOVE.length());
            size_t comma_pos = coords_str.find(',');
            if (comma_pos != std::string::npos) {
                int x = std::stoi(coords_str.substr(0, comma_pos));
                int y = std::stoi(coords_str.substr(comma_pos + 1));
                SetCursorPos(x, y);
            }
        } else if (message.rfind(Constants::InputMessages::MOUSE_PRESS, 0) == 0) {
            int button = std::stoi(message.substr(Constants::InputMessages::MOUSE_PRESS.length()));
            input.type = INPUT_MOUSE;
            if (button == 1) input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
            else if (button == 2) input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
            else if (button == 3) input.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
            SendInput(1, &input, sizeof(INPUT));
        } else if (message.rfind(Constants::InputMessages::MOUSE_RELEASE, 0) == 0) {
            int button = std::stoi(message.substr(Constants::InputMessages::MOUSE_RELEASE.length()));
            input.type = INPUT_MOUSE;
            if (button == 1) input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
            else if (button == 2) input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
            else if (button == 3) input.mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
            SendInput(1, &input, sizeof(INPUT));
        } else if (message.rfind(Constants::InputMessages::MOUSE_SCROLL, 0) == 0) {
            std::string scroll_dir = message.substr(Constants::InputMessages::MOUSE_SCROLL.length());
            input.type = INPUT_MOUSE;
            input.mi.dwFlags = MOUSEEVENTF_WHEEL;
            input.mi.mouseData = (scroll_dir == Constants::InputMessages::MOUSE_SCROLL_UP) ? WHEEL_DELTA : -WHEEL_DELTA;
            SendInput(1, &input, sizeof(INPUT));
        }
    }
}
#else
void inject_input_linux(const std::string& message) {
    Display *display = XOpenDisplay(nullptr);
    if (!display) {
        std::cerr << "Error: Could not open X display." << std::endl;
        return;
    }

    Window root = DefaultRootWindow(display);

    if (message.rfind(Constants::InputMessages::SCREEN_EXIT, 0) == 0) {
        controlling_local_screen = true;
        XDefineCursor(display, root, XCreateFontCursor(display, XC_left_ptr));
        XFlush(display);
        std::cout << "Client: Taking control, showing cursor." << std::endl;
    } else if (message.rfind(Constants::InputMessages::SCREEN_ENTER, 0) == 0) {
        controlling_local_screen = false;
        XDefineCursor(display, root, XCreateFontCursor(display, 0));
        XFlush(display);
        std::cout << "Client: Releasing control, hiding cursor." << std::endl;
    } else if (!controlling_local_screen) {
        if (message.rfind(Constants::InputMessages::KEY_PRESS, 0) == 0) {
            std::string key_str = message.substr(Constants::InputMessages::KEY_PRESS.length());
            KeySym key_sym = XStringToKeysym(key_str.c_str());
            if (key_sym != NoSymbol) {
                KeyCode key_code = XKeysymToKeycode(display, key_sym);
                XTestFakeKeyEvent(display, key_code, True, CurrentTime);
                XFlush(display);
            }
        } else if (message.rfind(Constants::InputMessages::KEY_RELEASE, 0) == 0) {
            std::string key_str = message.substr(Constants::InputMessages::KEY_RELEASE.length());
            KeySym key_sym = XStringToKeysym(key_str.c_str());
            if (key_sym != NoSymbol) {
                KeyCode key_code = XKeysymToKeycode(display, key_sym);
                XTestFakeKeyEvent(display, key_code, False, CurrentTime);
                XFlush(display);
            }
        } else if (message.rfind(Constants::InputMessages::MOUSE_MOVE, 0) == 0) {
            std::string coords_str = message.substr(Constants::InputMessages::MOUSE_MOVE.length());
            size_t comma_pos = coords_str.find(',');
            if (comma_pos != std::string::npos) {
                int x = std::stoi(coords_str.substr(0, comma_pos));
                int y = std::stoi(coords_str.substr(comma_pos + 1));
                XTestFakeMotionEvent(display, -1, x, y, CurrentTime);
                XFlush(display);
            }
        } else if (message.rfind(Constants::InputMessages::MOUSE_PRESS, 0) == 0) {
            int button = std::stoi(message.substr(Constants::InputMessages::MOUSE_PRESS.length()));
            XTestFakeButtonEvent(display, button, True, CurrentTime);
            XFlush(display);
        } else if (message.rfind(Constants::InputMessages::MOUSE_RELEASE, 0) == 0) {
            int button = std::stoi(message.substr(Constants::InputMessages::MOUSE_RELEASE.length()));
            XTestFakeButtonEvent(display, button, False, CurrentTime);
            XFlush(display);
        }
    }

    XCloseDisplay(display);
}
#endif

int main(int argc, char const *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <Server IP Address>" << std::endl;
        return 1;
    }

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return 1;
    }

    SOCKET udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket == INVALID_SOCKET) {
        std::cerr << "UDP socket creation failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in localAddr{};
    localAddr.sin_family = AF_INET;
    localAddr.sin_port = htons(Constants::Network::UDP_PORT);
    localAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(udpSocket, (sockaddr*)&localAddr, sizeof(localAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        closesocket(udpSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Client listening for input events on port " << Constants::Network::UDP_PORT << " via UDP.\n";

    char buffer[Constants::Network::BUFFER_SIZE];
    while (true) {
        memset(buffer, 0, Constants::Network::BUFFER_SIZE);
        int len = recvfrom(udpSocket, buffer, sizeof(buffer) - 1, 0, nullptr, nullptr);
        if (len > 0) {
            buffer[len] = '\0';
            std::string received_message(buffer);
            std::cout << "Received: " << received_message << std::endl;
            inject_input_windows(received_message);
        } else if (len == 0) {
            std::cout << "Server disconnected.\n";
            break;
        } else {
            std::cerr << "recvfrom failed: " << WSAGetLastError() << std::endl;
            break;
        }
    }

    closesocket(udpSocket);
    WSACleanup();
#else
    int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sock < 0) {
        std::cerr << "UDP socket creation error" << std::endl;
        return 1;
    }

    struct sockaddr_in local_udp_addr{};
    local_udp_addr.sin_family = AF_INET;
    local_udp_addr.sin_port = htons(Constants::Network::UDP_PORT);
    local_udp_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(udp_sock, (struct sockaddr *)&local_udp_addr, sizeof(local_udp_addr)) < 0) {
        perror("UDP bind failed");
        return 1;
    }

    std::cout << "Client listening for input events on port " << Constants::Network::UDP_PORT << " via UDP.\n";

    char udp_buffer[Constants::Network::BUFFER_SIZE] = {0};
    while (true) {
        memset(udp_buffer, 0, Constants::Network::BUFFER_SIZE);
        ssize_t valread = recvfrom(udp_sock, udp_buffer, Constants::Network::BUFFER_SIZE, 0, nullptr, nullptr);
        if (valread <= 0) {
            std::cout << "Server disconnected or error." << std::endl;
            break;
        }
        std::string received_message(udp_buffer);
        std::cout << "Received: " << received_message << std::endl;
        inject_input_linux(received_message);
    }

    close(udp_sock);
#endif

    return 0;
}
