/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include <qcoreevent.h>
#include <qdatetime.h>
#include <qlibraryinfo.h>
#include <qobject.h>
#include <qcoreapplication.h>
#include <private/qcoreapplication_p.h>

#include "stdio.h"
#include "stdlib.h"

QT_BEGIN_NAMESPACE

#include "qconfig_eval.cpp"

static const char boilerplate_supported_but_time_limited[] =
    "\nQt %1 Evaluation License\n"
    "Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).\n"
    "This trial version may only be used for evaluation purposes\n"
    "and will shut down after 120 minutes.\n"
    "Registered to:\n"
    "   Licensee: %2\n\n"
    "The evaluation expires in %4 days\n\n"
    "Contact http://qt.digia.com/contact-us for pricing and purchasing information.\n";

static const char boilerplate_supported[] =
    "\nQt %1 Evaluation License\n"
    "Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).\n"
    "This trial version may only be used for evaluation purposes\n"
    "Registered to:\n"
    "   Licensee: %2\n\n"
    "The evaluation expires in %4 days\n\n"
    "Contact http://qt.digia.com/contact-us for pricing and purchasing information.\n";

static const char boilerplate_expired[] =
    "This software is using the trial version of the Qt GUI toolkit.\n"
    "The trial period has expired. If you need more time to\n"
    "evaluate Qt, or if you have any questions about Qt, contact us\n"
    "at: http://qt.digia.com/contact-us.\n\n";

static const char will_shutdown_1min[] =
    "\nThe evaluation of Qt will SHUT DOWN in 1 minute.\n"
    "Contact http://qt.digia.com/contact-us for pricing and purchasing information.\n";

static const char will_shutdown_now[] =
    "\nThe evaluation of Qt has now reached its automatic\n"
    "timeout and will shut down.\n"
    "Contact http://qt.digia.com/contact-us for pricing and purchasing information.\n";

enum EvaluationStatus {
    EvaluationNotSupported = 0,
    EvaluationSupportedButTimeLimited,
    EvaluationSupported
};

static EvaluationStatus qt_eval_is_supported()
{
    const volatile char *const license_key = qt_eval_key_data + 12;

    // fast fail
    if (!qt_eval_key_data[0] || !*license_key)
        return EvaluationNotSupported;

    // is this an unsupported evaluation?
    const volatile char *typecode = license_key;
    int field = 2;
    for ( ; field && *typecode; ++typecode)
        if (*typecode == '-')
            --field;

    if (!field && typecode[1] == '4' && typecode[2] == 'M') {
        if (typecode[0] == 'Q')
            return EvaluationSupportedButTimeLimited;
        else if (typecode[0] == 'R' || typecode[0] == 'Z')
            return EvaluationSupported;
    }
    return EvaluationNotSupported;
}

static int qt_eval_days_left()
{
    QDate today = QDate::currentDate();
    QDate build = QLibraryInfo::buildDate();
    return qMax<qint64>(-1, today.daysTo(build) + 30);
}

static bool qt_eval_is_expired()
{
    return qt_eval_days_left() < 0;
}

static QString qt_eval_string()
{
    const char *msg;
    switch (qt_eval_is_supported()) {
    case EvaluationSupportedButTimeLimited:
        msg = boilerplate_supported_but_time_limited;
        break;
    case EvaluationSupported:
        msg = boilerplate_supported;
        break;
    default:
        return QString();
        msg = 0;
    }

    return QString::fromLatin1(msg)
        .arg(QLatin1String(QT_VERSION_STR))
        .arg(QLibraryInfo::licensee())
        .arg(qt_eval_days_left());
}

#define WARN_TIMEOUT 60 * 1000 * 119
#define KILL_DELAY 60 * 1000 * 1

class QCoreFuriCuri : public QObject
{
public:

    int warn;
    int kill;

    QCoreFuriCuri() : QObject(), warn(-1), kill(-1)
    {
        if (qt_eval_is_supported() == EvaluationSupportedButTimeLimited) {
            warn = startTimer(WARN_TIMEOUT);
            kill = 0;
        }
    }

