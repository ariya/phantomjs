#include <QObject>

class MyExtension : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString testProperty READ testProperty)

public:
    MyExtension();
    virtual ~MyExtension();

    QString testProperty() const;

public slots:
    QString testFunction();
};