/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include <qplatformdefs.h>
#include "qitemeditorfactory.h"
#include "qitemeditorfactory_p.h"

#ifndef QT_NO_ITEMVIEWS

#include <qcombobox.h>
#include <qdatetimeedit.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qspinbox.h>
#include <limits.h>
#include <float.h>
#include <qapplication.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE


#ifndef QT_NO_COMBOBOX

class QBooleanComboBox : public QComboBox
{
    Q_OBJECT
    Q_PROPERTY(bool value READ value WRITE setValue USER true)

public:
    QBooleanComboBox(QWidget *parent);
    void setValue(bool);
    bool value() const;
};

#endif // QT_NO_COMBOBOX

/*!
    \class QItemEditorFactory
    \brief The QItemEditorFactory class provides widgets for editing item data
    in views and delegates.
    \since 4.2
    \ingroup model-view

    When editing data in an item view, editors are created and
    displayed by a delegate. QItemDelegate, which is the delegate by
    default installed on Qt's item views, uses a QItemEditorFactory to
    create editors for it. A default unique instance provided by
    QItemEditorFactory is used by all item delegates.  If you set a
    new default factory with setDefaultFactory(), the new factory will
    be used by existing and new delegates.

    A factory keeps a collection of QItemEditorCreatorBase
    instances, which are specialized editors that produce editors
    for one particular QVariant data type (All Qt models store
    their data in \l{QVariant}s).

    \section1 Standard Editing Widgets

    The standard factory implementation provides editors for a variety of data
    types. These are created whenever a delegate needs to provide an editor for
    data supplied by a model. The following table shows the relationship between
    types and the standard editors provided.

    \table
    \header \o Type \o Editor Widget
    \row    \o bool \o QComboBox
    \row    \o double \o QDoubleSpinBox
    \row    \o int \o{1,2} QSpinBox
    \row    \o unsigned int
    \row    \o QDate \o QDateEdit
    \row    \o QDateTime \o QDateTimeEdit
    \row    \o QPixmap \o QLabel
    \row    \o QString \o QLineEdit
    \row    \o QTime \o QTimeEdit
    \endtable

    Additional editors can be registered with the registerEditor() function.

    \sa QItemDelegate, {Model/View Programming}, {Color Editor Factory Example}
*/

/*!
    \fn QItemEditorFactory::QItemEditorFactory()

    Constructs a new item editor factory.
*/

/*!
    Creates an editor widget with the given \a parent for the specified \a type of data,
    and returns it as a QWidget.

    \sa registerEditor()
*/
QWidget *QItemEditorFactory::createEditor(QVariant::Type type, QWidget *parent) const
{
    QItemEditorCreatorBase *creator = creatorMap.value(type, 0);
    if (!creator) {
        const QItemEditorFactory *dfactory = defaultFactory();
        return dfactory == this ? 0 : dfactory->createEditor(type, parent);
    }
    return creator->createWidget(parent);
}

/*!
    Returns the property name used to access data for the given \a type of data.
*/
QByteArray QItemEditorFactory::valuePropertyName(QVariant::Type type) const
{
    QItemEditorCreatorBase *creator = creatorMap.value(type, 0);
    if (!creator) {
        const QItemEditorFactory *dfactory = defaultFactory();
        return dfactory == this ? QByteArray() : dfactory->valuePropertyName(type);
    }
    return creator->valuePropertyName();
}

/*!
    Destroys the item editor factory.
*/
QItemEditorFactory::~QItemEditorFactory()
{
    //we make sure we delete all the QItemEditorCreatorBase
    //this has to be done only once, hence the QSet
    QSet<QItemEditorCreatorBase*> set = creatorMap.values().toSet();
    qDeleteAll(set);
}

/*!
    Registers an item editor creator specified by \a creator for the given \a type of data.

    \bold{Note:} The factory takes ownership of the item editor creator and will destroy
    it if a new creator for the same type is registered later.

    \sa createEditor()
*/
void QItemEditorFactory::registerEditor(QVariant::Type type, QItemEditorCreatorBase *creator)
{
    QHash<QVariant::Type, QItemEditorCreatorBase *>::iterator it = creatorMap.find(type);
    if (it != creatorMap.end()) {
        QItemEditorCreatorBase *oldCreator = it.value();
        Q_ASSERT(oldCreator);
        creatorMap.erase(it);
        if (!creatorMap.values().contains(oldCreator))
            delete oldCreator; // if it is no more in use we can delete it
    }

    creatorMap[type] = creator;
}

