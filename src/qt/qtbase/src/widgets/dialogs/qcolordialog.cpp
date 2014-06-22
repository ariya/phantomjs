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

#include "qcolordialog_p.h"

#ifndef QT_NO_COLORDIALOG

#include "qapplication.h"
#include "qdesktopwidget.h"
#include "qdrawutil.h"
#include "qevent.h"
#include "qimage.h"
#include "qdrag.h"
#include "qlabel.h"
#include "qlayout.h"
#include "qlineedit.h"
#include "qmenu.h"
#include "qpainter.h"
#include "qpixmap.h"
#include "qpushbutton.h"
#include "qsettings.h"
#include "qstyle.h"
#include "qstyleoption.h"
#include "qvalidator.h"
#include "qmimedata.h"
#include "qspinbox.h"
#include "qdialogbuttonbox.h"
#include "qscreen.h"
#include "qcursor.h"

#include <algorithm>

QT_BEGIN_NAMESPACE

//////////// QWellArray BEGIN

struct QWellArrayData;

class QWellArray : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int selectedColumn READ selectedColumn)
    Q_PROPERTY(int selectedRow READ selectedRow)

public:
    QWellArray(int rows, int cols, QWidget* parent=0);
    ~QWellArray() {}
    QString cellContent(int row, int col) const;

    int selectedColumn() const { return selCol; }
    int selectedRow() const { return selRow; }

    virtual void setCurrent(int row, int col);
    virtual void setSelected(int row, int col);

    QSize sizeHint() const;

    virtual void setCellBrush(int row, int col, const QBrush &);
    QBrush cellBrush(int row, int col);

    inline int cellWidth() const
        { return cellw; }

    inline int cellHeight() const
        { return cellh; }

    inline int rowAt(int y) const
        { return y / cellh; }

    inline int columnAt(int x) const
        { if (isRightToLeft()) return ncols - (x / cellw) - 1; return x / cellw; }

    inline int rowY(int row) const
        { return cellh * row; }

    inline int columnX(int column) const
        { if (isRightToLeft()) return cellw * (ncols - column - 1); return cellw * column; }

    inline int numRows() const
        { return nrows; }

    inline int numCols() const
        {return ncols; }

    inline QRect cellRect() const
        { return QRect(0, 0, cellw, cellh); }

    inline QSize gridSize() const
        { return QSize(ncols * cellw, nrows * cellh); }

    QRect cellGeometry(int row, int column)
        {
            QRect r;
            if (row >= 0 && row < nrows && column >= 0 && column < ncols)
                r.setRect(columnX(column), rowY(row), cellw, cellh);
            return r;
        }

    inline void updateCell(int row, int column) { update(cellGeometry(row, column)); }

signals:
    void selected(int row, int col);
    void currentChanged(int row, int col);

protected:
    virtual void paintCell(QPainter *, int row, int col, const QRect&);
    virtual void paintCellContents(QPainter *, int row, int col, const QRect&);

    void mousePressEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);
    void keyPressEvent(QKeyEvent*);
    void focusInEvent(QFocusEvent*);
    void focusOutEvent(QFocusEvent*);
    void paintEvent(QPaintEvent *);

private:
    Q_DISABLE_COPY(QWellArray)

    int nrows;
    int ncols;
    int cellw;
    int cellh;
    int curRow;
    int curCol;
    int selRow;
    int selCol;
    QWellArrayData *d;
};

void QWellArray::paintEvent(QPaintEvent *e)
{
    QRect r = e->rect();
    int cx = r.x();
    int cy = r.y();
    int ch = r.height();
    int cw = r.width();
    int colfirst = columnAt(cx);
    int collast = columnAt(cx + cw);
    int rowfirst = rowAt(cy);
    int rowlast = rowAt(cy + ch);

    if (isRightToLeft()) {
        int t = colfirst;
        colfirst = collast;
        collast = t;
    }

    QPainter painter(this);
    QPainter *p = &painter;
    QRect rect(0, 0, cellWidth(), cellHeight());


    if (collast < 0 || collast >= ncols)
        collast = ncols-1;
    if (rowlast < 0 || rowlast >= nrows)
        rowlast = nrows-1;

    // Go through the rows
    for (int r = rowfirst; r <= rowlast; ++r) {
        // get row position and height
        int rowp = rowY(r);

        // Go through the columns in the row r
        // if we know from where to where, go through [colfirst, collast],
        // else go through all of them
        for (int c = colfirst; c <= collast; ++c) {
            // get position and width of column c
            int colp = columnX(c);
            // Translate painter and draw the cell
            rect.translate(colp, rowp);
            paintCell(p, r, c, rect);
            rect.translate(-colp, -rowp);
        }
    }
}

struct QWellArrayData {
    QBrush *brush;
};

QWellArray::QWellArray(int rows, int cols, QWidget *parent)
    : QWidget(parent)
        ,nrows(rows), ncols(cols)
{
    d = 0;
    setFocusPolicy(Qt::StrongFocus);
    cellw = 28;
    cellh = 24;
    curCol = 0;
    curRow = 0;
    selCol = -1;
    selRow = -1;
}

QSize QWellArray::sizeHint() const
{
    ensurePolished();
    return gridSize().boundedTo(QSize(640, 480));
}


void QWellArray::paintCell(QPainter* p, int row, int col, const QRect &rect)
{
    int b = 3; //margin

    const QPalette & g = palette();
    QStyleOptionFrame opt;
    int dfw = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    opt.lineWidth = dfw;
    opt.midLineWidth = 1;
    opt.rect = rect.adjusted(b, b, -b, -b);
    opt.palette = g;
    opt.state = QStyle::State_Enabled | QStyle::State_Sunken;
    style()->drawPrimitive(QStyle::PE_Frame, &opt, p, this);
    b += dfw;

    if ((row == curRow) && (col == curCol)) {
        if (hasFocus()) {
            QStyleOptionFocusRect opt;
            opt.palette = g;
            opt.rect = rect;
            opt.state = QStyle::State_None | QStyle::State_KeyboardFocusChange;
            style()->drawPrimitive(QStyle::PE_FrameFocusRect, &opt, p, this);
        }
    }
    paintCellContents(p, row, col, opt.rect.adjusted(dfw, dfw, -dfw, -dfw));
}

/*!
  Reimplement this function to change the contents of the well array.
 */
void QWellArray::paintCellContents(QPainter *p, int row, int col, const QRect &r)
{
    if (d) {
        p->fillRect(r, d->brush[row*numCols()+col]);
    } else {
        p->fillRect(r, Qt::white);
        p->setPen(Qt::black);
        p->drawLine(r.topLeft(), r.bottomRight());
        p->drawLine(r.topRight(), r.bottomLeft());
    }
}

void QWellArray::mousePressEvent(QMouseEvent *e)
{
    // The current cell marker is set to the cell the mouse is pressed in
    QPoint pos = e->pos();
    setCurrent(rowAt(pos.y()), columnAt(pos.x()));
}

void QWellArray::mouseReleaseEvent(QMouseEvent * /* event */)
{
    // The current cell marker is set to the cell the mouse is clicked in
    setSelected(curRow, curCol);
}


/*
  Sets the cell currently having the focus. This is not necessarily
  the same as the currently selected cell.
*/

void QWellArray::setCurrent(int row, int col)
{
    if ((curRow == row) && (curCol == col))
        return;

    if (row < 0 || col < 0)
        row = col = -1;

    int oldRow = curRow;
    int oldCol = curCol;

    curRow = row;
    curCol = col;

    updateCell(oldRow, oldCol);
    updateCell(curRow, curCol);

    emit currentChanged(curRow, curCol);
}

/*
  Sets the currently selected cell to \a row, \a column. If \a row or
  \a column are less than zero, the current cell is unselected.

  Does not set the position of the focus indicator.
*/
void QWellArray::setSelected(int row, int col)
{
    int oldRow = selRow;
    int oldCol = selCol;

    if (row < 0 || col < 0)
        row = col = -1;

    selCol = col;
    selRow = row;

    updateCell(oldRow, oldCol);
    updateCell(selRow, selCol);
    if (row >= 0)
        emit selected(row, col);

#ifndef QT_NO_MENU
    if (isVisible() && qobject_cast<QMenu*>(parentWidget()))
        parentWidget()->close();
#endif
}

