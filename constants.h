#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <string>

namespace Constants {
    const int DEFAULT_UDP_PORT = 45454;
    const std::string APP_NAME = "Glide App";
    const std::string ORGANIZATION_NAME = "Glide Team";
    const std::string SETTINGS_FILE_NAME = "Settings";

    namespace SettingsKeys {
        const std::string SERVER_UDP_PORT = "server/udpPort";
        const std::string CLIENT_SERVER_IP = "client/serverIP";
        const std::string CLIENT_UDP_PORT = "client/udpPort";
        const std::string WINDOW_GEOMETRY = "window/geometry";

        // General Settings
        const std::string LANGUAGE = "general/language";
        const std::string SCREEN_NAME = "general/screenName";
        const std::string MINIMIZE_TO_TRAY = "general/minimizeToTray";
        const std::string HIDE_ON_STARTUP = "general/hideOnStartup";
        const std::string START_ON_STARTUP = "general/startOnStartup";

        // Networking Settings
        const std::string NETWORK_PORT = "networking/port";
        const std::string NETWORK_ADDRESS = "networking/address";
        const std::string ENABLE_SSL = "networking/enableSsl";
        const std::string REQUIRE_CLIENT_CERT = "networking/requireClientCert";

        // Logging Settings
        const std::string LOG_LEVEL = "logging/level";
        const std::string LOG_FILE_PATH = "logging/filePath";
    } // namespace SettingsKeys

    namespace LogTypes {
        const std::string INFO = "INFO";
        const std::string ERROR = "ERROR";
        const std::string SUCCESS = "SUCCESS";
        const std::string WARNING = "WARNING";
    } // namespace LogTypes

    namespace Network {
        const int UDP_PORT = 45454;
        const int TCP_PORT = 12345; // For file transfer
        const int BUFFER_SIZE = 1024;
    } // namespace Network

    namespace InputMessages {
        const std::string SCREEN_EXIT = "SCREEN_EXIT:";
        const std::string SCREEN_ENTER = "SCREEN_ENTER:";
        const std::string KEY_PRESS = "KEY_PRESS:";
        const std::string KEY_RELEASE = "KEY_RELEASE:";
        const std::string MOUSE_MOVE = "MOUSE_MOVE:";
        const std::string MOUSE_PRESS = "MOUSE_PRESS:";
        const std::string MOUSE_RELEASE = "MOUSE_RELEASE:";
        const std::string MOUSE_SCROLL = "MOUSE_SCROLL:";
        const std::string MOUSE_SCROLL_UP = "UP";
        const std::string MOUSE_SCROLL_DOWN = "DOWN";
    } // namespace InputMessages
} // namespace Constants

#endif // CONSTANTS_H
