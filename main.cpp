#include <QApplication>
#include "mainwindow.h" // This would be the header file for the above class

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application properties
    app.setApplicationName("Glide App");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("Glide Team");
    
    // Create and show main window
    GlideMainWindow window;
    window.show();
    
    return app.exec();
}