void QWellArray::focusInEvent(QFocusEvent*)
{
    updateCell(curRow, curCol);
    emit currentChanged(curRow, curCol);
}

void QWellArray::setCellBrush(int row, int col, const QBrush &b)
{
    if (!d) {
        d = new QWellArrayData;
        int i = numRows()*numCols();
        d->brush = new QBrush[i];
    }
    if (row >= 0 && row < numRows() && col >= 0 && col < numCols())
        d->brush[row*numCols()+col] = b;
}

/*
  Returns the brush set for the cell at \a row, \a column. If no brush is
  set, Qt::NoBrush is returned.
*/

QBrush QWellArray::cellBrush(int row, int col)
{
    if (d && row >= 0 && row < numRows() && col >= 0 && col < numCols())
        return d->brush[row*numCols()+col];
    return Qt::NoBrush;
}



/*!\reimp
*/

void QWellArray::focusOutEvent(QFocusEvent*)
{
    updateCell(curRow, curCol);
}

/*\reimp
*/
void QWellArray::keyPressEvent(QKeyEvent* e)
{
    switch(e->key()) {                        // Look at the key code
    case Qt::Key_Left:                                // If 'left arrow'-key,
        if(curCol > 0)                        // and cr't not in leftmost col
            setCurrent(curRow, curCol - 1);        // set cr't to next left column
        break;
    case Qt::Key_Right:                                // Correspondingly...
        if(curCol < numCols()-1)
            setCurrent(curRow, curCol + 1);
        break;
    case Qt::Key_Up:
        if(curRow > 0)
            setCurrent(curRow - 1, curCol);
        break;
    case Qt::Key_Down:
        if(curRow < numRows()-1)
            setCurrent(curRow + 1, curCol);
        break;
#if 0
    // bad idea that shouldn't have been implemented; very counterintuitive
    case Qt::Key_Return:
    case Qt::Key_Enter:
        /*
          ignore the key, so that the dialog get it, but still select
          the current row/col
        */
        e->ignore();
        // fallthrough intended
#endif
    case Qt::Key_Space:
        setSelected(curRow, curCol);
        break;
    default:                                // If not an interesting key,
        e->ignore();                        // we don't accept the event
        return;
    }

}

//////////// QWellArray END

// Event filter to be installed on the dialog while in color-picking mode.
class QColorPickingEventFilter : public QObject {
public:
    explicit QColorPickingEventFilter(QColorDialogPrivate *dp, QObject *parent = 0) : QObject(parent), m_dp(dp) {}

    bool eventFilter(QObject *, QEvent *event) Q_DECL_OVERRIDE
    {
        switch (event->type()) {
        case QEvent::MouseMove:
            return m_dp->handleColorPickingMouseMove(static_cast<QMouseEvent *>(event));
        case QEvent::MouseButtonRelease:
            return m_dp->handleColorPickingMouseButtonRelease(static_cast<QMouseEvent *>(event));
        case QEvent::KeyPress:
            return m_dp->handleColorPickingKeyPress(static_cast<QKeyEvent *>(event));
        default:
            break;
        }
        return false;
    }

private:
    QColorDialogPrivate *m_dp;
};

/*!
    Returns the number of custom colors supported by QColorDialog. All
    color dialogs share the same custom colors.
*/
int QColorDialog::customCount()
{
    return QColorDialogOptions::customColorCount();
}

/*!
    \since 4.5

    Returns the custom color at the given \a index as a QColor value.
*/
QColor QColorDialog::customColor(int index)
{
    return QColor(QColorDialogOptions::customColor(index));
}

/*!
    Sets the custom color at \a index to the QColor \a color value.

    \note This function does not apply to the Native Color Dialog on the Mac
    OS X platform. If you still require this function, use the
    QColorDialog::DontUseNativeDialog option.
*/
void QColorDialog::setCustomColor(int index, QColor color)
{
    QColorDialogOptions::setCustomColor(index, color.rgba());
}

/*!
    \since 5.0

    Returns the standard color at the given \a index as a QColor value.
*/
QColor QColorDialog::standardColor(int index)
{
    return QColor(QColorDialogOptions::standardColor(index));
}

/*!
    Sets the standard color at \a index to the QColor \a color value.

    \note This function does not apply to the Native Color Dialog on the Mac
    OS X platform. If you still require this function, use the
    QColorDialog::DontUseNativeDialog option.
*/
void QColorDialog::setStandardColor(int index, QColor color)
{
    QColorDialogOptions::setStandardColor(index, color.rgba());
}

static inline void rgb2hsv(QRgb rgb, int &h, int &s, int &v)
{
    QColor c;
    c.setRgb(rgb);
    c.getHsv(&h, &s, &v);
}

class QColorWell : public QWellArray
{
public:
    QColorWell(QWidget *parent, int r, int c, QRgb *vals)
        :QWellArray(r, c, parent), values(vals), mousePressed(false), oldCurrent(-1, -1)
    { setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum)); }

protected:
    void paintCellContents(QPainter *, int row, int col, const QRect&);
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
#ifndef QT_NO_DRAGANDDROP
    void dragEnterEvent(QDragEnterEvent *e);
    void dragLeaveEvent(QDragLeaveEvent *e);
    void dragMoveEvent(QDragMoveEvent *e);
    void dropEvent(QDropEvent *e);
#endif

private:
    QRgb *values;
    bool mousePressed;
    QPoint pressPos;
    QPoint oldCurrent;

};

void QColorWell::paintCellContents(QPainter *p, int row, int col, const QRect &r)
{
    int i = row + col*numRows();
    p->fillRect(r, QColor(values[i]));
}

void QColorWell::mousePressEvent(QMouseEvent *e)
{
    oldCurrent = QPoint(selectedRow(), selectedColumn());
    QWellArray::mousePressEvent(e);
    mousePressed = true;
    pressPos = e->pos();
}

void QColorWell::mouseMoveEvent(QMouseEvent *e)
{
    QWellArray::mouseMoveEvent(e);
#ifndef QT_NO_DRAGANDDROP
    if (!mousePressed)
        return;
    if ((pressPos - e->pos()).manhattanLength() > QApplication::startDragDistance()) {
        setCurrent(oldCurrent.x(), oldCurrent.y());
        int i = rowAt(pressPos.y()) + columnAt(pressPos.x()) * numRows();
        QColor col(values[i]);
        QMimeData *mime = new QMimeData;
        mime->setColorData(col);
        QPixmap pix(cellWidth(), cellHeight());
        pix.fill(col);
        QPainter p(&pix);
        p.drawRect(0, 0, pix.width() - 1, pix.height() - 1);
        p.end();
        QDrag *drg = new QDrag(this);
        drg->setMimeData(mime);
        drg->setPixmap(pix);
        mousePressed = false;
        drg->start();
    }
#endif
}

#ifndef QT_NO_DRAGANDDROP
void QColorWell::dragEnterEvent(QDragEnterEvent *e)
{
    if (qvariant_cast<QColor>(e->mimeData()->colorData()).isValid())
        e->accept();
    else
        e->ignore();
}

void QColorWell::dragLeaveEvent(QDragLeaveEvent *)
{
    if (hasFocus())
        parentWidget()->setFocus();
}

void QColorWell::dragMoveEvent(QDragMoveEvent *e)
{
    if (qvariant_cast<QColor>(e->mimeData()->colorData()).isValid()) {
        setCurrent(rowAt(e->pos().y()), columnAt(e->pos().x()));
        e->accept();
    } else {
        e->ignore();
    }
}

void QColorWell::dropEvent(QDropEvent *e)
{
    QColor col = qvariant_cast<QColor>(e->mimeData()->colorData());
    if (col.isValid()) {
        int i = rowAt(e->pos().y()) + columnAt(e->pos().x()) * numRows();
        values[i] = col.rgb();
        update();
        e->accept();
    } else {
        e->ignore();
    }
}

#endif // QT_NO_DRAGANDDROP

void QColorWell::mouseReleaseEvent(QMouseEvent *e)
{
    if (!mousePressed)
        return;
    QWellArray::mouseReleaseEvent(e);
    mousePressed = false;
}