class QDefaultItemEditorFactory : public QItemEditorFactory
{
public:
    inline QDefaultItemEditorFactory() {}
    QWidget *createEditor(QVariant::Type type, QWidget *parent) const;
    QByteArray valuePropertyName(QVariant::Type) const;
};

QWidget *QDefaultItemEditorFactory::createEditor(QVariant::Type type, QWidget *parent) const
{
    switch (type) {
#ifndef QT_NO_COMBOBOX
    case QVariant::Bool: {
        QBooleanComboBox *cb = new QBooleanComboBox(parent);
        cb->setFrame(false);
        return cb; }
#endif
#ifndef QT_NO_SPINBOX
    case QVariant::UInt: {
        QSpinBox *sb = new QSpinBox(parent);
        sb->setFrame(false);
        sb->setMaximum(INT_MAX);
        return sb; }
    case QVariant::Int: {
        QSpinBox *sb = new QSpinBox(parent);
        sb->setFrame(false);
        sb->setMinimum(INT_MIN);
        sb->setMaximum(INT_MAX);
        return sb; }
#endif
#ifndef QT_NO_DATETIMEEDIT
    case QVariant::Date: {
        QDateTimeEdit *ed = new QDateEdit(parent);
        ed->setFrame(false);
        return ed; }
    case QVariant::Time: {
        QDateTimeEdit *ed = new QTimeEdit(parent);
        ed->setFrame(false);
        return ed; }
    case QVariant::DateTime: {
        QDateTimeEdit *ed = new QDateTimeEdit(parent);
        ed->setFrame(false);
        return ed; }
#endif
    case QVariant::Pixmap:
        return new QLabel(parent);
#ifndef QT_NO_SPINBOX
    case QVariant::Double: {
        QDoubleSpinBox *sb = new QDoubleSpinBox(parent);
        sb->setFrame(false);
        sb->setMinimum(-DBL_MAX);
        sb->setMaximum(DBL_MAX);
        return sb; }
#endif
#ifndef QT_NO_LINEEDIT
    case QVariant::String:
    default: {
        // the default editor is a lineedit
        QExpandingLineEdit *le = new QExpandingLineEdit(parent);
        le->setFrame(le->style()->styleHint(QStyle::SH_ItemView_DrawDelegateFrame, 0, le));
        if (!le->style()->styleHint(QStyle::SH_ItemView_ShowDecorationSelected, 0, le))
            le->setWidgetOwnsGeometry(true);
        return le; }
#else
    default:
        break;
#endif
    }
    return 0;
}

QByteArray QDefaultItemEditorFactory::valuePropertyName(QVariant::Type type) const
{
    switch (type) {
#ifndef QT_NO_COMBOBOX
    case QVariant::Bool:
        return "currentIndex";
#endif
#ifndef QT_NO_SPINBOX
    case QVariant::UInt:
    case QVariant::Int:
    case QVariant::Double:
        return "value";
#endif
#ifndef QT_NO_DATETIMEEDIT
    case QVariant::Date:
        return "date";
    case QVariant::Time:
        return "time";
    case QVariant::DateTime:
        return "dateTime";
#endif
    case QVariant::String:
    default:
        // the default editor is a lineedit
        return "text";
    }
}

static QItemEditorFactory *q_default_factory = 0;
struct QDefaultFactoryCleaner
{
    inline QDefaultFactoryCleaner() {}
    ~QDefaultFactoryCleaner() { delete q_default_factory; q_default_factory = 0; }
};

/*!
    Returns the default item editor factory.

    \sa setDefaultFactory()
*/
const QItemEditorFactory *QItemEditorFactory::defaultFactory()
{
    static const QDefaultItemEditorFactory factory;
    if (q_default_factory)
        return q_default_factory;
    return &factory;
}

/*!
    Sets the default item editor factory to the given \a factory.
    Both new and existing delegates will use the new factory.

    \sa defaultFactory()
*/
void QItemEditorFactory::setDefaultFactory(QItemEditorFactory *factory)
{
    static const QDefaultFactoryCleaner cleaner;
    delete q_default_factory;
    q_default_factory = factory;
}