    void timerEvent(QTimerEvent *e) {
        if (e->timerId() == warn) {
            killTimer(warn);
            fprintf(stderr, "%s\n", will_shutdown_1min);
            kill = startTimer(KILL_DELAY);
        } else if (e->timerId() == kill) {
            fprintf(stderr, "%s\n", will_shutdown_now);
            QCoreApplication::instance()->quit();
        }
    }
};

#if defined(QT_BUILD_CORE_LIB) || defined (QT_BOOTSTRAPPED)

void qt_core_eval_init(QCoreApplicationPrivate::Type type)
{
    if (type != QCoreApplicationPrivate::Tty)
        return;

    if (!qt_eval_is_supported())
        return;

    if (qt_eval_is_expired()) {
        fprintf(stderr, "%s\n", boilerplate_expired);
        exit(0);
    } else {
        fprintf(stderr, "%s\n", qPrintable(qt_eval_string()));
        Q_UNUSED(new QCoreFuriCuri());
    }
}
#endif

#ifdef QT_BUILD_WIDGETS_LIB

QT_BEGIN_INCLUDE_NAMESPACE
#include <qdialog.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qmessagebox.h>
#include <qpushbutton.h>
#include <qtimer.h>
#include <qapplication.h>
QT_END_INCLUDE_NAMESPACE