class QColorPicker : public QFrame
{
    Q_OBJECT
public:
    QColorPicker(QWidget* parent);
    ~QColorPicker();

public slots:
    void setCol(int h, int s);

signals:
    void newCol(int h, int s);

protected:
    QSize sizeHint() const;
    void paintEvent(QPaintEvent*);
    void mouseMoveEvent(QMouseEvent *);
    void mousePressEvent(QMouseEvent *);
    void resizeEvent(QResizeEvent *);

private:
    int hue;
    int sat;

    QPoint colPt();
    int huePt(const QPoint &pt);
    int satPt(const QPoint &pt);
    void setCol(const QPoint &pt);

    QPixmap pix;
};

static int pWidth = 220;
static int pHeight = 200;

class QColorLuminancePicker : public QWidget
{
    Q_OBJECT
public:
    QColorLuminancePicker(QWidget* parent=0);
    ~QColorLuminancePicker();

public slots:
    void setCol(int h, int s, int v);
    void setCol(int h, int s);

signals:
    void newHsv(int h, int s, int v);

protected:
    void paintEvent(QPaintEvent*);
    void mouseMoveEvent(QMouseEvent *);
    void mousePressEvent(QMouseEvent *);

private:
    enum { foff = 3, coff = 4 }; //frame and contents offset
    int val;
    int hue;
    int sat;

    int y2val(int y);
    int val2y(int val);
    void setVal(int v);

    QPixmap *pix;
};


int QColorLuminancePicker::y2val(int y)
{
    int d = height() - 2*coff - 1;
    return 255 - (y - coff)*255/d;
}

int QColorLuminancePicker::val2y(int v)
{
    int d = height() - 2*coff - 1;
    return coff + (255-v)*d/255;
}

QColorLuminancePicker::QColorLuminancePicker(QWidget* parent)
    :QWidget(parent)
{
    hue = 100; val = 100; sat = 100;
    pix = 0;
    //    setAttribute(WA_NoErase, true);
}

QColorLuminancePicker::~QColorLuminancePicker()
{
    delete pix;
}

void QColorLuminancePicker::mouseMoveEvent(QMouseEvent *m)
{
    setVal(y2val(m->y()));
}
void QColorLuminancePicker::mousePressEvent(QMouseEvent *m)
{
    setVal(y2val(m->y()));
}

void QColorLuminancePicker::setVal(int v)
{
    if (val == v)
        return;
    val = qMax(0, qMin(v,255));
    delete pix; pix=0;
    repaint();
    emit newHsv(hue, sat, val);
}

//receives from a hue,sat chooser and relays.
void QColorLuminancePicker::setCol(int h, int s)
{
    setCol(h, s, val);
    emit newHsv(h, s, val);
}

void QColorLuminancePicker::paintEvent(QPaintEvent *)
{
    int w = width() - 5;

    QRect r(0, foff, w, height() - 2*foff);
    int wi = r.width() - 2;
    int hi = r.height() - 2;
    if (!pix || pix->height() != hi || pix->width() != wi) {
        delete pix;
        QImage img(wi, hi, QImage::Format_RGB32);
        int y;
        uint *pixel = (uint *) img.scanLine(0);
        for (y = 0; y < hi; y++) {
            uint *end = pixel + wi;
            std::fill(pixel, end, QColor::fromHsv(hue, sat, y2val(y + coff)).rgb());
            pixel = end;
        }
        pix = new QPixmap(QPixmap::fromImage(img));
    }
    QPainter p(this);
    p.drawPixmap(1, coff, *pix);
    const QPalette &g = palette();
    qDrawShadePanel(&p, r, g, true);
    p.setPen(g.foreground().color());
    p.setBrush(g.foreground());
    QPolygon a;
    int y = val2y(val);
    a.setPoints(3, w, y, w+5, y+5, w+5, y-5);
    p.eraseRect(w, 0, 5, height());
    p.drawPolygon(a);
}

void QColorLuminancePicker::setCol(int h, int s , int v)
{
    val = v;
    hue = h;
    sat = s;
    delete pix; pix=0;
    repaint();
}

QPoint QColorPicker::colPt()
{
    QRect r = contentsRect();
    return QPoint((360 - hue) * (r.width() - 1) / 360, (255 - sat) * (r.height() - 1) / 255);
}

int QColorPicker::huePt(const QPoint &pt)
{
    QRect r = contentsRect();
    return 360 - pt.x() * 360 / (r.width() - 1);
}

int QColorPicker::satPt(const QPoint &pt)
{
    QRect r = contentsRect();
    return 255 - pt.y() * 255 / (r.height() - 1);
}

void QColorPicker::setCol(const QPoint &pt)
{
    setCol(huePt(pt), satPt(pt));
}

QColorPicker::QColorPicker(QWidget* parent)
    : QFrame(parent)
{
    hue = 0; sat = 0;
    setCol(150, 255);

    setAttribute(Qt::WA_NoSystemBackground);
    setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed) );
}

QColorPicker::~QColorPicker()
{
}

QSize QColorPicker::sizeHint() const
{
    return QSize(pWidth + 2*frameWidth(), pHeight + 2*frameWidth());
}

void QColorPicker::setCol(int h, int s)
{
    int nhue = qMin(qMax(0,h), 359);
    int nsat = qMin(qMax(0,s), 255);
    if (nhue == hue && nsat == sat)
        return;

    QRect r(colPt(), QSize(20,20));
    hue = nhue; sat = nsat;
    r = r.united(QRect(colPt(), QSize(20,20)));
    r.translate(contentsRect().x()-9, contentsRect().y()-9);
    //    update(r);
    repaint(r);
}

void QColorPicker::mouseMoveEvent(QMouseEvent *m)
{
    QPoint p = m->pos() - contentsRect().topLeft();
    setCol(p);
    emit newCol(hue, sat);
}

void QColorPicker::mousePressEvent(QMouseEvent *m)
{
    QPoint p = m->pos() - contentsRect().topLeft();
    setCol(p);
    emit newCol(hue, sat);
}

void QColorPicker::paintEvent(QPaintEvent* )
{
    QPainter p(this);
    drawFrame(&p);
    QRect r = contentsRect();

    p.drawPixmap(r.topLeft(), pix);
    QPoint pt = colPt() + r.topLeft();
    p.setPen(Qt::black);

    p.fillRect(pt.x()-9, pt.y(), 20, 2, Qt::black);
    p.fillRect(pt.x(), pt.y()-9, 2, 20, Qt::black);

}

void QColorPicker::resizeEvent(QResizeEvent *ev)
{
    QFrame::resizeEvent(ev);

    int w = width() - frameWidth() * 2;
    int h = height() - frameWidth() * 2;
    QImage img(w, h, QImage::Format_RGB32);
    int x, y;
    uint *pixel = (uint *) img.scanLine(0);
    for (y = 0; y < h; y++) {
        const uint *end = pixel + w;
        x = 0;
        while (pixel < end) {
            QPoint p(x, y);
            QColor c;
            c.setHsv(huePt(p), satPt(p), 200);
            *pixel = c.rgb();
            ++pixel;
            ++x;
        }
    }
    pix = QPixmap::fromImage(img);
}


class QColSpinBox : public QSpinBox
{
public:
    QColSpinBox(QWidget *parent)
        : QSpinBox(parent) { setRange(0, 255); }
    void setValue(int i) {
        const QSignalBlocker blocker(this);
        QSpinBox::setValue(i);
    }
};

class QColorShowLabel;

class QColorShower : public QWidget
{
    Q_OBJECT
public:
    QColorShower(QColorDialog *parent);

    //things that don't emit signals
    void setHsv(int h, int s, int v);

    int currentAlpha() const
    { return (colorDialog->options() & QColorDialog::ShowAlphaChannel) ? alphaEd->value() : 255; }
    void setCurrentAlpha(int a) { alphaEd->setValue(a); rgbEd(); }
    void showAlpha(bool b);
    bool isAlphaVisible() const;

