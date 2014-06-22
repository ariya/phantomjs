/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#ifndef QWIZARD_H
#define QWIZARD_H

#include <QtWidgets/qdialog.h>

QT_BEGIN_NAMESPACE


#ifndef QT_NO_WIZARD

class QAbstractButton;
class QWizardPage;
class QWizardPrivate;

class Q_WIDGETS_EXPORT QWizard : public QDialog
{
    Q_OBJECT
    Q_ENUMS(WizardStyle WizardOption)
    Q_FLAGS(WizardOptions)
    Q_PROPERTY(WizardStyle wizardStyle READ wizardStyle WRITE setWizardStyle)
    Q_PROPERTY(WizardOptions options READ options WRITE setOptions)
    Q_PROPERTY(Qt::TextFormat titleFormat READ titleFormat WRITE setTitleFormat)
    Q_PROPERTY(Qt::TextFormat subTitleFormat READ subTitleFormat WRITE setSubTitleFormat)
    Q_PROPERTY(int startId READ startId WRITE setStartId)
    Q_PROPERTY(int currentId READ currentId NOTIFY currentIdChanged)

public:
    enum WizardButton {
        BackButton,
        NextButton,
        CommitButton,
        FinishButton,
        CancelButton,
        HelpButton,
        CustomButton1,
        CustomButton2,
        CustomButton3,
        Stretch,

        NoButton = -1,
        NStandardButtons = 6,
        NButtons = 9
    };

    enum WizardPixmap {
        WatermarkPixmap,
        LogoPixmap,
        BannerPixmap,
        BackgroundPixmap,
        NPixmaps
    };

    enum WizardStyle {
        ClassicStyle,
        ModernStyle,
        MacStyle,
        AeroStyle,
        NStyles
    };

    enum WizardOption {
        IndependentPages                = 0x00000001,
        IgnoreSubTitles                 = 0x00000002,
        ExtendedWatermarkPixmap         = 0x00000004,
        NoDefaultButton                 = 0x00000008,
        NoBackButtonOnStartPage         = 0x00000010,
        NoBackButtonOnLastPage          = 0x00000020,
        DisabledBackButtonOnLastPage    = 0x00000040,
        HaveNextButtonOnLastPage        = 0x00000080,
        HaveFinishButtonOnEarlyPages    = 0x00000100,
        NoCancelButton                  = 0x00000200,
        CancelButtonOnLeft              = 0x00000400,
        HaveHelpButton                  = 0x00000800,
        HelpButtonOnRight               = 0x00001000,
        HaveCustomButton1               = 0x00002000,
        HaveCustomButton2               = 0x00004000,
        HaveCustomButton3               = 0x00008000,
        NoCancelButtonOnLastPage        = 0x00010000
    };

    Q_DECLARE_FLAGS(WizardOptions, WizardOption)

    explicit QWizard(QWidget *parent = 0, Qt::WindowFlags flags = 0);
    ~QWizard();

    int addPage(QWizardPage *page);
    void setPage(int id, QWizardPage *page);
    void removePage(int id);
    QWizardPage *page(int id) const;
    bool hasVisitedPage(int id) const;
    QList<int> visitedPages() const;    // ### Qt 6: visitedIds()?
    QList<int> pageIds() const;
    void setStartId(int id);
    int startId() const;
    QWizardPage *currentPage() const;
    int currentId() const;

    virtual bool validateCurrentPage();
    virtual int nextId() const;

    void setField(const QString &name, const QVariant &value);
    QVariant field(const QString &name) const;

    void setWizardStyle(WizardStyle style);
    WizardStyle wizardStyle() const;

    void setOption(WizardOption option, bool on = true);
    bool testOption(WizardOption option) const;
    void setOptions(WizardOptions options);
    WizardOptions options() const;

    void setButtonText(WizardButton which, const QString &text);
    QString buttonText(WizardButton which) const;
    void setButtonLayout(const QList<WizardButton> &layout);
    void setButton(WizardButton which, QAbstractButton *button);
    QAbstractButton *button(WizardButton which) const;

    void setTitleFormat(Qt::TextFormat format);
    Qt::TextFormat titleFormat() const;
    void setSubTitleFormat(Qt::TextFormat format);
    Qt::TextFormat subTitleFormat() const;
    void setPixmap(WizardPixmap which, const QPixmap &pixmap);
    QPixmap pixmap(WizardPixmap which) const;

    void setSideWidget(QWidget *widget);
    QWidget *sideWidget() const;

    void setDefaultProperty(const char *className, const char *property,
                            const char *changedSignal);

    void setVisible(bool visible);
    QSize sizeHint() const;

Q_SIGNALS:
    void currentIdChanged(int id);
    void helpRequested();
    void customButtonClicked(int which);
    void pageAdded(int id);
    void pageRemoved(int id);

public Q_SLOTS:
    void back();
    void next();
    void restart();

protected:
    bool event(QEvent *event);
    void resizeEvent(QResizeEvent *event);
    void paintEvent(QPaintEvent *event);
#ifdef Q_OS_WIN
    bool nativeEvent(const QByteArray &eventType, void * message, long * result);
#endif
    void done(int result);
    virtual void initializePage(int id);
    virtual void cleanupPage(int id);

private:
    Q_DISABLE_COPY(QWizard)
    Q_DECLARE_PRIVATE(QWizard)
    Q_PRIVATE_SLOT(d_func(), void _q_emitCustomButtonClicked())
    Q_PRIVATE_SLOT(d_func(), void _q_updateButtonStates())
    Q_PRIVATE_SLOT(d_func(), void _q_handleFieldObjectDestroyed(QObject *))

    friend class QWizardPage;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QWizard::WizardOptions)

class QWizardPagePrivate;

class Q_WIDGETS_EXPORT QWizardPage : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString title READ title WRITE setTitle)
    Q_PROPERTY(QString subTitle READ subTitle WRITE setSubTitle)

public:
    explicit QWizardPage(QWidget *parent = 0);
    ~QWizardPage();

    void setTitle(const QString &title);
    QString title() const;
    void setSubTitle(const QString &subTitle);
    QString subTitle() const;
    void setPixmap(QWizard::WizardPixmap which, const QPixmap &pixmap);
    QPixmap pixmap(QWizard::WizardPixmap which) const;
    void setFinalPage(bool finalPage);
    bool isFinalPage() const;
    void setCommitPage(bool commitPage);
    bool isCommitPage() const;
    void setButtonText(QWizard::WizardButton which, const QString &text);
    QString buttonText(QWizard::WizardButton which) const;

    virtual void initializePage();
    virtual void cleanupPage();
    virtual bool validatePage();
    virtual bool isComplete() const;
    virtual int nextId() const;

Q_SIGNALS:
    void completeChanged();

protected:
    void setField(const QString &name, const QVariant &value);
    QVariant field(const QString &name) const;
    void registerField(const QString &name, QWidget *widget, const char *property = 0,
                       const char *changedSignal = 0);
    QWizard *wizard() const;

private:
    Q_DISABLE_COPY(QWizardPage)
    Q_DECLARE_PRIVATE(QWizardPage)
    Q_PRIVATE_SLOT(d_func(), void _q_maybeEmitCompleteChanged())
    Q_PRIVATE_SLOT(d_func(), void _q_updateCachedCompleteState())

    friend class QWizard;
    friend class QWizardPrivate;
};

QT_END_NAMESPACE

#endif // QT_NO_WIZARD

#endif // QWIZARD_H
