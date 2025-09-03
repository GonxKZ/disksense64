#include <QApplication>
#include "ui/mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application properties
    app.setApplicationName("DiskSense64");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("DiskSense");
    
    MainWindow window;
    window.show();
    
    return app.exec();
}

#include "ui/mainwindow.moc"