    QRgb currentColor() const { return curCol; }
    QColor currentQColor() const { return curQColor; }
    void retranslateStrings();
    void updateQColor();

public slots:
    void setRgb(QRgb rgb);

signals:
    void newCol(QRgb rgb);
    void currentColorChanged(const QColor &color);

private slots:
    void rgbEd();
    void hsvEd();
    void htmlEd();

private:
    void showCurrentColor();
    int hue, sat, val;
    QRgb curCol;
    QColor curQColor;
    QLabel *lblHue;
    QLabel *lblSat;
    QLabel *lblVal;
    QLabel *lblRed;
    QLabel *lblGreen;
    QLabel *lblBlue;
    QLabel *lblHtml;
    QColSpinBox *hEd;
    QColSpinBox *sEd;
    QColSpinBox *vEd;
    QColSpinBox *rEd;
    QColSpinBox *gEd;
    QColSpinBox *bEd;
    QColSpinBox *alphaEd;
    QLabel *alphaLab;
    QLineEdit *htEd;
    QColorShowLabel *lab;
    bool rgbOriginal;
    QColorDialog *colorDialog;

    friend class QColorDialog;
    friend class QColorDialogPrivate;
};

class QColorShowLabel : public QFrame
{
    Q_OBJECT

public:
    QColorShowLabel(QWidget *parent) : QFrame(parent) {
        setFrameStyle(QFrame::Panel|QFrame::Sunken);
        setAcceptDrops(true);
        mousePressed = false;
    }
    void setColor(QColor c) { col = c; }

signals:
    void colorDropped(QRgb);

protected:
    void paintEvent(QPaintEvent *);
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
#ifndef QT_NO_DRAGANDDROP
    void dragEnterEvent(QDragEnterEvent *e);
    void dragLeaveEvent(QDragLeaveEvent *e);
    void dropEvent(QDropEvent *e);
#endif

private:
    QColor col;
    bool mousePressed;
    QPoint pressPos;
};

void QColorShowLabel::paintEvent(QPaintEvent *e)
{
    QPainter p(this);
    drawFrame(&p);
    p.fillRect(contentsRect()&e->rect(), col);
}

void QColorShower::showAlpha(bool b)
{
    alphaLab->setVisible(b);
    alphaEd->setVisible(b);
}

inline bool QColorShower::isAlphaVisible() const
{
    return alphaLab->isVisible();
}

void QColorShowLabel::mousePressEvent(QMouseEvent *e)
{
    mousePressed = true;
    pressPos = e->pos();
}

void QColorShowLabel::mouseMoveEvent(QMouseEvent *e)
{
#ifdef QT_NO_DRAGANDDROP
    Q_UNUSED(e);
#else
    if (!mousePressed)
        return;
    if ((pressPos - e->pos()).manhattanLength() > QApplication::startDragDistance()) {
        QMimeData *mime = new QMimeData;
        mime->setColorData(col);
        QPixmap pix(30, 20);
        pix.fill(col);
        QPainter p(&pix);
        p.drawRect(0, 0, pix.width() - 1, pix.height() - 1);
        p.end();
        QDrag *drg = new QDrag(this);
        drg->setMimeData(mime);
        drg->setPixmap(pix);
        mousePressed = false;
        drg->start();
    }
#endif
}

#ifndef QT_NO_DRAGANDDROP
void QColorShowLabel::dragEnterEvent(QDragEnterEvent *e)
{
    if (qvariant_cast<QColor>(e->mimeData()->colorData()).isValid())
        e->accept();
    else
        e->ignore();
}

void QColorShowLabel::dragLeaveEvent(QDragLeaveEvent *)
{
}

void QColorShowLabel::dropEvent(QDropEvent *e)
{
    QColor color = qvariant_cast<QColor>(e->mimeData()->colorData());
    if (color.isValid()) {
        col = color;
        repaint();
        emit colorDropped(col.rgb());
        e->accept();
    } else {
        e->ignore();
    }
}
#endif // QT_NO_DRAGANDDROP

void QColorShowLabel::mouseReleaseEvent(QMouseEvent *)
{
    if (!mousePressed)
        return;
    mousePressed = false;
}

QColorShower::QColorShower(QColorDialog *parent)
    : QWidget(parent)
{
    colorDialog = parent;

    curCol = qRgb(255, 255, 255);
    curQColor = Qt::white;

    QGridLayout *gl = new QGridLayout(this);
    gl->setMargin(gl->spacing());
    lab = new QColorShowLabel(this);

#ifndef Q_OS_WINCE
#ifdef QT_SMALL_COLORDIALOG
    lab->setMinimumHeight(60);
#endif
    lab->setMinimumWidth(60);
#else
    lab->setMinimumWidth(20);
#endif

// For QVGA screens only the comboboxes and color label are visible.
// For nHD screens only color and luminence pickers and color label are visible.
#if !defined(QT_SMALL_COLORDIALOG)
    gl->addWidget(lab, 0, 0, -1, 1);
#else
    if (nonTouchUI)
        gl->addWidget(lab, 0, 0, 1, -1);
    else
        gl->addWidget(lab, 0, 0, -1, 1);
#endif
    connect(lab, SIGNAL(colorDropped(QRgb)), this, SIGNAL(newCol(QRgb)));
    connect(lab, SIGNAL(colorDropped(QRgb)), this, SLOT(setRgb(QRgb)));

    hEd = new QColSpinBox(this);
    hEd->setRange(0, 359);
    lblHue = new QLabel(this);
#ifndef QT_NO_SHORTCUT
    lblHue->setBuddy(hEd);
#endif
    lblHue->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
#if !defined(QT_SMALL_COLORDIALOG)
    gl->addWidget(lblHue, 0, 1);
    gl->addWidget(hEd, 0, 2);
#else
    if (nonTouchUI) {
        gl->addWidget(lblHue, 1, 0);
        gl->addWidget(hEd, 2, 0);
    } else {
        lblHue->hide();
        hEd->hide();
    }
#endif

    sEd = new QColSpinBox(this);
    lblSat = new QLabel(this);
#ifndef QT_NO_SHORTCUT
    lblSat->setBuddy(sEd);
#endif
    lblSat->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
#if !defined(QT_SMALL_COLORDIALOG)
    gl->addWidget(lblSat, 1, 1);
    gl->addWidget(sEd, 1, 2);
#else
    if (nonTouchUI) {
        gl->addWidget(lblSat, 1, 1);
        gl->addWidget(sEd, 2, 1);
    } else {
        lblSat->hide();
        sEd->hide();
    }
#endif

    vEd = new QColSpinBox(this);
    lblVal = new QLabel(this);
#ifndef QT_NO_SHORTCUT
    lblVal->setBuddy(vEd);
#endif
    lblVal->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
#if !defined(QT_SMALL_COLORDIALOG)
    gl->addWidget(lblVal, 2, 1);
    gl->addWidget(vEd, 2, 2);
#else
    if (nonTouchUI) {
        gl->addWidget(lblVal, 1, 2);
        gl->addWidget(vEd, 2, 2);
    } else {
        lblVal->hide();
        vEd->hide();
    }
#endif

    rEd = new QColSpinBox(this);
    lblRed = new QLabel(this);
#ifndef QT_NO_SHORTCUT
    lblRed->setBuddy(rEd);
#endif
    lblRed->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
#if !defined(QT_SMALL_COLORDIALOG)
    gl->addWidget(lblRed, 0, 3);
    gl->addWidget(rEd, 0, 4);
#else
    if (nonTouchUI) {
        gl->addWidget(lblRed, 3, 0);
        gl->addWidget(rEd, 4, 0);
    } else {
        lblRed->hide();
        rEd->hide();
    }
#endif

    gEd = new QColSpinBox(this);
    lblGreen = new QLabel(this);
#ifndef QT_NO_SHORTCUT
    lblGreen->setBuddy(gEd);
#endif
    lblGreen->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
#if !defined(QT_SMALL_COLORDIALOG)
    gl->addWidget(lblGreen, 1, 3);
    gl->addWidget(gEd, 1, 4);
#else
    if (nonTouchUI) {
        gl->addWidget(lblGreen, 3, 1);
        gl->addWidget(gEd, 4, 1);
    } else {
        lblGreen->hide();
        gEd->hide();
    }
#endif

    bEd = new QColSpinBox(this);
    lblBlue = new QLabel(this);
#ifndef QT_NO_SHORTCUT
    lblBlue->setBuddy(bEd);
#endif
    lblBlue->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
#if !defined(QT_SMALL_COLORDIALOG)
    gl->addWidget(lblBlue, 2, 3);
    gl->addWidget(bEd, 2, 4);
#else
    if (nonTouchUI) {
        gl->addWidget(lblBlue, 3, 2);
        gl->addWidget(bEd, 4, 2);
    } else {
        lblBlue->hide();
        bEd->hide();
    }
#endif

    alphaEd = new QColSpinBox(this);
    alphaLab = new QLabel(this);
#ifndef QT_NO_SHORTCUT
    alphaLab->setBuddy(alphaEd);
#endif
    alphaLab->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
#if !defined(QT_SMALL_COLORDIALOG)
    gl->addWidget(alphaLab, 3, 1, 1, 3);
    gl->addWidget(alphaEd, 3, 4);
#else
    if (nonTouchUI) {
        gl->addWidget(alphaLab, 1, 3, 3, 1);
        gl->addWidget(alphaEd, 4, 3);
    } else {
        alphaLab->hide();
        alphaEd->hide();
    }
#endif
    alphaEd->hide();
    alphaLab->hide();
    lblHtml = new QLabel(this);
    htEd = new QLineEdit(this);
#ifndef QT_NO_SHORTCUT
    lblHtml->setBuddy(htEd);
#endif

    QRegularExpression regExp(QStringLiteral("#([A-Fa-f0-9]{6}|[A-Fa-f0-9]{3})"));
    QRegularExpressionValidator *validator = new QRegularExpressionValidator(regExp, this);
    htEd->setValidator(validator);

    lblHtml->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
    gl->addWidget(lblHtml, 5, 1);
    gl->addWidget(htEd, 5, 2);

    connect(hEd, SIGNAL(valueChanged(int)), this, SLOT(hsvEd()));
    connect(sEd, SIGNAL(valueChanged(int)), this, SLOT(hsvEd()));
    connect(vEd, SIGNAL(valueChanged(int)), this, SLOT(hsvEd()));

    connect(rEd, SIGNAL(valueChanged(int)), this, SLOT(rgbEd()));
    connect(gEd, SIGNAL(valueChanged(int)), this, SLOT(rgbEd()));
    connect(bEd, SIGNAL(valueChanged(int)), this, SLOT(rgbEd()));
    connect(alphaEd, SIGNAL(valueChanged(int)), this, SLOT(rgbEd()));
    connect(htEd, SIGNAL(textEdited(QString)), this, SLOT(htmlEd()));

    retranslateStrings();
}