static const char * const qtlogo_eval_xpm[] = {
/* columns rows colors chars-per-pixel */
"46 55 174 2",
"   c #002E02",
".  c #00370D",
"X  c #003A0E",
"o  c #003710",
"O  c #013C13",
"+  c #043E1A",
"@  c #084F0A",
"#  c #0B520C",
"$  c #054413",
"%  c #0C4C17",
"&  c #07421D",
"*  c #09451D",
"=  c #0D491E",
"-  c #125515",
";  c #13541A",
":  c #17591B",
">  c #1B5C1D",
",  c #1F611F",
"<  c #20621E",
"1  c #337B1E",
"2  c #0B4521",
"3  c #0F4923",
"4  c #114B24",
"5  c #154D2A",
"6  c #175323",
"7  c #1C5924",
"8  c #1C532F",
"9  c #1E5432",
"0  c #245936",
"q  c #265938",
"w  c #295C3B",
"e  c #246324",
"r  c #266823",
"t  c #2A6C24",
"y  c #276628",
"u  c #2D7026",
"i  c #327427",
"p  c #367927",
"a  c #37782A",
"s  c #397C2A",
"d  c #2E613E",
"f  c #336C37",
"g  c #2F6040",
"h  c #356545",
"j  c #3C6B4E",
"k  c #3F6C51",
"l  c #406E4F",
"z  c #406D52",
"x  c #477457",
"c  c #497557",
"v  c #4B7857",
"b  c #517B5E",
"n  c #3C8423",
"m  c #3E812C",
"M  c #53A61D",
"N  c #41862C",
"B  c #458A2D",
"V  c #498F2D",
"C  c #479324",
"Z  c #489226",
"A  c #4D952C",
"S  c #478B30",
"D  c #488C30",
"F  c #4D9232",
"G  c #509632",
"H  c #549A33",
"J  c #589F35",
"K  c #56A526",
"L  c #57A821",
"P  c #5BAA27",
"I  c #57A32A",
"U  c #5CA72E",
"Y  c #5DAB2A",
"T  c #5CA336",
"R  c #60AD2E",
"E  c #63B12D",
"W  c #65AF35",
"Q  c #62A53F",
"!  c #65AE39",
"~  c #66B036",
"^  c #6AB437",
"/  c #67B138",
"(  c #6AB339",
")  c #6DB838",
"_  c #70BA3C",
"`  c #4D8545",
"'  c #4E8942",
"]  c #548851",
"[  c #6FAF4A",
"{  c #6DB243",
"}  c #71B546",
"|  c #70B840",
" . c #73B648",
".. c #79BA4E",
"X. c #7CBB53",
"o. c #598266",
"O. c #62886D",
"+. c #6A8F75",
"@. c #6B9173",
"#. c #70937A",
"$. c #799F79",
"%. c #7BAF66",
"&. c #81BD5B",
"*. c #85BF60",
"=. c #85AC7F",
"-. c #8DBA7B",
";. c #87C061",
":. c #8AC364",
">. c #8DC46A",
",. c #90C56E",
"<. c #93C771",
"1. c #96CA73",
"2. c #9ACB7C",
"3. c #9FD07D",
"4. c #779981",
"5. c #7F9F89",
"6. c #809F88",
"7. c #82A18B",
"8. c #86A192",
"9. c #8DA994",
"0. c #8FA998",
"q. c #94AF9B",
"w. c #97B991",
"e. c #97B19E",
"r. c #9DB6A3",
"t. c #A3BCA7",
"y. c #A6BCAB",
"u. c #A9BEB1",
"i. c #9ECD81",
"p. c #A2CF85",
"a. c #A5D284",
"s. c #A6D189",
"d. c #A9D28E",
"f. c #ABD491",
"g. c #B1D797",
"h. c #B1D699",
"j. c #B5D89E",
"k. c #ADC5AC",
"l. c #B1CAAE",
"z. c #B9DAA3",
"x. c #BDDDA8",
"c. c #ADC1B4",
"v. c #B2C6B6",
"b. c #B5C6BC",
"n. c #B6C9BA",
"m. c #BCD1BA",
"M. c #C6E1B4",
"N. c #CDE5BD",
"B. c #C2D2C6",
"V. c #CADEC2",
"C. c #C6D3CC",
"Z. c #C8D7CB",
"A. c #CEDAD2",
"S. c #D2DDD4",
"D. c #D3E9C6",
"F. c #D7EBC9",
"G. c #D9EBCD",
"H. c #DEEED4",
"J. c #D6E0D9",
"K. c #DAE4DC",
"L. c #E0EFD7",
"P. c #E5F2DD",
"I. c #DFE8E0",
"U. c #E4EBE5",
"Y. c #E9EFEA",
"T. c #EDF4EB",
"R. c #F0FAE6",
"E. c #F1F8EC",
"W. c #EDF0F0",
"Q. c #F4F7F3",
"!. c #F6F9F4",
"~. c #F8FAF7",
"^. c #FEFEFE",
"/. c None",
/* pixels */
"/././././.c h ' Q / W _ &.p././././././././././././././././././././././././././././././././.",
"/././.4 O % Z ~ ~ W ~ W R U R R ( X.>.p././././././././././././././././././././././././././.",
"/./.. * = J _ ~ ~ ~ ~ ~ / / / / W W U P P U W  .;.2././././././././././././././././././././.",
"/.= = & a ) W ~ ~ ~ ~ ~ / W / ~ ~ ~ ^ ( ( ^ ~ R R U P Y ~  .;.2././././././././././././././.",
"O.O = = T ^ W ~ ~ ~ ~ ~ ~ W W / W ~ ~ ~ ~ ~ ~ ~ ( ( ( ( ~ W Y Y Y Y W { &.1././././././././.",
"0 = * 7 ~ ~ ~ ~ ~ ~ ~ ~ ~ / / W ~ ~ ~ ~ ~ ~ ~ ~ W W W ~ ~ ~ ~ ( ( ( W W R U P U W { X.1.f./.",
"= = & e ^ W ~ ~ ~ ~ ~ ~ ~ ~ / / ~ ~ ~ ~ ~ ~ ~ ~ W ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ^ ( ( / ~ W R U U Y ",
"= = & e ^ W ~ ~ ~ ~ ~ ~ ~ ~ W W ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ W ( W ~ ~ ~ ^ ^ ( ",
"= = * e ^ W ~ ~ ~ ~ ~ ~ / W / W ! ( / ~ W ^ ( ( ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ W ~ W W ~ ~ ~ ~ ~ ~ ",
"= = & e ^ ! ~ ~ ~ ~ ~ ~ W W ^ _ ~ K Y W W R P Y W ( ~ ~ ~ ~ ~ ~ ~ W / ~ ~ ~ ^ W ~ ~ ~ ~ ~ ~ ",
"= = & e ^ W ~ ~ ~ ~ ~ ~ W ) W 1 ` w.V.L.H.D.z.,.~ Y ^ ~ ~ ~ ~ ~ W ~ ~ ~ ( ~ W W ~ ~ ~ ~ ~ ~ ",
"= = & e ^ W ~ ~ ~ ~ ~ W ) V = 8.~.^.^.^.^.^.^.^.U.<.Y ~ ~ ~ ~ ~ W W ! ~ Y W ^ W ~ ~ ~ ~ ~ ~ ",
"= = & e ^ W ~ ~ ~ ~ W ^ B O u.^.~.^.^.^.^.~.~.^.^.^.h.Y ^ ~ ~ ^ F $ k.R.G.1.Y / ~ ~ ~ ~ ~ ~ ",
"= = & e ^ ~ ~ ~ / W ( J X 7.^.~.^.^.^.^.^.^.^.^.^.^.^.s.Y / W ) a 2 U.^.^.d.U ( ~ ~ ~ ~ ~ ~ ",
"= = & e ^ W / ~ ~ ~ ^ > w ~.^.^.^.^.^.F.%.v c.^.^.^.^.~.X.W ~ ^ > h ^.^.^.d.P ( ~ ~ ~ ~ ~ ~ ",
"= = & e ^ W ~ ~ W ^ H o e.^.^.^.^.^.G.Y E n . y.^.^.^.^.M.Y ( ! $ @.^.~.^.f.U ( / ~ ~ W ~ ~ ",
"= = & e ^ W ~ W ! ) t 4 U.^.^.^.^.^.>.U ( _ , 9 ~.^.^.^.~...^ A   y.^.~.^.s.M W Y ~ ~ ~ ~ ~ ",
"= 3 & e ^ W ~ ( ^ ( $ c ^.^.^.^.^.E.) ~ ~ ^ S o n.^.^.^.^.=.- l.v.Y.^.^.^.M.:.:.X.~ ~ ~ ~ ~ ",
"= = & e ^ ! W W ( J X 7.^.^.^.^.^.F.Y ( W ^ T X 6.^.^.~.^.c.. J.^.^.^.^.^.^.^.^.P.~ ~ ~ ~ ~ ",
"= = & r ^ W / W ) B o v.^.~.^.^.^.M.U / ~ ~ ! $ o.^.^.^.^.K.* S.^.^.^.^.^.^.^.^.P.~ ~ ~ ~ ~ ",
"= = & e ^ ! ~ W ) a + S.^.^.^.^.^.z.P ( W ~ ( % z ^.^.^.^.~.f t.U.^.^.^.^.~.^.^.P.~ ~ ~ ~ ~ ",
"* = & e ^ W ~ W ) t 3 Y.^.^.^.^.^.f.P ( ~ ~ ^ ; h ^.^.^.^.^.:.@ j ^.^.^.^.h.{ X.&.~ ~ ~ ~ ~ ",
"3 = & e ^ W ~ ~ ^ e 8 Q.^.^.^.^.^.s.P ~ ~ W ^ > 0 ~.^.^.^.^.1.# z ^.^.^.^.d.L W R ~ ~ ~ ~ ~ ",
"= = & e ^ W ~ ~ ^ > q ~.^.^.^.^.^.p.U ^ ~ W ) e 9 ~.^.^.^.^.3.# k ^.^.^.^.f.Y ( / ~ ~ ~ ~ ~ ",
"= = & e ^ W / W ^ > w ~.^.^.^.^.^.i.Y / ~ W ^ e 8 Q.^.^.^.^.a.# z ^.^.^.^.f.Y / ~ ~ ~ ~ ~ ~ ",
"= = & e ^ W / W ^ > w ^.^.^.^.^.^.2.Y / ~ ~ ) e 8 Q.^.^.^.^.s.# z ^.^.^.^.d.P ( ~ ~ ~ ~ ~ ~ ",
"= = & e ^ W W W ^ > q ^.^.^.^.^.^.p.Y / ~ ~ ^ e 9 Q.^.^.^.^.a.@ z ^.^.^.^.f.U / ~ ~ ~ ~ ~ ~ ",
"= = & e ^ W / W ) 7 9 Q.^.^.^.^.^.a.P / ~ W ) , 9 Q.^.^.^.^.3.# z ^.^.~.^.f.P ^ ~ ~ ~ ~ ~ ~ ",
"= = & e ^ W / W ) r 5 T.^.^.^.^.^.d.Y / ~ W ) > q ~.^.^.^.^.1.# k ^.^.^.^.f.Y ( ~ ~ ~ ~ ~ ~ ",
"= = & e ^ / / W ) i 2 I.^.^.^.^.^.h.P ( ~ W ( > g ^.^.^.^.^.:.# z ^.^.^.^.f.P / ~ ~ ~ ~ ~ ~ ",
"= = & e ( W / W ) m O Z.^.^.^.^.^.x.P / ~ ~ ( ; j ^.^.^.^.~.&.- k ^.^.~.^.f.P / ~ ~ ~ ~ ~ ~ ",
"= = & e ( W / W ) F o y.^.~.^.^.^.N.U ( ~ ~ W $ b ^.^.^.^.R._ - k ^.^.^.^.f.Y ( ~ ~ ~ ~ ~ ~ ",
"= = & e ^ W ~ ~ ^ J X 4.^.^.^.^.^.L.~ ~ W ^ T X #.^.^.^.^.F.~ ; j ^.^.^.^.f.U ( ~ ~ ~ ~ ~ ~ ",
"= = & e ^ ~ ~ ~ / ^ % l ^.^.^.^.^.!. .R ^ ^ G . r.^.~.^.^.j.E : j ^.^.^.^.f.P ) ( ~ ~ ~ ~ ~ ",
"= = & e ^ W ~ ~ W ) u = U.^.^.^.^.^.1.Y ! ) a & K.^.^.^.^.;.~ : j ^.^.~.^.z.M I I / ~ ~ W ~ ",
"= = & e ( W ~ ~ W ( G . q.^.^.^.^.^.D.U ^ ! X o.^.^.^.^.P.~ ^ > g ^.^.^.^.E.-.$.m.X.W ~ ~ ~ ",
"= = & e ^ / ~ ~ ^ ! ( > w ~.^.^.^.^.^.h.T > j T.^.^.~.^.a.Y _ i 3 U.^.^.^.^.^.^.^.X.R ~ ~ ~ ",
"= = & e ^ / ~ ~ W W ^ H . 9.^.~.^.^.^.^.K.C.~.^.^.^.^.H.W W ^ T . q.^.~.^.^.^.^.^.X.R ~ ~ ~ ",
"= = + e ^ W / ~ W W W ) m + B.^.~.^.^.^.^.^.^.^.^.^.E.X.Y ( W ^ B 6 y.^.^.^.E.D.2.( ~ ~ ~ ~ ",
"= = * e ^ ! / ! W ^ W W ) a 4 b.^.^.^.^.^.^.^.^.^.P...Y ( ! W ! ^ W Z [ *.X.{ Y U ~ ~ ~ ~ ~ ",
"= = & e ( W ~ ~ W / W / W ) A < +.A.~.^.^.^.^.!.p.W R ~ ~ ~ ~ ~ W / ) E U W W / ^ ~ ~ ~ ~ ~ ",
"= = & e ^ W ~ ~ / W / / / W ( _ Z X 6.^.^.^.^.E.W ~ ^ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ / ~ ~ ~ ~ ~ ~ ~ ~ ",
"= = & e ^ ~ ~ ~ W W / W ~ ~ ~ ~ ) ; h ^.^.^.^.^.d.M U ~ / ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ W ",
"= = & e ^ W ~ ~ ^ W W / ~ ~ ~ W ) p + S.^.^.^.^.~.M.f. .W ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~  .",
"= = & e ^ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ W ( T O +.^.~.^.^.^.^.^.&.Y ( ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ W ( Y 2.",
"= = & e ( W ~ ~ ~ ~ ~ ~ ~ ~ ~ / W ) N + b.^.^.^.^.^.^.&.R ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ W /.",
"= = & e ^ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ W ^ N 7 r.W.^.^.^.!.X.W ~ ~ W ~ W ~ ~ ~ ~ ~ ~ / ( ( K p./.",
"= = & e ( W ~ ~ W ~ ~ ~ ~ ~ ~ ~ ~ ~ W ( W C Q &.:.X.| ~ ~ ~ ~ W ~ / ~ ( / ( ~ W E U P 1././.",
"= = + e ^ / / / ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ W / ) ^ R Y W W ~ ~ ( / ( / W R Y Y U R ( X.,././././.",
"= = * e ( / ~ / ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ W W W ! ( ( ( W W E U P Y W ( X.,.d./././././././././.",
"= = * e ( W ~ ~ ~ ~ W ! ~ W ~ W ~ ( ( / ^ W W U Y P W ( X.,.d./././././././././././././././.",
"8 $ * e ( W ~ ~ ~ ! ( ( ( / ( W R Y Y Y R ( X.>.d./././././././././././././././././././././.",
"/.d . y ^ / / / ( W Y Y P P W ( X.>.d./././././././././././././././././././././././././././.",
"/./.h : ^ R R R W ( X.<.f./././././././././././././././././././././././././././././././././.",
"/././.] _ *.3./././././././././././././././././././././././././././././././././././././././."
};

