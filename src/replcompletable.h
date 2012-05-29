#ifndef REPLCOMPLETABLE_H
#define REPLCOMPLETABLE_H

#include <QMultiHash>
#include <QStringList>
#include <QCache>

/**
  * This subclass of QObject is used by the REPL to better control
  * what is "shown" of a QObject exposed in the Javascript Runtime.
  *
  * By default the JS environment will see all the slots and the Q_INVOKABLE
  * of a "exposed" QObject. But also some extra QObject specific methods
  * that, in our case, we prefer not to list during REPL autocompletion
  * listing or expression result prettyfication.
  */
class REPLCompletable : public QObject
{
    Q_OBJECT

public:
    REPLCompletable(QObject *parent = 0);
    virtual ~REPLCompletable();

    Q_INVOKABLE bool _isCompletable();
    Q_INVOKABLE QStringList _getCompletions(const QString &prefixToComplete);

protected:
    /**
     * Used by sublcasses to register a possible "completion".
     *
     * @param completion Array of characters representing a function/property
     *        that will be listed as possible completion
     */
    void addCompletion(const char *completion);
    /**
     * Used by sublcasses to register a possible "completion".
     *
     * @param completion String representing a function/property
     *        that will be listed as possible completion
     */
    void addCompletion(QString completion);

private:
    /**
     * This is where subclasses should use REPLCompletable#addCompletion(...)
     * to declare/register their completions for the REPL.
     * This ensures that ONLY if a REPL is actually requested by the user,
     * we bother registering the completion strings.
     */
    virtual void initCompletions() = 0;

    static QMultiHash<const char *, QString> *getCompletionsIndex();
    static QCache<QString, QStringList> *getCompletionsCache();

private:
    bool mCompletionsInitialised;
};

#endif // REPLCOMPLETABLE_H