inline QRgb QColorDialogPrivate::currentColor() const { return cs->currentColor(); }
inline int QColorDialogPrivate::currentAlpha() const { return cs->currentAlpha(); }
inline void QColorDialogPrivate::setCurrentAlpha(int a) { cs->setCurrentAlpha(a); }
inline void QColorDialogPrivate::showAlpha(bool b) { cs->showAlpha(b); }
inline bool QColorDialogPrivate::isAlphaVisible() const { return cs->isAlphaVisible(); }

QColor QColorDialogPrivate::currentQColor() const
{
    if (nativeDialogInUse)
        return platformColorDialogHelper()->currentColor();
    return cs->currentQColor();
}

void QColorShower::showCurrentColor()
{
    lab->setColor(currentColor());
    lab->repaint();
}

void QColorShower::rgbEd()
{
    rgbOriginal = true;
    curCol = qRgba(rEd->value(), gEd->value(), bEd->value(), currentAlpha());

    rgb2hsv(currentColor(), hue, sat, val);

    hEd->setValue(hue);
    sEd->setValue(sat);
    vEd->setValue(val);

    htEd->setText(QColor(curCol).name());

    showCurrentColor();
    emit newCol(currentColor());
    updateQColor();
}

void QColorShower::hsvEd()
{
    rgbOriginal = false;
    hue = hEd->value();
    sat = sEd->value();
    val = vEd->value();

    QColor c;
    c.setHsv(hue, sat, val);
    curCol = c.rgb();

    rEd->setValue(qRed(currentColor()));
    gEd->setValue(qGreen(currentColor()));
    bEd->setValue(qBlue(currentColor()));

    htEd->setText(c.name());

    showCurrentColor();
    emit newCol(currentColor());
    updateQColor();
}

void QColorShower::htmlEd()
{
    QColor c;
    QString t = htEd->text();
    c.setNamedColor(t);
    if (!c.isValid())
        return;
    curCol = qRgba(c.red(), c.green(), c.blue(), currentAlpha());
    rgb2hsv(curCol, hue, sat, val);

    hEd->setValue(hue);
    sEd->setValue(sat);
    vEd->setValue(val);

    rEd->setValue(qRed(currentColor()));
    gEd->setValue(qGreen(currentColor()));
    bEd->setValue(qBlue(currentColor()));

    showCurrentColor();
    emit newCol(currentColor());
    updateQColor();
}

void QColorShower::setRgb(QRgb rgb)
{
    rgbOriginal = true;
    curCol = rgb;

    rgb2hsv(currentColor(), hue, sat, val);

    hEd->setValue(hue);
    sEd->setValue(sat);
    vEd->setValue(val);

    rEd->setValue(qRed(currentColor()));
    gEd->setValue(qGreen(currentColor()));
    bEd->setValue(qBlue(currentColor()));

    htEd->setText(QColor(rgb).name());

    showCurrentColor();
    updateQColor();
}

void QColorShower::setHsv(int h, int s, int v)
{
    if (h < -1 || (uint)s > 255 || (uint)v > 255)
        return;

    rgbOriginal = false;
    hue = h; val = v; sat = s;
    QColor c;
    c.setHsv(hue, sat, val);
    curCol = c.rgb();

    hEd->setValue(hue);
    sEd->setValue(sat);
    vEd->setValue(val);

    rEd->setValue(qRed(currentColor()));
    gEd->setValue(qGreen(currentColor()));
    bEd->setValue(qBlue(currentColor()));

    htEd->setText(c.name());

    showCurrentColor();
    updateQColor();
}

void QColorShower::retranslateStrings()
{
    lblHue->setText(QColorDialog::tr("Hu&e:"));
    lblSat->setText(QColorDialog::tr("&Sat:"));
    lblVal->setText(QColorDialog::tr("&Val:"));
    lblRed->setText(QColorDialog::tr("&Red:"));
    lblGreen->setText(QColorDialog::tr("&Green:"));
    lblBlue->setText(QColorDialog::tr("Bl&ue:"));
    alphaLab->setText(QColorDialog::tr("A&lpha channel:"));
    lblHtml->setText(QColorDialog::tr("&HTML:"));
}

void QColorShower::updateQColor()
{
    QColor oldQColor(curQColor);
    curQColor.setRgba(qRgba(qRed(curCol), qGreen(curCol), qBlue(curCol), currentAlpha()));
    if (curQColor != oldQColor)
        emit currentColorChanged(curQColor);
}

//sets all widgets to display h,s,v
void QColorDialogPrivate::_q_newHsv(int h, int s, int v)
{
    if (!nativeDialogInUse) {
        cs->setHsv(h, s, v);
        cp->setCol(h, s);
        lp->setCol(h, s, v);
    }
}

//sets all widgets to display rgb
void QColorDialogPrivate::setCurrentColor(QRgb rgb)
{
    if (!nativeDialogInUse) {
        cs->setRgb(rgb);
        _q_newColorTypedIn(rgb);
    }
}

// hack; doesn't keep curCol in sync, so use with care
void QColorDialogPrivate::setCurrentQColor(const QColor &color)
{
    Q_Q(QColorDialog);
    if (cs->curQColor != color) {
        cs->curQColor = color;
        emit q->currentColorChanged(color);
    }
}

bool QColorDialogPrivate::selectColor(const QColor &col)
{
    QRgb color = col.rgb();
    int i = 0, j = 0;
    // Check standard colors
    if (standard) {
        const QRgb *standardColors = QColorDialogOptions::standardColors();
        for (i = 0; i < 6; i++) {
            for (j = 0; j < 8; j++) {
                if (color == standardColors[i + j*6]) {
                    _q_newStandard(i, j);
                    standard->setCurrent(i, j);
                    standard->setSelected(i, j);
                    standard->setFocus();
                    return true;
                }
            }
        }
    }
    // Check custom colors
    if (custom) {
        const QRgb *customColors = QColorDialogOptions::customColors();
        for (i = 0; i < 2; i++) {
            for (j = 0; j < 8; j++) {
                if (color == customColors[i + j*2]) {
                    _q_newCustom(i, j);
                    custom->setCurrent(i, j);
                    custom->setSelected(i, j);
                    custom->setFocus();
                    return true;
                }
            }
        }
    }
    return false;
}

