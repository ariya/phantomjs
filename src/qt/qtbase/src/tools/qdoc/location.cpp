/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the tools applications of the Qt Toolkit.
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

#include <qdebug.h>
#include "config.h"
#include "location.h"
#include "generator.h"
#include <qdir.h>
#include <qregexp.h>
#include <stdlib.h>
#include <limits.h>

#include <stdio.h>

QT_BEGIN_NAMESPACE

QT_STATIC_CONST_IMPL Location Location::null;

int Location::tabSize;
QString Location::programName;
QRegExp *Location::spuriousRegExp = 0;
bool Location::logProgress_ = false;

/*!
  \class Location

  \brief The Location class provides a way to mark a location in a file.

  It maintains a stack of file positions. A file position
  consists of the file path, line number, and column number.
  The location is used for printing error messages that are
  tied to a location in a file.
 */

/*!
  Constructs an empty location.
 */
Location::Location()
    : stk(0), stkTop(&stkBottom), stkDepth(0), etcetera(false)
{
    // nothing.
}

/*!
  Constructs a location with (fileName, 1, 1) on its file
  position stack.
 */
Location::Location(const QString& fileName)
    : stk(0), stkTop(&stkBottom), stkDepth(0), etcetera(false)
{
    push(fileName);
}

/*!
  The copy constructor copies the contents of \a other into
  this Location using the assignment operator.
 */
Location::Location(const Location& other)
    : stk(0), stkTop(&stkBottom), stkDepth(0), etcetera(false)
{
    *this = other;
}

/*!
  The assignment operator does a deep copy of the entire
  state of \a other into this Location.
 */
Location& Location::operator=(const Location& other)
{
    QStack<StackEntry> *oldStk = stk;

    stkBottom = other.stkBottom;
    if (other.stk == 0) {
        stk = 0;
        stkTop = &stkBottom;
    }
    else {
        stk = new QStack<StackEntry>(*other.stk);
        stkTop = &stk->top();
    }
    stkDepth = other.stkDepth;
    etcetera = other.etcetera;
    delete oldStk;
    return *this;
}

/*!
  If the file position on top of the stack has a line number
  less than 1, set its line number to 1 and its column number
  to 1. Otherwise, do nothing.
  */
void Location::start()
{
    if (stkTop->lineNo < 1) {
        stkTop->lineNo = 1;
        stkTop->columnNo = 1;
    }
}

/*!
  Advance the current file position, using \a ch to decide how to do
  that. If \a ch is a \c{'\\n'}, increment the current line number and
  set the column number to 1. If \ch is a \c{'\\t'}, increment to the
  next tab column. Otherwise, increment the column number by 1.

  The current file position is the one on top of the position stack.
 */
void Location::advance(QChar ch)
{
    if (ch == QLatin1Char('\n')) {
        stkTop->lineNo++;
        stkTop->columnNo = 1;
    }
    else if (ch == QLatin1Char('\t')) {
        stkTop->columnNo =
                1 + tabSize * (stkTop->columnNo + tabSize-1) / tabSize;
    }
    else {
        stkTop->columnNo++;
    }
}

/*!
  Pushes \a filePath onto the file position stack. The current
  file position becomes (\a filePath, 1, 1).

  \sa pop()
*/
void Location::push(const QString& filePath)
{
    if (stkDepth++ >= 1) {
        if (stk == 0)
            stk = new QStack<StackEntry>;
        stk->push(StackEntry());
        stkTop = &stk->top();
    }

    stkTop->filePath = filePath;
    stkTop->lineNo = INT_MIN;
    stkTop->columnNo = 1;
}

/*!
  Pops the top of the internal stack. The current file position
  becomes the next one in the new top of stack.

  \sa push()
*/
void Location::pop()
{
    if (--stkDepth == 0) {
        stkBottom = StackEntry();
    }
    else {
        stk->pop();
        if (stk->isEmpty()) {
            delete stk;
            stk = 0;
            stkTop = &stkBottom;
        }
        else {
            stkTop = &stk->top();
        }
    }
}

/*! \fn bool Location::isEmpty() const

  Returns \c true if there is no file name set yet; returns \c false
  otherwise. The functions filePath(), lineNo() and columnNo()
  must not be called on an empty Location object.
 */

/*! \fn const QString& Location::filePath() const
  Returns the current path and file name.
  Must not be called on an empty Location object.

  \sa lineNo(), columnNo()
 */

/*!
  Returns the file name part of the file path, ie the
  current file.  Must not be called on an empty Location
  object.
 */
QString Location::fileName() const
{
    QString fp = filePath();
    return fp.mid(fp.lastIndexOf('/') + 1);
}


/*!
  \brief  Returns \a path which is canonicalized and relative to the config file.

  QDir::relativeFilePath does not canonicalize the paths, so
  if the config file is located at qtbase\src\widgets\doc\qtwidgets.qdocconf
  and it has a reference to any ancestor folder (e.g. ".." or even "../doc")
 */
