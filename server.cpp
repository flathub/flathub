#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <cstring> // For memset
#include <fstream> // For file operations
#include "constants.h"
#include "file_transfer_server.h"

#ifdef _WIN32
    #include <winsock2.h>
    #include <windows.h> // For input hooks
    #include <ws2tcpip.h> // For inet_pton
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h> // For inet_pton
    #include <unistd.h>
    #include <X11/Xlib.h>
    #include <X11/Xutil.h>
    #include <X11/extensions/XTest.h>
    #include <X11/cursorfont.h> // For XC_left_ptr
#endif

#ifdef _WIN32
HHOOK keyboardHook;
HHOOK mouseHook;
bool onLocalScreen = true; // Flag for server's local screen control
SOCKET udpSocket;
sockaddr_in clientAddr;

// Function to hide the cursor
void hideCursor() {
    while (ShowCursor(FALSE) >= 0);
}

// Function to show the cursor
void showCursor() {
    while (ShowCursor(TRUE) < 0);
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && onLocalScreen) { // Only capture if controlling local screen
        KBDLLHOOKSTRUCT* pKBDLLHookStruct = (KBDLLHOOKSTRUCT*)lParam;
        std::string message;
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            message = Constants::InputMessages::KEY_PRESS + std::to_string(pKBDLLHookStruct->vkCode);
        } else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
            message = Constants::InputMessages::KEY_RELEASE + std::to_string(pKBDLLHookStruct->vkCode);
        }

        if (!message.empty() && udpSocket != INVALID_SOCKET) {
            sendto(udpSocket, message.c_str(), message.length(), 0, (sockaddr *)&clientAddr, sizeof(clientAddr));
        }
    }
    return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        MSLLHOOKSTRUCT* pMSLLHookStruct = (MSLLHOOKSTRUCT*)lParam;
        std::string message;
        POINT p = pMSLLHookStruct->pt; // Use point from hook struct for mouse move

        // Get screen dimensions
        int screen_width = GetSystemMetrics(SM_CXSCREEN);

        if (wParam == WM_MOUSEMOVE) {
            if (onLocalScreen) {
                if (p.x >= screen_width - 1) { // Mouse moved to the right edge
                    onLocalScreen = false;
                    hideCursor();
                    std::cout << "Server: Exit to remote screen, hiding cursor.\n";
                    message = Constants::InputMessages::SCREEN_EXIT + "RIGHT";
                    sendto(udpSocket, message.c_str(), message.length(), 0, (sockaddr *)&clientAddr, sizeof(clientAddr));
                } else {
                    message = Constants::InputMessages::MOUSE_MOVE + std::to_string(p.x) + "," + std::to_string(p.y);
                }
            } else { // Mouse is on client screen, waiting to return
                if (p.x <= 0) { // Mouse moved back to the left edge
                    onLocalScreen = true;
                    showCursor();
                    std::cout << "Server: Return to local screen, showing cursor.\n";
                    message = Constants::InputMessages::SCREEN_ENTER + "LEFT";
                    sendto(udpSocket, message.c_str(), message.length(), 0, (sockaddr *)&clientAddr, sizeof(clientAddr));
                }
                // Do not send mouse move events if not onLocalScreen
            }
        } else if (onLocalScreen) { // Only send other mouse events if controlling local screen
            if (wParam == WM_LBUTTONDOWN) {
                message = Constants::InputMessages::MOUSE_PRESS + "1";
            } else if (wParam == WM_LBUTTONUP) {
                message = Constants::InputMessages::MOUSE_RELEASE + "1";
            } else if (wParam == WM_RBUTTONDOWN) {
                message = Constants::InputMessages::MOUSE_PRESS + "2";
            } else if (wParam == WM_RBUTTONUP) {
                message = Constants::InputMessages::MOUSE_RELEASE + "2";
            } else if (wParam == WM_MBUTTONDOWN) {
                message = Constants::InputMessages::MOUSE_PRESS + "3";
            } else if (wParam == WM_MBUTTONUP) {
                message = Constants::InputMessages::MOUSE_RELEASE + "3";
            } else if (wParam == WM_MOUSEWHEEL) {
                short wheel_delta = GET_WHEEL_DELTA_WPARAM(pMSLLHookStruct->mouseData);
                if (wheel_delta > 0) {
                    message = Constants::InputMessages::MOUSE_SCROLL + Constants::InputMessages::MOUSE_SCROLL_UP;
                } else {
                    message = Constants::InputMessages::MOUSE_SCROLL + Constants::InputMessages::MOUSE_SCROLL_DOWN;
                }
            }
        }

        if (!message.empty() && udpSocket != INVALID_SOCKET) {
            sendto(udpSocket, message.c_str(), message.length(), 0, (sockaddr *)&clientAddr, sizeof(clientAddr));
        }
    }
    return CallNextHookEx(mouseHook, nCode, wParam, lParam);
}

void capture_input_windows() {
    keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
    mouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, GetModuleHandle(NULL), 0);

    if (!keyboardHook || !mouseHook) {
        std::cerr << "Failed to set hooks." << std::endl;
        return;
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(keyboardHook);
    UnhookWindowsHookEx(mouseHook);
}


