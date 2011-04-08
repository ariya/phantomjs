#include <QFile>
#include <QDebug>
#include <iostream>

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
    std::cerr << qPrintable(QString::fromUtf8(file.readAll()));
    file.close();
}

// private:
Utils::Utils()
{
    // Nothing to do here
}
