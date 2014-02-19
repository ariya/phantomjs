#include "module.hpp"

MyExtension::MyExtension()
    : QObject()
{
}

QString MyExtension::testProperty() const
{
    return "testPropertyValue";
}

QString MyExtension::testFunction()
{
    return "testFunctionResult";
}

MyExtension::~MyExtension()
{
}

extern "C"
{
    Q_DECL_EXPORT QObject * createExtension() { return new MyExtension(); }
}