QColor QColorDialogPrivate::grabScreenColor(const QPoint &p)
{
    const QDesktopWidget *desktop = QApplication::desktop();
    const QPixmap pixmap = QGuiApplication::screens().at(desktop->screenNumber())->grabWindow(desktop->winId(),
                                                                                              p.x(), p.y(), 1, 1);
    QImage i = pixmap.toImage();
    return i.pixel(0, 0);
}

//sets all widgets except cs to display rgb
void QColorDialogPrivate::_q_newColorTypedIn(QRgb rgb)
{
    if (!nativeDialogInUse) {
        int h, s, v;
        rgb2hsv(rgb, h, s, v);
        cp->setCol(h, s);
        lp->setCol(h, s, v);
    }
}

void QColorDialogPrivate::_q_nextCustom(int r, int c)
{
    nextCust = r + 2 * c;
}

void QColorDialogPrivate::_q_newCustom(int r, int c)
{
    const int i = r + 2 * c;
    setCurrentColor(QColorDialogOptions::customColor(i));
    if (standard)
        standard->setSelected(-1,-1);
}

void QColorDialogPrivate::_q_newStandard(int r, int c)
{
    setCurrentColor(QColorDialogOptions::standardColor(r + c * 6));
    if (custom)
        custom->setSelected(-1,-1);
}

void QColorDialogPrivate::_q_pickScreenColor()
{
    Q_Q(QColorDialog);
    if (!colorPickingEventFilter)
        colorPickingEventFilter = new QColorPickingEventFilter(this);
    q->installEventFilter(colorPickingEventFilter);
    // If user pushes Escape, the last color before picking will be restored.
    beforeScreenColorPicking = cs->currentColor();
    /*For some reason, q->grabMouse(Qt::CrossCursor) doesn't change
     * the cursor, and therefore I have to change it manually.
     */
    q->grabMouse();
#ifndef QT_NO_CURSOR
    q->setCursor(Qt::CrossCursor);
#endif
    q->grabKeyboard();
    /* With setMouseTracking(true) the desired color can be more precisedly picked up,
     * and continuously pushing the mouse button is not necessary.
     */
    q->setMouseTracking(true);

    addCusBt->setDisabled(true);
    buttons->setDisabled(true);
    screenColorPickerButton->setDisabled(true);

    q->setCurrentColor(grabScreenColor(QCursor::pos()));
    lblScreenColorInfo->setText(QColorDialog::tr("Cursor at %1, %2, color: %3\nPress ESC to cancel")
                                .arg(QCursor::pos().x())
                                .arg(QCursor::pos().y())
                                .arg(q->currentColor().name()));
}

void QColorDialogPrivate::releaseColorPicking()
{
    Q_Q(QColorDialog);
    q->removeEventFilter(colorPickingEventFilter);
    q->releaseMouse();
    q->releaseKeyboard();
#ifndef QT_NO_CURSOR
    q->setCursor(Qt::ArrowCursor);
#endif
    q->setMouseTracking(false);
    lblScreenColorInfo->setText(QLatin1String("\n"));
    addCusBt->setDisabled(false);
    buttons->setDisabled(false);
    screenColorPickerButton->setDisabled(false);
}

void QColorDialogPrivate::init(const QColor &initial)
{
    Q_Q(QColorDialog);

    q->setSizeGripEnabled(false);
    q->setWindowTitle(QColorDialog::tr("Select Color"));

    // default: use the native dialog if possible.  Can be overridden in setOptions()
    nativeDialogInUse = (platformColorDialogHelper() != 0);
    colorPickingEventFilter = 0;
    nextCust = 0;

    if (!nativeDialogInUse)
        initWidgets();

#ifdef Q_WS_MAC
    delegate = 0;
#endif

    q->setCurrentColor(initial);
}

void QColorDialogPrivate::initWidgets()
{
    Q_Q(QColorDialog);
    QVBoxLayout *mainLay = new QVBoxLayout(q);
    // there's nothing in this dialog that benefits from sizing up
    mainLay->setSizeConstraint(QLayout::SetFixedSize);

    QHBoxLayout *topLay = new QHBoxLayout();
    mainLay->addLayout(topLay);

    leftLay = 0;

#if defined(Q_OS_WINCE) || defined(QT_SMALL_COLORDIALOG)
    smallDisplay = true;
    const int lumSpace = 20;
#else
    // small displays (e.g. PDAs) cannot fit the full color dialog,
    // so just use the color picker.
    smallDisplay = (QApplication::desktop()->width() < 480 || QApplication::desktop()->height() < 350);
    const int lumSpace = topLay->spacing() / 2;
#endif

    if (!smallDisplay) {
        leftLay = new QVBoxLayout;
        topLay->addLayout(leftLay);
    }

    if (!smallDisplay) {
        standard = new QColorWell(q, 6, 8, QColorDialogOptions::standardColors());
        lblBasicColors = new QLabel(q);
#ifndef QT_NO_SHORTCUT
        lblBasicColors->setBuddy(standard);
#endif
        q->connect(standard, SIGNAL(selected(int,int)), SLOT(_q_newStandard(int,int)));
        leftLay->addWidget(lblBasicColors);
        leftLay->addWidget(standard);

#if !defined(Q_OS_WINCE) && !defined(QT_SMALL_COLORDIALOG)
        // The screen color picker button
        screenColorPickerButton = new QPushButton(QColorDialog::tr("Pick Screen Color"));
        leftLay->addWidget(screenColorPickerButton);
        lblScreenColorInfo = new QLabel(QLatin1String("\n"));
        leftLay->addWidget(lblScreenColorInfo);
        q->connect(screenColorPickerButton, SIGNAL(clicked()), SLOT(_q_pickScreenColor()));
#endif

#if !defined(Q_OS_WINCE)
        leftLay->addStretch();
#endif

        custom = new QColorWell(q, 2, 8, QColorDialogOptions::customColors());
        custom->setAcceptDrops(true);

        q->connect(custom, SIGNAL(selected(int,int)), SLOT(_q_newCustom(int,int)));
        q->connect(custom, SIGNAL(currentChanged(int,int)), SLOT(_q_nextCustom(int,int)));
        lblCustomColors = new QLabel(q);
#ifndef QT_NO_SHORTCUT
        lblCustomColors->setBuddy(custom);
#endif
        leftLay->addWidget(lblCustomColors);
        leftLay->addWidget(custom);

        addCusBt = new QPushButton(q);
        QObject::connect(addCusBt, SIGNAL(clicked()), q, SLOT(_q_addCustom()));
        leftLay->addWidget(addCusBt);
    } else {
        // better color picker size for small displays
#if defined(QT_SMALL_COLORDIALOG)
        QSize screenSize = QApplication::desktop()->availableGeometry(QCursor::pos()).size();
        pWidth = pHeight = qMin(screenSize.width(), screenSize.height());
        pHeight -= 20;
        if(screenSize.height() > screenSize.width())
            pWidth -= 20;
#else
        pWidth = 150;
        pHeight = 100;
#endif
        custom = 0;
        standard = 0;
    }

    QVBoxLayout *rightLay = new QVBoxLayout;
    topLay->addLayout(rightLay);

    QHBoxLayout *pickLay = new QHBoxLayout;
    rightLay->addLayout(pickLay);

    QVBoxLayout *cLay = new QVBoxLayout;
    pickLay->addLayout(cLay);
    cp = new QColorPicker(q);

    cp->setFrameStyle(QFrame::Panel + QFrame::Sunken);

#if defined(QT_SMALL_COLORDIALOG)
    if (!nonTouchUI) {
        pickLay->addWidget(cp);
        cLay->addSpacing(lumSpace);
    } else {
        cp->hide();
    }
#else
    cLay->addSpacing(lumSpace);
    cLay->addWidget(cp);
#endif
    cLay->addSpacing(lumSpace);

    lp = new QColorLuminancePicker(q);
#if defined(QT_SMALL_COLORDIALOG)
    QSize screenSize = QApplication::desktop()->availableGeometry(QCursor::pos()).size();
    const int minDimension = qMin(screenSize.height(), screenSize.width());
    //set picker to be finger-usable
    int pickerWidth = !nonTouchUI ? minDimension/9 : minDimension/12;
    lp->setFixedWidth(pickerWidth);
    if (!nonTouchUI)
        pickLay->addWidget(lp);
    else
        lp->hide();
#else
    lp->setFixedWidth(20);
    pickLay->addWidget(lp);
#endif

    QObject::connect(cp, SIGNAL(newCol(int,int)), lp, SLOT(setCol(int,int)));
    QObject::connect(lp, SIGNAL(newHsv(int,int,int)), q, SLOT(_q_newHsv(int,int,int)));

    rightLay->addStretch();

    cs = new QColorShower(q);
    QObject::connect(cs, SIGNAL(newCol(QRgb)), q, SLOT(_q_newColorTypedIn(QRgb)));
    QObject::connect(cs, SIGNAL(currentColorChanged(QColor)),
                     q, SIGNAL(currentColorChanged(QColor)));
#if defined(QT_SMALL_COLORDIALOG)
    if (!nonTouchUI)
        pWidth -= cp->size().width();
    topLay->addWidget(cs);
#else
    rightLay->addWidget(cs);
#endif

    buttons = new QDialogButtonBox(q);
    mainLay->addWidget(buttons);

    ok = buttons->addButton(QDialogButtonBox::Ok);
    QObject::connect(ok, SIGNAL(clicked()), q, SLOT(accept()));
    ok->setDefault(true);
    cancel = buttons->addButton(QDialogButtonBox::Cancel);
    QObject::connect(cancel, SIGNAL(clicked()), q, SLOT(reject()));

    retranslateStrings();
}

