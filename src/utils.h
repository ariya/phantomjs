#ifndef UTILS_H
#define UTILS_H

#include <QtGlobal>

/**
 * Aggregate common utility functions.
 * Functions are static methods.
 * It's important to notice that, at the moment, this class can't be instantiated by design.
 */
class Utils
{
public:
    static void showUsage();
    static void messageHandler(QtMsgType type, const char *msg);

private:
    Utils(); //< This class shouldn't be instantiated
};

#endif // UTILS_H
