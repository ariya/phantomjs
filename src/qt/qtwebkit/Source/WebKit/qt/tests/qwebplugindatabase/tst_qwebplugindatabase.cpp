/*
    Copyright (C) 2009 Jakub Wieczorek <faw217@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <QtTest/QtTest>

#include <qdir.h>
#include <qwebframe.h>
#include <qwebpage.h>
#include <qwebplugindatabase.h>
#include <qwebsettings.h>
#include <qvariant.h>

class tst_QWebPluginDatabase : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void installedPlugins();
    void searchPaths();
    void null_data();
    void null();
    void pluginForMimeType();
    void enabled();
    void operatorequal_data();
    void operatorequal();
    void preferredPlugin();
    void operatorassign_data();
    void operatorassign();
};

typedef QWebPluginInfo::MimeType MimeType;

void tst_QWebPluginDatabase::installedPlugins()
{
    QWebPage page;
    page.settings()->setAttribute(QWebSettings::PluginsEnabled, true);
    QWebFrame* frame = page.mainFrame();

    QVariantMap jsPluginsMap = frame->evaluateJavaScript("window.navigator.plugins").toMap();
    QList<QWebPluginInfo> plugins = QWebSettings::pluginDatabase()->plugins();
    QCOMPARE(plugins, QWebSettings::pluginDatabase()->plugins());

    int length = jsPluginsMap["length"].toInt();
    QCOMPARE(length, plugins.count());

    for (int i = 0; i < length; ++i) {
        QWebPluginInfo plugin = plugins.at(i);

        QVariantMap jsPlugin = frame->evaluateJavaScript(QString("window.navigator.plugins[%1]").arg(i)).toMap();
        QString name = jsPlugin["name"].toString();
        QString description = jsPlugin["description"].toString();
        QString fileName = jsPlugin["filename"].toString();

        QCOMPARE(plugin.name(), name);
        QCOMPARE(plugin.description(), description);
        QCOMPARE(QFileInfo(plugin.path()).fileName(), fileName);

        QList<MimeType> mimeTypes;
        int mimeTypesCount = jsPlugin["length"].toInt();

        for (int j = 0; j < mimeTypesCount; ++j) {
            QVariantMap jsMimeType = frame->evaluateJavaScript(QString("window.navigator.plugins[%1][%2]").arg(i).arg(j)).toMap();

            MimeType mimeType;
            mimeType.name = jsMimeType["type"].toString();
            mimeType.description = jsMimeType["description"].toString();
            mimeType.fileExtensions = jsMimeType["suffixes"].toString().split(',', QString::SkipEmptyParts);

            mimeTypes.append(mimeType);
            QVERIFY(plugin.supportsMimeType(mimeType.name));
        }

        QCOMPARE(plugin.mimeTypes(), mimeTypes);

        QVERIFY(!plugin.isNull());
        QVERIFY(plugin.isEnabled());
    }
}

void tst_QWebPluginDatabase::searchPaths()
{
    QWebPluginDatabase* database = QWebSettings::pluginDatabase();
    QList<QWebPluginInfo> plugins = database->plugins();
    QStringList directories = database->searchPaths();
    QCOMPARE(QWebPluginDatabase::defaultSearchPaths(), directories);

    database->setSearchPaths(directories);
    QCOMPARE(QWebPluginDatabase::defaultSearchPaths(), directories);
    QCOMPARE(database->searchPaths(), directories);
    QCOMPARE(database->plugins(), plugins);
    database->refresh();
    QCOMPARE(database->plugins(), plugins);

    database->setSearchPaths(QStringList());
    QCOMPARE(QWebPluginDatabase::defaultSearchPaths(), directories);
    QCOMPARE(database->searchPaths(), QStringList());
    QCOMPARE(database->plugins().count(), 0);

    database->setSearchPaths(directories);
    QCOMPARE(database->searchPaths(), directories);
    database->addSearchPath(QDir::tempPath());
    QCOMPARE(database->searchPaths().count(), directories.count() + 1);
    QVERIFY(database->searchPaths().contains(QDir::tempPath()));
    directories.append(QDir::tempPath());
    QCOMPARE(database->searchPaths(), directories);

    // As an empty set of search paths has been set, the database has been rebuilt
    // from scratch after bringing the old path set back.
    // Because the QWebPlugins no longer point to the same PluginPackages,
    // the list is also no longer equal to the older one, even though it contains
    // the same information.
    QCOMPARE(database->plugins().count(), plugins.count());
    plugins = database->plugins();
    QCOMPARE(database->plugins(), plugins);

    for (int i = (directories.count() - 1); i >= 0; --i) {
        QDir directory(directories.at(i));
        if (!directory.exists() || !directory.count())
            directories.removeAt(i);
    }

    database->setSearchPaths(directories);
    QCOMPARE(database->plugins(), plugins);
    database->refresh();
    QCOMPARE(database->plugins(), plugins);

    database->setSearchPaths(QWebPluginDatabase::defaultSearchPaths());
    directories = QWebPluginDatabase::defaultSearchPaths();
    QCOMPARE(QWebPluginDatabase::defaultSearchPaths(), directories);
    QCOMPARE(database->searchPaths(), directories);
    QCOMPARE(database->plugins(), plugins);
}

Q_DECLARE_METATYPE(QWebPluginInfo)
void tst_QWebPluginDatabase::null_data()
{
    QTest::addColumn<QWebPluginInfo>("plugin");
    QTest::addColumn<bool>("null");

    QTest::newRow("null") << QWebPluginInfo() << true;
    QTest::newRow("foo") << QWebSettings::pluginDatabase()->pluginForMimeType("foobarbaz") << true;

    QList<QWebPluginInfo> plugins = QWebSettings::pluginDatabase()->plugins();
    for (int i = 0; i < plugins.count(); ++i)
        QTest::newRow(QString::number(i).toUtf8().constData()) << plugins.at(i) << false;
}

void tst_QWebPluginDatabase::null()
{
    QFETCH(QWebPluginInfo, plugin);
    QFETCH(bool, null);

    QCOMPARE(plugin.isNull(), null);
}

void tst_QWebPluginDatabase::pluginForMimeType()
{
    QMultiMap<QString, QWebPluginInfo> pluginsMap;
    QWebPluginDatabase* database = QWebSettings::pluginDatabase();
    QList<QWebPluginInfo> plugins = database->plugins();

    for (int i = 0; i < plugins.count(); ++i) {
        QWebPluginInfo plugin = plugins.at(i);

        QList<MimeType> mimeTypes = plugin.mimeTypes();
        for (int j = 0; j < mimeTypes.count(); ++j) {
            QString mimeType = mimeTypes.at(j).name;
            pluginsMap.insert(mimeType, plugin);
            QVERIFY(plugin.supportsMimeType(mimeType));
        }
    }

    for (int i = 0; i < plugins.count(); ++i) {
        QWebPluginInfo plugin = plugins.at(i);

        QList<MimeType> mimeTypes = plugin.mimeTypes();
        for (int j = 0; j < mimeTypes.count(); ++j) {
            QString mimeType = mimeTypes.at(j).name;

            QVERIFY(pluginsMap.count(mimeType) > 0);
            if (pluginsMap.count(mimeType) > 1)
                continue;

            QWebPluginInfo pluginForMimeType = database->pluginForMimeType(mimeType);
            QCOMPARE(pluginForMimeType, plugin);
            database->setSearchPaths(database->searchPaths());
            QCOMPARE(pluginForMimeType, plugin);
            QCOMPARE(pluginForMimeType, database->pluginForMimeType(mimeType.toUpper()));
            QCOMPARE(pluginForMimeType, database->pluginForMimeType(mimeType.toLower()));
            QVERIFY(plugin.supportsMimeType(mimeType));
            QVERIFY(!pluginForMimeType.isNull());
            QVERIFY(!plugin.isNull());
        }
    }
}

void tst_QWebPluginDatabase::enabled()
{
    QMultiMap<QString, QWebPluginInfo> pluginsMap;
    QWebPluginDatabase* database = QWebSettings::pluginDatabase();
    QList<QWebPluginInfo> plugins = database->plugins();

    for (int i = 0; i < plugins.count(); ++i) {
        QWebPluginInfo plugin = plugins.at(i);

        QList<MimeType> mimeTypes = plugin.mimeTypes();
        for (int j = 0; j < mimeTypes.count(); ++j) {
            QString mimeType = mimeTypes.at(j).name;
            pluginsMap.insert(mimeType, plugin);
            QVERIFY(plugin.supportsMimeType(mimeType));
        }
    }

    QMultiMap<QString, QWebPluginInfo>::iterator it = pluginsMap.begin();
    while (it != pluginsMap.end()) {
        QString mimeType = it.key();
        QWebPluginInfo plugin = it.value();
        QWebPluginInfo pluginForMimeType = database->pluginForMimeType(mimeType);

        QVERIFY(pluginsMap.count(mimeType) > 0);

        if (pluginsMap.count(mimeType) == 1) {
            QCOMPARE(plugin, pluginForMimeType);

            QVERIFY(plugin.isEnabled());
            QVERIFY(pluginForMimeType.isEnabled());
            plugin.setEnabled(false);
            QVERIFY(!plugin.isEnabled());
            QVERIFY(!pluginForMimeType.isEnabled());
        } else {
            QVERIFY(plugin.isEnabled());
            QVERIFY(pluginForMimeType.isEnabled());
            plugin.setEnabled(false);
            QVERIFY(!plugin.isEnabled());
        }

        QVERIFY(!plugin.isNull());
        QVERIFY(!pluginForMimeType.isNull());

        QWebPluginInfo pluginForMimeType2 = database->pluginForMimeType(mimeType);
        if (pluginsMap.count(mimeType) == 1) {
            QVERIFY(pluginForMimeType2 != plugin);
            QVERIFY(pluginForMimeType2.isNull());
        } else {
            QVERIFY(pluginForMimeType2 != plugin);
            QVERIFY(!pluginForMimeType2.isNull());
        }

        plugin.setEnabled(true);

        ++it;
    }
}

void tst_QWebPluginDatabase::operatorequal_data()
{
    QTest::addColumn<QWebPluginInfo>("first");
    QTest::addColumn<QWebPluginInfo>("second");
    QTest::addColumn<bool>("equal");

    QWebPluginDatabase* database = QWebSettings::pluginDatabase();
    QTest::newRow("null") << QWebPluginInfo() << QWebPluginInfo() << true;
    QTest::newRow("application/x-shockwave-flash") << database->pluginForMimeType("application/x-shockwave-flash")
                                                   << database->pluginForMimeType("application/x-shockwave-flash") << true;
    QTest::newRow("foo/bar-baz") << database->pluginForMimeType("foo/bar-baz")
                                 << database->pluginForMimeType("foo/bar-baz") << true;

    QList<QWebPluginInfo> plugins = database->plugins();
    for (int i = 0; i < (plugins.count() - 1); ++i) {
        QWebPluginInfo first = plugins.at(i);
        QWebPluginInfo second = plugins.at(i + 1);

        QTest::newRow(QString("%1==%2").arg(first.name(), second.name()).toUtf8().constData())
                                    << first << second << false;
    }
}

void tst_QWebPluginDatabase::operatorequal()
{
    QFETCH(QWebPluginInfo, first);
    QFETCH(QWebPluginInfo, second);
    QFETCH(bool, equal);

    QCOMPARE(first == second, equal);
}

void tst_QWebPluginDatabase::preferredPlugin()
{
    QMultiMap<QString, QWebPluginInfo> pluginsMap;
    QWebPluginDatabase* database = QWebSettings::pluginDatabase();
    QList<QWebPluginInfo> plugins = database->plugins();

    for (int i = 0; i < plugins.count(); ++i) {
        QWebPluginInfo plugin = plugins.at(i);

        QList<MimeType> mimeTypes = plugin.mimeTypes();
        for (int j = 0; j < mimeTypes.count(); ++j) {
            QString mimeType = mimeTypes.at(j).name;
            pluginsMap.insert(mimeType, plugin);
        }
    }

    QMultiMap<QString, QWebPluginInfo>::iterator it = pluginsMap.begin();
    while (it != pluginsMap.end()) {
        QString mimeType = it.key();

        if (pluginsMap.count(mimeType) > 1) {
            QList<QWebPluginInfo> pluginsForMimeType = pluginsMap.values(mimeType);
            QWebPluginInfo plugin = database->pluginForMimeType(mimeType);
            QVERIFY(plugin.supportsMimeType(mimeType));

            pluginsForMimeType.removeAll(plugin);
            for (int i = 0; i < pluginsForMimeType.count(); ++i) {
                QWebPluginInfo anotherPlugin = pluginsForMimeType.at(i);
                QVERIFY(plugin.supportsMimeType(mimeType));
                QVERIFY(plugin != anotherPlugin);

                QCOMPARE(database->pluginForMimeType(mimeType), plugin);
                database->setPreferredPluginForMimeType(mimeType, anotherPlugin);
                QCOMPARE(database->pluginForMimeType(mimeType), anotherPlugin);

                anotherPlugin.setEnabled(false);
                QCOMPARE(database->pluginForMimeType(mimeType), plugin);

                anotherPlugin.setEnabled(true);
                QCOMPARE(database->pluginForMimeType(mimeType), anotherPlugin);
                database->setSearchPaths(database->searchPaths());
                QCOMPARE(database->pluginForMimeType(mimeType), anotherPlugin);

                database->setPreferredPluginForMimeType(mimeType, QWebPluginInfo());
                QCOMPARE(database->pluginForMimeType(mimeType), plugin);
            }
        } else {
            QWebPluginInfo plugin = database->pluginForMimeType(mimeType);
            QCOMPARE(pluginsMap.value(mimeType), plugin);

            database->setPreferredPluginForMimeType(mimeType, plugin);
            QCOMPARE(database->pluginForMimeType(mimeType), plugin);

            plugin.setEnabled(false);
            QCOMPARE(database->pluginForMimeType(mimeType), QWebPluginInfo());
            plugin.setEnabled(true);

            database->setPreferredPluginForMimeType(mimeType, QWebPluginInfo());
            QCOMPARE(database->pluginForMimeType(mimeType), plugin);
        }

        ++it;
    }

    if (pluginsMap.keys().count() >= 2) {
        QStringList mimeTypes = pluginsMap.uniqueKeys();

        QString mimeType1 = mimeTypes.at(0);
        QString mimeType2 = mimeTypes.at(1);
        QWebPluginInfo plugin1 = database->pluginForMimeType(mimeType1);
        QWebPluginInfo plugin2 = database->pluginForMimeType(mimeType2);

        int i = 2;
        while (plugin2.supportsMimeType(mimeType1)
               && !mimeType2.isEmpty()
               && i < mimeTypes.count()) {
            mimeType2 = mimeTypes.at(i);
            plugin2 = database->pluginForMimeType(mimeType2);
            ++i;
        }

        plugin1 = database->pluginForMimeType(mimeType1);
        QVERIFY(plugin1.supportsMimeType(mimeType1));
        QVERIFY(!plugin1.isNull());
        plugin2 = database->pluginForMimeType(mimeType2);
        QVERIFY(plugin2.supportsMimeType(mimeType2));
        QVERIFY(!plugin2.isNull());

        database->setPreferredPluginForMimeType(mimeType2, plugin1);
        QVERIFY(!plugin1.supportsMimeType(mimeType2));
        QCOMPARE(database->pluginForMimeType(mimeType2), plugin2);

        database->setPreferredPluginForMimeType(mimeType1, plugin1);
        QVERIFY(!plugin2.supportsMimeType(mimeType1));
        QCOMPARE(database->pluginForMimeType(mimeType2), plugin2);
    }
}

void tst_QWebPluginDatabase::operatorassign_data()
{
    QTest::addColumn<QWebPluginInfo>("first");
    QTest::addColumn<QWebPluginInfo>("second");

    QWebPluginDatabase* database = QWebSettings::pluginDatabase();
    QTest::newRow("null") << QWebPluginInfo() << QWebPluginInfo();

    QList<QWebPluginInfo> plugins = database->plugins();
    for (int i = 0; i < (plugins.count() - 1); ++i) {
        QWebPluginInfo first = plugins.at(i);
        QWebPluginInfo second = plugins.at(i + 1);

        QTest::newRow(QString("%1=%2").arg(first.name(), second.name()).toUtf8().constData()) << first << second;
    }
}

void tst_QWebPluginDatabase::operatorassign()
{
    QFETCH(QWebPluginInfo, first);
    QFETCH(QWebPluginInfo, second);

    QWebPluginInfo info;
    QCOMPARE(info.mimeTypes(), QList<MimeType>());
    QCOMPARE(info = first, first);
    QCOMPARE(info, first);
    QCOMPARE(info.mimeTypes(), first.mimeTypes());
    QCOMPARE(info = second, second);
    QCOMPARE(info, second);
    QCOMPARE(info.mimeTypes(), second.mimeTypes());
    QCOMPARE(info = QWebPluginInfo(), QWebPluginInfo());
    QCOMPARE(info.mimeTypes(), QList<MimeType>());
}

QTEST_MAIN(tst_QWebPluginDatabase)

#include "tst_qwebplugindatabase.moc"