void QColorDialogPrivate::initHelper(QPlatformDialogHelper *h)
{
    QColorDialog *d = q_func();
    QObject::connect(h, SIGNAL(currentColorChanged(QColor)), d, SIGNAL(currentColorChanged(QColor)));
    QObject::connect(h, SIGNAL(colorSelected(QColor)), d, SIGNAL(colorSelected(QColor)));
    static_cast<QPlatformColorDialogHelper *>(h)->setOptions(options);
}

void QColorDialogPrivate::helperPrepareShow(QPlatformDialogHelper *)
{
    options->setWindowTitle(q_func()->windowTitle());
}

void QColorDialogPrivate::_q_addCustom()
{
    QColorDialogOptions::setCustomColor(nextCust, cs->currentColor());
    if (custom)
        custom->update();
    nextCust = (nextCust+1) % 16;
}

void QColorDialogPrivate::retranslateStrings()
{
    if (!smallDisplay) {
        lblBasicColors->setText(QColorDialog::tr("&Basic colors"));
        lblCustomColors->setText(QColorDialog::tr("&Custom colors"));
        addCusBt->setText(QColorDialog::tr("&Add to Custom Colors"));
    }

    cs->retranslateStrings();
}

bool QColorDialogPrivate::canBeNativeDialog() const
{
    Q_Q(const QColorDialog);
    if (nativeDialogInUse)
        return true;
    if (q->testAttribute(Qt::WA_DontShowOnScreen))
        return false;
    if (q->options() & QColorDialog::DontUseNativeDialog)
        return false;

    QLatin1String staticName(QColorDialog::staticMetaObject.className());
    QLatin1String dynamicName(q->metaObject()->className());
    return (staticName == dynamicName);
}

static const Qt::WindowFlags DefaultWindowFlags =
        Qt::Dialog | Qt::WindowTitleHint | Qt::MSWindowsFixedSizeDialogHint
        | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint;

/*!
    \class QColorDialog
    \brief The QColorDialog class provides a dialog widget for specifying colors.

    \ingroup standard-dialogs
    \inmodule QtWidgets

    The color dialog's function is to allow users to choose colors.
    For example, you might use this in a drawing program to allow the
    user to set the brush color.

    The static functions provide modal color dialogs.
    \omit
    If you require a modeless dialog, use the QColorDialog constructor.
    \endomit

    The static getColor() function shows the dialog, and allows the user to
    specify a color. This function can also be used to let users choose a
    color with a level of transparency: pass the ShowAlphaChannel option as
    an additional argument.

    The user can store customCount() different custom colors. The
    custom colors are shared by all color dialogs, and remembered
    during the execution of the program. Use setCustomColor() to set
    the custom colors, and use customColor() to get them.

    When pressing the "Pick Screen Color" button, the cursor changes to a haircross
    and the colors on the screen are scanned. The user can pick up one by clicking
    the mouse or the Enter button. Pressing Escape restores the last color selected
    before entering this mode.

    The \l{dialogs/standarddialogs}{Standard Dialogs} example shows
    how to use QColorDialog as well as other built-in Qt dialogs.

    \image fusion-colordialog.png A color dialog in the Fusion widget style.

    \sa QColor, QFileDialog, QFontDialog, {Standard Dialogs Example}
*/

/*!
    \since 4.5

    Constructs a color dialog with the given \a parent.
*/
QColorDialog::QColorDialog(QWidget *parent)
    : QDialog(*new QColorDialogPrivate, parent, DefaultWindowFlags)
{
    Q_D(QColorDialog);
    d->init(Qt::white);
}

/*!
    \since 4.5

    Constructs a color dialog with the given \a parent and specified
    \a initial color.
*/
QColorDialog::QColorDialog(const QColor &initial, QWidget *parent)
    : QDialog(*new QColorDialogPrivate, parent, DefaultWindowFlags)
{
    Q_D(QColorDialog);
    d->init(initial);
}

/*!
    \property QColorDialog::currentColor
    \brief the currently selected color in the dialog
*/

void QColorDialog::setCurrentColor(const QColor &color)
{
    Q_D(QColorDialog);
    if (d->nativeDialogInUse)
        d->platformColorDialogHelper()->setCurrentColor(color);
    else {
        d->setCurrentColor(color.rgb());
        d->selectColor(color);
        d->setCurrentAlpha(color.alpha());
    }
}

QColor QColorDialog::currentColor() const
{
    Q_D(const QColorDialog);
    return d->currentQColor();
}

/*!
    Returns the color that the user selected by clicking the \uicontrol{OK}
    or equivalent button.

    \note This color is not always the same as the color held by the
    \l currentColor property since the user can choose different colors
    before finally selecting the one to use.
*/
QColor QColorDialog::selectedColor() const
{
    Q_D(const QColorDialog);
    return d->selectedQColor;
}

/*!
    Sets the given \a option to be enabled if \a on is true;
    otherwise, clears the given \a option.

    \sa options, testOption()
*/
void QColorDialog::setOption(ColorDialogOption option, bool on)
{
    const QColorDialog::ColorDialogOptions previousOptions = options();
    if (!(previousOptions & option) != !on)
        setOptions(previousOptions ^ option);
}

/*!
    \since 4.5

    Returns \c true if the given \a option is enabled; otherwise, returns
    false.

    \sa options, setOption()
*/
bool QColorDialog::testOption(ColorDialogOption option) const
{
    Q_D(const QColorDialog);
    return d->options->testOption(static_cast<QColorDialogOptions::ColorDialogOption>(option));
}

/*!
    \property QColorDialog::options
    \brief the various options that affect the look and feel of the dialog

    By default, all options are disabled.

    Options should be set before showing the dialog. Setting them while the
    dialog is visible is not guaranteed to have an immediate effect on the
    dialog (depending on the option and on the platform).

    \sa setOption(), testOption()
*/
void QColorDialog::setOptions(ColorDialogOptions options)
{
    Q_D(QColorDialog);

    if (QColorDialog::options() == options)
        return;

    d->options->setOptions(QColorDialogOptions::ColorDialogOptions(int(options)));
    if ((options & DontUseNativeDialog) && d->nativeDialogInUse) {
        d->nativeDialogInUse = false;
        d->initWidgets();
    }
    if (!d->nativeDialogInUse) {
        d->buttons->setVisible(!(options & NoButtons));
        d->showAlpha(options & ShowAlphaChannel);
    }
}