QString Location::canonicalRelativePath(const QString &path)
{
    QDir configFileDir(QDir::current());
    QDir dir(path);
    const QString canon = dir.canonicalPath();
    return configFileDir.relativeFilePath(canon);
}

/*! \fn int Location::lineNo() const
  Returns the current line number.
  Must not be called on an empty Location object.

  \sa filePath(), columnNo()
*/

/*! \fn int Location::columnNo() const
  Returns the current column number.
  Must not be called on an empty Location object.

  \sa filePath(), lineNo()
*/

/*!
  Writes \a message and \a detals to stderr as a formatted
  warning message. Does not write the message if qdoc is in
  the Prepare phase.
 */
void Location::warning(const QString& message, const QString& details) const
{
    if (!Generator::runPrepareOnly())
        emitMessage(Warning, message, details);
}

/*!
  Writes \a message and \a detals to stderr as a formatted
  error message. Does not write the message if qdoc is in
  the Prepare phase.
 */
void Location::error(const QString& message, const QString& details) const
{
    if (!Generator::runPrepareOnly())
        emitMessage(Error, message, details);
}

/*!
  Writes \a message and \a detals to stderr as a formatted
  error message and then exits the program. qdoc prints fatal
  errors in either phase (Prepare or Generate).
 */
void Location::fatal(const QString& message, const QString& details) const
{
    emitMessage(Error, message, details);
    information(message);
    information(details);
    information("Aborting");
    exit(EXIT_FAILURE);
}

/*!
  Gets several parameters from the \a config, including
  tab size, program name, and a regular expression that
  appears to be used for matching certain error messages
  so that emitMessage() can avoid printing them.
 */
void Location::initialize(const Config& config)
{
    tabSize = config.getInt(CONFIG_TABSIZE);
    programName = config.programName();

    QRegExp regExp = config.getRegExp(CONFIG_SPURIOUS);
    if (regExp.isValid()) {
        spuriousRegExp = new QRegExp(regExp);
    }
    else {
        config.lastLocation().warning(tr("Invalid regular expression '%1'")
                                      .arg(regExp.pattern()));
    }
}

/*!
  Apparently, all this does is delete the regular expression
  used for intercepting certain error messages that should
  not be emitted by emitMessage().
 */
void Location::terminate()
{
    delete spuriousRegExp;
    spuriousRegExp = 0;
}

/*!
  Prints \a message to \c stdout followed by a \c{'\n'}.
 */
void Location::information(const QString& message)
{
    printf("%s\n", message.toLatin1().data());
    fflush(stdout);
}

/*!
  Prints \a message to \c stderr followed by a \c{'\n'},
  but only if the -log-progress option is set.
 */
void Location::logToStdErr(const QString& message)
{
    if (logProgress_) {
        fprintf(stderr, "LOG: %s\n", message.toLatin1().data());
        fflush(stderr);
    }
}

/*!
  Report a program bug, including the \a hint.
 */
void Location::internalError(const QString& hint)
{
    Location::null.fatal(tr("Internal error (%1)").arg(hint),
                         tr("There is a bug in %1. Seek advice from your local"
                            " %2 guru.")
                         .arg(programName).arg(programName));
}

/*!
  Formats \a message and \a details into a single string
  and outputs that string to \c stderr. \a type specifies
  whether the \a message is an error or a warning.
 */
void Location::emitMessage(MessageType type,
                           const QString& message,
                           const QString& details) const
{
    if (type == Warning &&
            spuriousRegExp != 0 &&
            spuriousRegExp->exactMatch(message))
        return;

    QString result = message;
    if (!details.isEmpty())
        result += "\n[" + details + QLatin1Char(']');
    result.replace("\n", "\n    ");
    if (type == Error)
        result.prepend(tr(": error: "));
    else if (type == Warning)
        result.prepend(tr(": warning: "));
    result.prepend(toString());
    fprintf(stderr, "%s\n", result.toLatin1().data());
    fflush(stderr);
}

/*!
  Converts the location to a string to be prepended to error
  messages.
 */
QString Location::toString() const
{
    QString str;

    if (isEmpty()) {
        str = programName;
    } else {
        Location loc2 = *this;
        loc2.setEtc(false);
        loc2.pop();
        if (!loc2.isEmpty()) {
            QString blah = tr("In file included from ");
            for (;;) {
                str += blah;
                str += loc2.top();
                loc2.pop();
                if (loc2.isEmpty())
                    break;
                str += tr(",");
                str += QLatin1Char('\n');
                blah.fill(' ');
            }
            str += tr(":");
            str += QLatin1Char('\n');
        }
        str += top();
    }
    return str;
}

QString Location::top() const
{
    QString str = filePath();
    if (!QDir::isAbsolutePath(str)) {
        QDir path(str);
        str = path.absolutePath();
    }
    if (lineNo() >= 1) {
        str += QLatin1Char(':');
        str += QString::number(lineNo());
    }
    if (etc())
        str += QLatin1String(" (etc.)");
    return str;
}

QT_END_NAMESPACE
