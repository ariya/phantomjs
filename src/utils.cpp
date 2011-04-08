#include <iostream>
#include <QFile>
#include <QDebug>

#include "utils.h"

// public:
void Utils::showUsage()
{
    QFile file;
    file.setFileName(":/usage.txt");
    if ( !file.open(QFile::ReadOnly) ) {
        qFatal("Unable to print the usage message");
        exit(1);
    }
    std::cout << qPrintable(QString::fromUtf8(file.readAll()));
    file.close();
}

void Utils::messageHandler(QtMsgType type, const char *msg)
{
    switch (type) {
    case QtDebugMsg:
        fprintf(stdout, "[DEBUG]: %s\n", msg);
        break;
    case QtWarningMsg:
        fprintf(stderr, "[WARNING]: %s\n", msg);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "[CRITICAL]: %s\n", msg);
        break;
    case QtFatalMsg:
        fprintf(stderr, "[FATAL]: %s\n", msg);
        abort();
    }
}

// private:
Utils::Utils()
{
    // Nothing to do here
}