QColorDialog::ColorDialogOptions QColorDialog::options() const
{
    Q_D(const QColorDialog);
    return QColorDialog::ColorDialogOptions(int(d->options->options()));
}

/*!
    \enum QColorDialog::ColorDialogOption

    \since 4.5

    This enum specifies various options that affect the look and feel
    of a color dialog.

    \value ShowAlphaChannel Allow the user to select the alpha component of a color.
    \value NoButtons Don't display \uicontrol{OK} and \uicontrol{Cancel} buttons. (Useful for "live dialogs".)
    \value DontUseNativeDialog  Use Qt's standard color dialog instead of the operating system
                                native color dialog.

    \sa options, setOption(), testOption(), windowModality()
*/

/*!
    \fn void QColorDialog::currentColorChanged(const QColor &color)

    This signal is emitted whenever the current color changes in the dialog.
    The current color is specified by \a color.

    \sa color, colorSelected()
*/

#ifdef Q_WS_MAC
// can only have one Cocoa color panel active
bool QColorDialogPrivate::sharedColorPanelAvailable = true;
#endif

/*!
    \fn void QColorDialog::colorSelected(const QColor &color);

    This signal is emitted just after the user has clicked \uicontrol{OK} to
    select a color to use. The chosen color is specified by \a color.

    \sa color, currentColorChanged()
*/

/*!
    Changes the visibility of the dialog. If \a visible is true, the dialog
    is shown; otherwise, it is hidden.
*/
void QColorDialog::setVisible(bool visible)
{
    Q_D(QColorDialog);

    if (visible){
        if (testAttribute(Qt::WA_WState_ExplicitShowHide) && !testAttribute(Qt::WA_WState_Hidden))
            return;
    } else if (testAttribute(Qt::WA_WState_ExplicitShowHide) && testAttribute(Qt::WA_WState_Hidden))
        return;

    if (visible)
        d->selectedQColor = QColor();

#if defined(Q_WS_MAC)
    if (visible) {
        if (d->delegate || (QColorDialogPrivate::sharedColorPanelAvailable &&
                !(testAttribute(Qt::WA_DontShowOnScreen) || (d->opts & DontUseNativeDialog)))){
            d->openCocoaColorPanel(currentColor(), parentWidget(), windowTitle(), options());
            QColorDialogPrivate::sharedColorPanelAvailable = false;
            setAttribute(Qt::WA_DontShowOnScreen);
        }
        setWindowFlags(windowModality() == Qt::WindowModal ? Qt::Sheet : DefaultWindowFlags);
    } else {
        if (d->delegate) {
            d->closeCocoaColorPanel();
            setAttribute(Qt::WA_DontShowOnScreen, false);
        }
    }
#else

    if (d->nativeDialogInUse) {
        d->setNativeDialogVisible(visible);
        // Set WA_DontShowOnScreen so that QDialog::setVisible(visible) below
        // updates the state correctly, but skips showing the non-native version:
        setAttribute(Qt::WA_DontShowOnScreen);
    } else {
        setAttribute(Qt::WA_DontShowOnScreen, false);
    }
#endif

    QDialog::setVisible(visible);
}

/*!
    \overload
    \since 4.5

    Opens the dialog and connects its colorSelected() signal to the slot specified
    by \a receiver and \a member.

    The signal will be disconnected from the slot when the dialog is closed.
*/
void QColorDialog::open(QObject *receiver, const char *member)
{
    Q_D(QColorDialog);
    connect(this, SIGNAL(colorSelected(QColor)), receiver, member);
    d->receiverToDisconnectOnClose = receiver;
    d->memberToDisconnectOnClose = member;
    QDialog::open();
}

/*!
    \fn QColorDialog::open()

    \since 4.5
    Shows the dialog as a \l{QDialog#Modal Dialogs}{window modal dialog},
    returning immediately.

    \sa QDialog::open()
*/

/*!
    \since 4.5

    Pops up a modal color dialog with the given window \a title (or "Select Color" if none is
    specified), lets the user choose a color, and returns that color. The color is initially set
    to \a initial. The dialog is a child of \a parent. It returns an invalid (see
    QColor::isValid()) color if the user cancels the dialog.

    The \a options argument allows you to customize the dialog.
*/
QColor QColorDialog::getColor(const QColor &initial, QWidget *parent, const QString &title,
                              ColorDialogOptions options)
{
    QColorDialog dlg(parent);
    if (!title.isEmpty())
        dlg.setWindowTitle(title);
    dlg.setOptions(options);
    dlg.setCurrentColor(initial);
    dlg.exec();
    return dlg.selectedColor();
}

/*!
    \obsolete

    Pops up a modal color dialog to allow the user to choose a color
    and an alpha channel (transparency) value. The color+alpha is
    initially set to \a initial. The dialog is a child of \a parent.

    If \a ok is non-null, \e *\a ok is set to true if the user clicked
    \uicontrol{OK}, and to false if the user clicked Cancel.

    If the user clicks Cancel, the \a initial value is returned.

    Use QColorDialog::getColor() instead, passing the
    QColorDialog::ShowAlphaChannel option.
*/

QRgb QColorDialog::getRgba(QRgb initial, bool *ok, QWidget *parent)
{
    QColor color(getColor(QColor(initial), parent, QString(), ShowAlphaChannel));
    QRgb result = color.isValid() ? color.rgba() : initial;
    if (ok)
        *ok = color.isValid();
    return result;
}

/*!
    Destroys the color dialog.
*/

QColorDialog::~QColorDialog()
{
#if defined(Q_WS_MAC)
    Q_D(QColorDialog);
    if (d->delegate) {
        d->releaseCocoaColorPanelDelegate();
        QColorDialogPrivate::sharedColorPanelAvailable = true;
    }
#endif
}

/*!
    \reimp
*/
void QColorDialog::changeEvent(QEvent *e)
{
    Q_D(QColorDialog);
    if (e->type() == QEvent::LanguageChange)
        d->retranslateStrings();
    QDialog::changeEvent(e);
}

bool QColorDialogPrivate::handleColorPickingMouseMove(QMouseEvent *e)
{
    Q_Q(QColorDialog);
    const QPoint globalPos = e->globalPos();
    const QColor color = grabScreenColor(globalPos);
    q->setCurrentColor(color);
    lblScreenColorInfo->setText(QColorDialog::tr("Cursor at %1, %2, color: %3\nPress ESC to cancel")
                                .arg(globalPos.x()).arg(globalPos.y()).arg(color.name()));
    return true;
}

bool QColorDialogPrivate::handleColorPickingMouseButtonRelease(QMouseEvent *e)
{
    Q_Q(QColorDialog);
    q->setCurrentColor(grabScreenColor(e->globalPos()));
    releaseColorPicking();
    return true;
}

bool QColorDialogPrivate::handleColorPickingKeyPress(QKeyEvent *e)
{
    Q_Q(QColorDialog);
    switch (e->key()) {
    case Qt::Key_Escape:
        releaseColorPicking();
        q->setCurrentColor(beforeScreenColorPicking);
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        q->setCurrentColor(grabScreenColor(QCursor::pos()));
        releaseColorPicking();
        break;
    }
    e->accept();
    return true;
}

/*!
  Closes the dialog and sets its result code to \a result. If this dialog
  is shown with exec(), done() causes the local event loop to finish,
  and exec() to return \a result.

  \sa QDialog::done()
*/
void QColorDialog::done(int result)
{
    Q_D(QColorDialog);
    if (result == Accepted) {
        d->selectedQColor = d->currentQColor();
        emit colorSelected(d->selectedQColor);
    } else {
        d->selectedQColor = QColor();
    }
    QDialog::done(result);
    if (d->receiverToDisconnectOnClose) {
        disconnect(this, SIGNAL(colorSelected(QColor)),
                   d->receiverToDisconnectOnClose, d->memberToDisconnectOnClose);
        d->receiverToDisconnectOnClose = 0;
    }
    d->memberToDisconnectOnClose.clear();
}

QT_END_NAMESPACE

#include "qcolordialog.moc"
#include "moc_qcolordialog.cpp"

#endif // QT_NO_COLORDIALOG

