/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QGUIPLATFORM_P_H
#define QGUIPLATFORM_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qplugin.h>
#include <QtCore/qfactoryinterface.h>
#include <QtGui/qdialog.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QStyle;
class QPalette;
class QIcon;
class QFileDialog;
class QColorDialog;
class QFileInfo;

struct Q_GUI_EXPORT QGuiPlatformPluginInterface  : public QFactoryInterface
{
};

#define QGuiPlatformPluginInterface_iid "com.nokia.qt.QGuiPlatformPluginInterface"

Q_DECLARE_INTERFACE(QGuiPlatformPluginInterface, QGuiPlatformPluginInterface_iid)

class Q_GUI_EXPORT QGuiPlatformPlugin : public QObject, public QGuiPlatformPluginInterface
{
    Q_OBJECT
    Q_INTERFACES(QGuiPlatformPluginInterface:QFactoryInterface)
    public:
        explicit QGuiPlatformPlugin(QObject *parent = 0);
        ~QGuiPlatformPlugin();

        virtual QStringList keys() const {  return QStringList() << QLatin1String("default");  };

        virtual QString styleName();
        virtual QPalette palette();
        virtual QString systemIconThemeName();
        virtual QStringList iconThemeSearchPaths();
        virtual QIcon fileSystemIcon(const QFileInfo &);

        enum PlatformHint { PH_ToolButtonStyle, PH_ToolBarIconSize, PH_ItemView_ActivateItemOnSingleClick };
        virtual int platformHint(PlatformHint hint);


        virtual void fileDialogDelete(QFileDialog *) {}
        virtual bool fileDialogSetVisible(QFileDialog *, bool) { return false; }
        virtual QDialog::DialogCode fileDialogResultCode(QFileDialog *) { return QDialog::Rejected; }
        virtual void fileDialogSetDirectory(QFileDialog *, const QString &) {}
        virtual QString fileDialogDirectory(const QFileDialog *) const { return QString(); }
        virtual void fileDialogSelectFile(QFileDialog *, const QString &) {}
        virtual QStringList fileDialogSelectedFiles(const QFileDialog *) const { return QStringList(); }
        virtual void fileDialogSetFilter(QFileDialog *) {}
        virtual void fileDialogSetNameFilters(QFileDialog *, const QStringList &) {}
        virtual void fileDialogSelectNameFilter(QFileDialog *, const QString &) {}
        virtual QString fileDialogSelectedNameFilter(const QFileDialog *) const { return QString(); }

        virtual void colorDialogDelete(QColorDialog *) {}
        virtual bool colorDialogSetVisible(QColorDialog *, bool) { return false; }
        virtual void colorDialogSetCurrentColor(QColorDialog *, const QColor &) {}
};

//internal
QGuiPlatformPlugin *qt_guiPlatformPlugin();

QT_END_NAMESPACE

QT_END_HEADER


#endif // QGUIPLATFORMPLUGIN_H