/*!
    \class QItemEditorCreatorBase
    \brief The QItemEditorCreatorBase class provides an abstract base class that
    must be subclassed when implementing new item editor creators.
    \since 4.2
    \ingroup model-view

    QItemEditorCreatorBase objects are specialized widget factories that
    provide editor widgets for one particular QVariant data type. They
    are used by QItemEditorFactory to create editors for
    \l{QItemDelegate}s. Creator bases must be registered with
    QItemEditorFactory::registerEditor().

    An editor should provide a user property for the data it edits.
    QItemDelagates can then access the property using Qt's
    \l{Meta-Object System}{meta-object system} to set and retrieve the
    editing data. A property is set as the user property with the USER
    keyword:

    \snippet doc/src/snippets/code/src_gui_itemviews_qitemeditorfactory.cpp 0

    If the editor does not provide a user property, it must return the
    name of the property from valuePropertyName(); delegates will then
    use the name to access the property. If a user property exists,
    item delegates will not call valuePropertyName().

    QStandardItemEditorCreator is a convenience template class that can be used
    to register widgets without the need to subclass QItemEditorCreatorBase.

    \sa QStandardItemEditorCreator, QItemEditorFactory,
    {Model/View Programming}, {Color Editor Factory Example}
*/

/*!
    \fn QItemEditorCreatorBase::~QItemEditorCreatorBase()

    Destroys the editor creator object.
*/

/*!
    \fn QWidget *QItemEditorCreatorBase::createWidget(QWidget *parent) const

    Returns an editor widget with the given \a parent.

    When implementing this function in subclasses of this class, you must
    construct and return new editor widgets with the parent widget specified.
*/

/*!
    \fn QByteArray QItemEditorCreatorBase::valuePropertyName() const

    Returns the name of the property used to get and set values in the creator's
    editor widgets.

    When implementing this function in subclasses, you must ensure that the
    editor widget's property specified by this function can accept the type
    the creator is registered for. For example, a creator which constructs
    QCheckBox widgets to edit boolean values would return the
    \l{QCheckBox::checkable}{checkable} property name from this function,
    and must be registered in the item editor factory for the QVariant::Bool
    type.

    Note: Since Qt 4.2 the item delegates query the user property of widgets,
    and only call this function if the widget has no user property. You can
    override this behavior by reimplementing QAbstractItemDelegate::setModelData()
    and QAbstractItemDelegate::setEditorData().

    \sa QMetaObject::userProperty(), QItemEditorFactory::registerEditor()
*/

/*!
    \class QItemEditorCreator
    \brief The QItemEditorCreator class makes it possible to create
	   item editor creator bases without subclassing
	   QItemEditorCreatorBase.

    \since 4.2
    \ingroup model-view

    QItemEditorCreator is a convenience template class. It uses
    the template class to create editors for QItemEditorFactory.
    This way, it is not necessary to subclass
    QItemEditorCreatorBase.

    \snippet doc/src/snippets/code/src_gui_itemviews_qitemeditorfactory.cpp 1

    The constructor takes the name of the property that contains the
    editing data. QItemDelegate can then access the property by name
    when it sets and retrieves editing data. Only use this class if
    your editor does not define a user property (using the USER
    keyword in the Q_PROPERTY macro).  If the widget has a user
    property, you should use QStandardItemEditorCreator instead.

    \sa QItemEditorCreatorBase, QStandardItemEditorCreator,
	QItemEditorFactory, {Color Editor Factory Example}
*/

/*!
    \fn QItemEditorCreator::QItemEditorCreator(const QByteArray &valuePropertyName)

    Constructs an editor creator object using \a valuePropertyName
    as the name of the property to be used for editing. The
    property name is used by QItemDelegate when setting and
    getting editor data.

    Note that the \a valuePropertyName is only used if the editor
    widget does not have a user property defined.
*/

/*!
    \fn QWidget *QItemEditorCreator::createWidget(QWidget *parent) const
    \reimp
*/

/*!
    \fn QByteArray QItemEditorCreator::valuePropertyName() const
    \reimp
*/

