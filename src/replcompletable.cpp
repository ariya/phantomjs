#include "replcompletable.h"

// public:
REPLCompletable::REPLCompletable(QObject *parent)
    : QObject(parent),
      mCompletionsInitialised(false)
{ }

REPLCompletable::~REPLCompletable()
{ }

bool REPLCompletable::_isCompletable()
{
    return true;
}

QStringList REPLCompletable::_getCompletions(const QString &prefix)
{
    // First time this method is invoked, initialise the completions (lazy init)
    if (!mCompletionsInitialised) {
        initCompletions();
        mCompletionsInitialised = true;
    }

    // If no prefix provided?
    if (prefix.isEmpty()) {
        // Return all the possible completions
        return REPLCompletable::getCompletionsIndex()->values(
                    this->metaObject()->className());
    }

    // make a key to store the (new) completions list
    QString cacheKey = QString("%1-%2").arg(this->metaObject()->className()).arg(prefix);
    // If a list of completion withi this key is not already in the cache
    if (!getCompletionsCache()->contains(cacheKey)) {
        // Loop over the completions and pick the one that match the given prefix
        QStringList allCompletions = REPLCompletable::getCompletionsIndex()->values(
                    this->metaObject()->className());
        QStringList *matchingPrefixCompletions = new QStringList();

        QStringList::iterator i;
        for (i = allCompletions.begin(); i != allCompletions.end(); ++i) {
            if (((QString) *i).startsWith(prefix)) {
                matchingPrefixCompletions->append((QString) *i);
            }
        }

        // Store the result in the cache
        getCompletionsCache()->insert(cacheKey, matchingPrefixCompletions);
    }

    return *(getCompletionsCache()->object(cacheKey));
}

// protected:
void REPLCompletable::addCompletion(const char *completion)
{
    addCompletion(QString(completion));
}

void REPLCompletable::addCompletion(QString completion)
{
    // Accept a completion only if it's unique per MetaObject ClassName
    if (!REPLCompletable::getCompletionsIndex()->contains(this->metaObject()->className(), completion)) {
        REPLCompletable::getCompletionsIndex()->insert(this->metaObject()->className(), completion);
    }
}

// private:
QMultiHash<const char *, QString> *REPLCompletable::getCompletionsIndex()
{
    static QMultiHash<const char *, QString> *compIndex = NULL;
    if (!compIndex) {
        compIndex = new QMultiHash<const char *, QString>();
    }
    return compIndex;
}

QCache<QString, QStringList> *REPLCompletable::getCompletionsCache()
{
    static QCache<QString, QStringList> *compCache = NULL;
    if (!compCache) {
        compCache = new QCache<QString, QStringList>();
    }
    return compCache;
}