class EvalMessageBox : public QDialog
{
public:
    EvalMessageBox(bool expired)
    {
        setWindowTitle(QLatin1String(" "));

        QString str = expired ? QLatin1String(boilerplate_expired) : qt_eval_string();
        str = str.trimmed();

        QFrame *border = new QFrame(this);

        QLabel *pixmap_label = new QLabel(border);
        pixmap_label->setPixmap(QPixmap(qtlogo_eval_xpm));
        pixmap_label->setAlignment(Qt::AlignTop);

        QLabel *text_label = new QLabel(str, border);

        QHBoxLayout *pm_and_text_layout = new QHBoxLayout();
        pm_and_text_layout->addWidget(pixmap_label);
        pm_and_text_layout->addWidget(text_label);

        QVBoxLayout *master_layout = new QVBoxLayout(border);
        master_layout->addLayout(pm_and_text_layout);

        QVBoxLayout *border_layout = new QVBoxLayout(this);
        border_layout->setMargin(0);
        border_layout->addWidget(border);

        if (expired) {
            QPushButton *cmd = new QPushButton(QLatin1String("OK"), border);
            cmd->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            cmd->setDefault(true);

            QHBoxLayout *button_layout = new QHBoxLayout();
            master_layout->addLayout(button_layout);
            button_layout->addWidget(cmd);

            connect(cmd, SIGNAL(clicked()), this, SLOT(close()));
        } else {
            border->setFrameShape(QFrame::WinPanel);
            border->setFrameShadow(QFrame::Raised);
            setParent(parentWidget(), Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
            QTimer::singleShot(7000, this, SLOT(close()));
            setAttribute(Qt::WA_DeleteOnClose);
            setAttribute(Qt::WA_QuitOnClose, false);
        }

        setFixedSize(sizeHint());
    }
};

class QGuiFuriCuri : public QCoreFuriCuri
{
public:
    void timerEvent(QTimerEvent *e) {
        if (e->timerId() == warn) {
            killTimer(warn);
            QMessageBox::information(0, QLatin1String("Automatic Timeout"), QLatin1String(will_shutdown_1min));
            kill = startTimer(KILL_DELAY);
        } else if (e->timerId() == kill) {
            killTimer(kill);
            QMessageBox::information(0, QLatin1String("Automatic Timeout"), QLatin1String(will_shutdown_now));
            qApp->quit();
        }
    }
};


void qt_gui_eval_init(QCoreApplicationPrivate::Type type)
{
    Q_UNUSED(type);

    if (!qt_eval_is_supported())
        return;

    if (qt_eval_is_expired()) {
        EvalMessageBox box(true);
        box.exec();
        ::exit(0);
    } else {
        Q_UNUSED(new QGuiFuriCuri());
    }
}

static QString qt_eval_title_prefix()
{
    return QLatin1String("[Qt Evaluation] ");
}

QString qt_eval_adapt_window_title(const QString &title)
{
    if (!qt_eval_is_supported())
        return title;
    return qt_eval_title_prefix() + title;
}

void qt_eval_init_widget(QWidget *w)
{
    if (!qt_eval_is_supported())
        return;
    if (w->isTopLevel() && w->windowTitle().isEmpty() && w->windowType() != Qt::Desktop ) {
        w->setWindowTitle(QLatin1String(" "));
    }
}
#endif

QT_END_NAMESPACE