/*!
    \class QStandardItemEditorCreator

    \brief The QStandardItemEditorCreator class provides the
    possibility to register widgets without having to subclass
    QItemEditorCreatorBase.

    \since 4.2
    \ingroup model-view

    This convenience template class makes it possible to register widgets without
    having to subclass QItemEditorCreatorBase.

    Example:

    \snippet doc/src/snippets/code/src_gui_itemviews_qitemeditorfactory.cpp 2

    Setting the \c editorFactory created above in an item delegate via
    QItemDelegate::setItemEditorFactory() makes sure that all values of type
    QVariant::DateTime will be edited in \c{MyFancyDateTimeEdit}.

    The editor must provide a user property that will contain the
    editing data. The property is used by \l{QItemDelegate}s to set
    and retrieve the data (using Qt's \l{Meta-Object
    System}{meta-object system}). You set the user property with
    the USER keyword:

    \snippet doc/src/snippets/code/src_gui_itemviews_qitemeditorfactory.cpp 3

    \sa QItemEditorCreatorBase, QItemEditorCreator,
	QItemEditorFactory, QItemDelegate, {Color Editor Factory Example}
*/

/*!
    \fn QStandardItemEditorCreator::QStandardItemEditorCreator()

    Constructs an editor creator object.
*/

/*!
    \fn QWidget *QStandardItemEditorCreator::createWidget(QWidget *parent) const
    \reimp
*/

/*!
    \fn QByteArray QStandardItemEditorCreator::valuePropertyName() const
    \reimp
*/

#ifndef QT_NO_LINEEDIT

QExpandingLineEdit::QExpandingLineEdit(QWidget *parent)
    : QLineEdit(parent), originalWidth(-1), widgetOwnsGeometry(false)
{
    connect(this, SIGNAL(textChanged(QString)), this, SLOT(resizeToContents()));
    updateMinimumWidth();
}

void QExpandingLineEdit::changeEvent(QEvent *e)
{
    switch (e->type())
    {
    case QEvent::FontChange:
    case QEvent::StyleChange:
    case QEvent::ContentsRectChange:
        updateMinimumWidth();
        break;
    default:
        break;
    }

    QLineEdit::changeEvent(e);
}

void QExpandingLineEdit::updateMinimumWidth()
{
    int left, right;
    getTextMargins(&left, 0, &right, 0);
    int width = left + right + 4 /*horizontalMargin in qlineedit.cpp*/;
    getContentsMargins(&left, 0, &right, 0);
    width += left + right;

    QStyleOptionFrameV2 opt;
    initStyleOption(&opt);
    
    int minWidth = style()->sizeFromContents(QStyle::CT_LineEdit, &opt, QSize(width, 0).
                                      expandedTo(QApplication::globalStrut()), this).width();
    setMinimumWidth(minWidth);
}

void QExpandingLineEdit::resizeToContents()
{
    int oldWidth = width();
    if (originalWidth == -1)
        originalWidth = oldWidth;
    if (QWidget *parent = parentWidget()) {
        QPoint position = pos();
        int hintWidth = minimumWidth() + fontMetrics().width(displayText());
        int parentWidth = parent->width();
        int maxWidth = isRightToLeft() ? position.x() + oldWidth : parentWidth - position.x();
        int newWidth = qBound(originalWidth, hintWidth, maxWidth);
        if (widgetOwnsGeometry)
            setMaximumWidth(newWidth);
        if (isRightToLeft())
            move(position.x() - newWidth + oldWidth, position.y());
        resize(newWidth, height());
    }
}

#endif // QT_NO_LINEEDIT

#ifndef QT_NO_COMBOBOX

QBooleanComboBox::QBooleanComboBox(QWidget *parent)
    : QComboBox(parent)
{
    addItem(QComboBox::tr("False"));
    addItem(QComboBox::tr("True"));
}

void QBooleanComboBox::setValue(bool value)
{
    setCurrentIndex(value ? 1 : 0);
}

bool QBooleanComboBox::value() const
{
    return (currentIndex() == 1);
}

#endif // QT_NO_COMBOBOX

QT_END_NAMESPACE

#if !defined(QT_NO_LINEEDIT) || !defined(QT_NO_COMBOBOX)
#include "qitemeditorfactory.moc"
#endif

#endif // QT_NO_ITEMVIEWS
