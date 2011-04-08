#include <iostream>
#include <QFile>
#include <QDebug>
#include <QDateTime>

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
    QDateTime now = QDateTime::currentDateTime();

    switch (type) {
    case QtDebugMsg:
        fprintf(stdout, "%s [DEBUG] %s\n", qPrintable(now.toString(Qt::ISODate)), msg);
        break;
    case QtWarningMsg:
        fprintf(stderr, "%s [WARNING] %s\n", qPrintable(now.toString(Qt::ISODate)), msg);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "%s [CRITICAL] %s\n", qPrintable(now.toString(Qt::ISODate)), msg);
        break;
    case QtFatalMsg:
        fprintf(stderr, "%s [FATAL] %s\n", qPrintable(now.toString(Qt::ISODate)), msg);
        abort();
    }
}

// private:
Utils::Utils()
{
    // Nothing to do here
}