#else // Linux X11 specific
bool onLocalScreen = true; // Flag for server's local screen control

void capture_input_linux(int udp_sock, const sockaddr_in& client_udp_addr) {
    Display *display = XOpenDisplay(nullptr);
    if (!display) {
        std::cerr << "Error: Could not open X display." << std::endl;
        return;
    }

    Window root = DefaultRootWindow(display);
    int screen_num = DefaultScreen(display);
    int screen_width = XDisplayWidth(display, screen_num);
    // int screen_height = XDisplayHeight(display, screen_num); // Unused

    XGrabKeyboard(display, root, True, GrabModeAsync, GrabModeAsync, CurrentTime);
    XGrabPointer(display, root, False, PointerMotionMask | ButtonPressMask | ButtonReleaseMask,
                 GrabModeAsync, GrabModeAsync, None, None, CurrentTime);

    XEvent event;
    while (true) {
        XNextEvent(display, &event);
        std::string message;

        if (event.type == MotionNotify) {
            int x = event.xmotion.x;
            // int y = event.xmotion.y; // Unused

            if (onLocalScreen) {
                if (x >= screen_width - 1) { // Mouse moved to the right edge
                    message = Constants::InputMessages::SCREEN_EXIT + "RIGHT";
                    onLocalScreen = false;
                    // Hide local cursor
                    XDefineCursor(display, root, XCreateFontCursor(display, 0)); // Invisible cursor
                    XFlush(display);
                } else {
                    message = Constants::InputMessages::MOUSE_MOVE + std::to_string(x) + "," + std::to_string(event.xmotion.y);
                }
            } else { // Mouse is on client screen, waiting to return
                if (x <= 0) { // Mouse moved back to the left edge
                    onLocalScreen = true;
                    message = Constants::InputMessages::SCREEN_ENTER + "LEFT";
                    // Show local cursor
                    XDefineCursor(display, root, XCreateFontCursor(display, XC_left_ptr)); // Default cursor
                    XFlush(display);
                }
                // Do not send mouse move events if not onLocalScreen
            }
        } else if (event.type == KeyPress && onLocalScreen) {
            KeySym key_sym = XLookupKeysym(&event.xkey, 0);
            const char* key_name = XKeysymToString(key_sym);
            if (key_name != nullptr) {
                message = Constants::InputMessages::KEY_PRESS + std::string(key_name);
            }
        } else if (event.type == KeyRelease && onLocalScreen) {
            KeySym key_sym = XLookupKeysym(&event.xkey, 0);
            const char* key_name = XKeysymToString(key_sym);
            if (key_name != nullptr) {
                message = Constants::InputMessages::KEY_RELEASE + std::string(key_name);
            }
        } else if (event.type == ButtonPress && onLocalScreen) {
            message = Constants::InputMessages::MOUSE_PRESS + std::to_string(event.xbutton.button);
        } else if (event.type == ButtonRelease && onLocalScreen) {
            message = Constants::InputMessages::MOUSE_RELEASE + std::to_string(event.xbutton.button);
        }

        if (!message.empty()) {
            sendto(udp_sock, message.c_str(), message.length(), 0, (sockaddr *)&client_udp_addr, sizeof(client_udp_addr));
        }
    }

    XUngrabKeyboard(display, CurrentTime);
    XUngrabPointer(display, CurrentTime);
    XCloseDisplay(display);
}

#endif

int main(int argc, char const *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <Client IP Address>" << std::endl;
        return 1;
    }

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return 1;
    }

    // Setup UDP socket for input
    udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket == INVALID_SOCKET) {
        std::cerr << "UDP socket creation failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons(Constants::Network::UDP_PORT);
    if (inet_pton(AF_INET, argv[1], &clientAddr.sin_addr) != 1) {
        std::cerr << "Invalid client IP address: " << argv[1] << std::endl;
        closesocket(udpSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server started. Sending input events to " << argv[1] << ":" << Constants::Network::UDP_PORT << " via UDP.\n";

    // Start TCP file transfer server in a separate thread
    std::thread file_transfer_thread(handle_file_transfer_server);
    file_transfer_thread.detach(); // Detach to run independently

    capture_input_windows();

    closesocket(udpSocket);
    WSACleanup();

#else // Linux
    // Setup UDP socket for input
    int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sock < 0) {
        std::cerr << "UDP socket creation error" << std::endl;
        return 1;
    }

    struct sockaddr_in client_udp_addr{};
    client_udp_addr.sin_family = AF_INET;
    client_udp_addr.sin_port = htons(Constants::Network::UDP_PORT);
    if (inet_pton(AF_INET, argv[1], &client_udp_addr.sin_addr) != 1) {
        std::cerr << "Invalid client IP address: " << argv[1] << std::endl;
        close(udp_sock);
        return 1;
    }

    std::cout << "Server started. Sending input events to " << argv[1] << ":" << Constants::Network::UDP_PORT << " via UDP.\n";

    // Start TCP file transfer server in a separate thread
    std::thread file_transfer_thread(FileTransferServer::handle_file_transfer_server_linux);
    file_transfer_thread.detach(); // Detach to run independently

    capture_input_linux(udp_sock, client_udp_addr);

    close(udp_sock);
#endif

    return 0;
}
