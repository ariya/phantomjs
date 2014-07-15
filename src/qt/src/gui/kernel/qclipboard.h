/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QCLIPBOARD_H
#define QCLIPBOARD_H

#include <QtCore/qobject.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_CLIPBOARD

class QMimeSource;
class QMimeData;
class QImage;
class QPixmap;

class QClipboardPrivate;

class Q_GUI_EXPORT QClipboard : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QClipboard)
private:
    QClipboard(QObject *parent);
    ~QClipboard();

public:
    enum Mode { Clipboard, Selection, FindBuffer, LastMode = FindBuffer };

    void clear(Mode mode = Clipboard);

    bool supportsSelection() const;
    bool supportsFindBuffer() const;

    bool ownsSelection() const;
    bool ownsClipboard() const;
    bool ownsFindBuffer() const;

    QString text(Mode mode = Clipboard) const;
    QString text(QString& subtype, Mode mode = Clipboard) const;
    void setText(const QString &, Mode mode = Clipboard);

#ifdef QT3_SUPPORT
    QT3_SUPPORT QMimeSource *data(Mode mode = Clipboard) const;
    QT3_SUPPORT void setData(QMimeSource*, Mode mode = Clipboard);
#endif
    const QMimeData *mimeData(Mode mode = Clipboard ) const;
    void setMimeData(QMimeData *data, Mode mode = Clipboard);

    QImage image(Mode mode = Clipboard) const;
    QPixmap pixmap(Mode mode = Clipboard) const;
    void setImage(const QImage &, Mode mode  = Clipboard);
    void setPixmap(const QPixmap &, Mode mode  = Clipboard);

Q_SIGNALS:
    void changed(QClipboard::Mode mode);
    void selectionChanged();
    void findBufferChanged();
    void dataChanged();
private Q_SLOTS:
    void ownerDestroyed();

protected:
    void connectNotify(const char *);
    bool event(QEvent *);

    friend class QApplication;
    friend class QApplicationPrivate;
    friend class QBaseApplication;
    friend class QDragManager;
    friend class QMimeSource;
    friend class QPlatformClipboard;

private:
    Q_DISABLE_COPY(QClipboard)

    bool supportsMode(Mode mode) const;
    bool ownsMode(Mode mode) const;
    void emitChanged(Mode mode);
};

#endif // QT_NO_CLIPBOARD

QT_END_NAMESPACE

QT_END_HEADER

#endif // QCLIPBOARD_H
