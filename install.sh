flatpak remote-delete UDPLoggerRepo
flatpak remote-add UDPLoggerRepo UDPLoggerRepo --no-gpg-verify --if-not-exists
flatpak install UDPLoggerRepo com.gitlab.Murmele.UDPLogger
