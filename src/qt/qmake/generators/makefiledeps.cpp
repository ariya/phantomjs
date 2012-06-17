/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the qmake application of the Qt Toolkit.
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

#include "makefiledeps.h"
#include "option.h"
#include <qdir.h>
#include <qdatetime.h>
#include <qfileinfo.h>
#include <qbuffer.h>
#include <qplatformdefs.h>
#if defined(Q_OS_UNIX)
# include <unistd.h>
#else
# include <io.h>
#endif
#include <qdebug.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#if defined(_MSC_VER) && _MSC_VER >= 1400
#include <share.h>
#endif

QT_BEGIN_NAMESPACE

#if 1
#define qmake_endOfLine(c) (c == '\r' || c == '\n')
#else
inline bool qmake_endOfLine(const char &c) { return (c == '\r' || c == '\n'); }
#endif

//#define QMAKE_USE_CACHE

QMakeLocalFileName::QMakeLocalFileName(const QString &name) : is_null(name.isNull())
{
    if(!name.isEmpty()) {
        if(name.at(0) == QLatin1Char('"') && name.at(name.length()-2) == QLatin1Char('"'))
            real_name = name.mid(1, name.length()-2);
        else
            real_name = name;
    }
}
const QString
&QMakeLocalFileName::local() const
{
    if(!is_null && local_name.isNull())
        local_name = Option::fixPathToLocalOS(real_name, true);
    return local_name;
}

struct SourceDependChildren;
struct SourceFile {
    SourceFile() : deps(0), type(QMakeSourceFileInfo::TYPE_UNKNOWN),
                   mocable(0), traversed(0), exists(1),
                   moc_checked(0), dep_checked(0), included_count(0) { }
    ~SourceFile();
    QMakeLocalFileName file;
    SourceDependChildren *deps;
    QMakeSourceFileInfo::SourceFileType type;
    uint mocable : 1, traversed : 1, exists : 1;
    uint moc_checked : 1,  dep_checked : 1;
    uchar included_count;
};
struct SourceDependChildren {
    SourceFile **children;
    int num_nodes, used_nodes;
    SourceDependChildren() : children(0), num_nodes(0), used_nodes(0) { }
    ~SourceDependChildren() { if(children) free(children); children = 0; }
    void addChild(SourceFile *s) {
        if(num_nodes <= used_nodes) {
            num_nodes += 200;
            children = (SourceFile**)realloc(children, sizeof(SourceFile*)*(num_nodes));
        }
        children[used_nodes++] = s;
    }
};
SourceFile::~SourceFile() { delete deps; }
class SourceFiles {
    int hash(const char *);
public:
    SourceFiles();
    ~SourceFiles();

    SourceFile *lookupFile(const char *);
    inline SourceFile *lookupFile(const QString &f) { return lookupFile(f.toLatin1().constData()); }
    inline SourceFile *lookupFile(const QMakeLocalFileName &f) { return lookupFile(f.local().toLatin1().constData()); }
    void addFile(SourceFile *, const char *k=0, bool own=true);

    struct SourceFileNode {
        SourceFileNode() : key(0), next(0), file(0), own_file(1) { }
        ~SourceFileNode() {
            delete [] key;
            if(own_file)
                delete file;
        }
        char *key;
        SourceFileNode *next;
        SourceFile *file;
        uint own_file : 1;
    } **nodes;
    int num_nodes;
};
SourceFiles::SourceFiles()
{
    nodes = (SourceFileNode**)malloc(sizeof(SourceFileNode*)*(num_nodes=3037));
    for(int n = 0; n < num_nodes; n++)
        nodes[n] = 0;
}

SourceFiles::~SourceFiles()
{
    for(int n = 0; n < num_nodes; n++) {
        for(SourceFileNode *next = nodes[n]; next;) {
            SourceFileNode *next_next = next->next;
            delete next;
            next = next_next;
        }
    }
    free(nodes);
}

int SourceFiles::hash(const char *file)
{
    uint h = 0, g;
    while (*file) {
        h = (h << 4) + *file;
        if ((g = (h & 0xf0000000)) != 0)
            h ^= g >> 23;
        h &= ~g;
        file++;
    }
    return h;
}

SourceFile *SourceFiles::lookupFile(const char *file)
{
    int h = hash(file) % num_nodes;
    for(SourceFileNode *p = nodes[h]; p; p = p->next) {
        if(!strcmp(p->key, file))
            return p->file;
    }
    return 0;
}

void SourceFiles::addFile(SourceFile *p, const char *k, bool own_file)
{
    QByteArray ba = p->file.local().toLatin1();
    if(!k)
        k = ba;
    int h = hash(k) % num_nodes;
    SourceFileNode *pn = new SourceFileNode;
    pn->own_file = own_file;
    pn->key = qstrdup(k);
    pn->file = p;
    pn->next = nodes[h];
    nodes[h] = pn;
}

void QMakeSourceFileInfo::dependTreeWalker(SourceFile *node, SourceDependChildren *place)
{
    if(node->traversed || !node->exists)
        return;
    place->addChild(node);
    node->traversed = true; //set flag
    if(node->deps) {
        for(int i = 0; i < node->deps->used_nodes; i++)
            dependTreeWalker(node->deps->children[i], place);
    }
}

void QMakeSourceFileInfo::setDependencyPaths(const QList<QMakeLocalFileName> &l)
{
    // Ensure that depdirs does not contain the same paths several times, to minimize the stats
    QList<QMakeLocalFileName> ll;
    for (int i = 0; i < l.count(); ++i) {
        if (!ll.contains(l.at(i)))
            ll.append(l.at(i));
    }
    depdirs = ll;
}

QStringList QMakeSourceFileInfo::dependencies(const QString &file)
{
    QStringList ret;
    if(!files)
        return ret;

    if(SourceFile *node = files->lookupFile(QMakeLocalFileName(file))) {
        if(node->deps) {
            /* I stick them into a SourceDependChildren here because it is faster to just
               iterate over the list to stick them in the list, and reset the flag, then it is
               to loop over the tree (about 50% faster I saw) --Sam */
            SourceDependChildren place;
            for(int i = 0; i < node->deps->used_nodes; i++)
                dependTreeWalker(node->deps->children[i], &place);
            if(place.children) {
                for(int i = 0; i < place.used_nodes; i++) {
                    place.children[i]->traversed = false; //reset flag
                    ret.append(place.children[i]->file.real());
                }
           }
       }
    }
    return ret;
}

int
QMakeSourceFileInfo::included(const QString &file)
{
    if (!files)
        return 0;

    if(SourceFile *node = files->lookupFile(QMakeLocalFileName(file)))
        return node->included_count;
    return 0;
}

bool QMakeSourceFileInfo::mocable(const QString &file)
{
    if(SourceFile *node = files->lookupFile(QMakeLocalFileName(file)))
        return node->mocable;
    return false;
}

QMakeSourceFileInfo::QMakeSourceFileInfo(const QString &cf)
{
    //dep_mode
    dep_mode = Recursive;

    //quick project lookups
    includes = files = 0;
    files_changed = false;

    //buffer
    spare_buffer = 0;
    spare_buffer_size = 0;

    //cache
    cachefile = cf;
    if(!cachefile.isEmpty())
        loadCache(cachefile);
}

QMakeSourceFileInfo::~QMakeSourceFileInfo()
{
    //cache
    if(!cachefile.isEmpty() /*&& files_changed*/)
        saveCache(cachefile);

    //buffer
    if(spare_buffer) {
        free(spare_buffer);
        spare_buffer = 0;
        spare_buffer_size = 0;
    }

    //quick project lookup
    delete files;
    delete includes;
}

void QMakeSourceFileInfo::setCacheFile(const QString &cf)
{
    cachefile = cf;
    loadCache(cachefile);
}

void QMakeSourceFileInfo::addSourceFiles(const QStringList &l, uchar seek,
                                         QMakeSourceFileInfo::SourceFileType type)
{
    for(int i=0; i<l.size(); ++i)
        addSourceFile(l.at(i), seek, type);
}
void QMakeSourceFileInfo::addSourceFile(const QString &f, uchar seek,
                                        QMakeSourceFileInfo::SourceFileType type)
{
    if(!files)
        files = new SourceFiles;

    QMakeLocalFileName fn(f);
    SourceFile *file = files->lookupFile(fn);
    if(!file) {
        file = new SourceFile;
        file->file = fn;
        files->addFile(file);
    } else {
        if(file->type != type && file->type != TYPE_UNKNOWN && type != TYPE_UNKNOWN)
            warn_msg(WarnLogic, "%s is marked as %d, then %d!", f.toLatin1().constData(),
                     file->type, type);
    }
    if(type != TYPE_UNKNOWN)
        file->type = type;

    if(seek & SEEK_MOCS && !file->moc_checked)
        findMocs(file);
    if(seek & SEEK_DEPS && !file->dep_checked)
        findDeps(file);
}

bool QMakeSourceFileInfo::containsSourceFile(const QString &f, SourceFileType type)
{
    if(SourceFile *file = files->lookupFile(QMakeLocalFileName(f)))
        return (file->type == type || file->type == TYPE_UNKNOWN || type == TYPE_UNKNOWN);
    return false;
}

char *QMakeSourceFileInfo::getBuffer(int s) {
    if(!spare_buffer || spare_buffer_size < s)
        spare_buffer = (char *)realloc(spare_buffer, spare_buffer_size=s);
    return spare_buffer;
}

#ifndef S_ISDIR
#define S_ISDIR(x) (x & _S_IFDIR)
#endif

QMakeLocalFileName QMakeSourceFileInfo::fixPathForFile(const QMakeLocalFileName &f, bool)
{
    return f;
}

QMakeLocalFileName QMakeSourceFileInfo::findFileForDep(const QMakeLocalFileName &/*dep*/,
                                                       const QMakeLocalFileName &/*file*/)
{
    return QMakeLocalFileName();
}

QFileInfo QMakeSourceFileInfo::findFileInfo(const QMakeLocalFileName &dep)
{
    return QFileInfo(dep.real());
}

bool QMakeSourceFileInfo::findDeps(SourceFile *file)
{
    if(file->dep_checked || file->type == TYPE_UNKNOWN)
        return true;
    files_changed = true;
    file->dep_checked = true;

    const QMakeLocalFileName sourceFile = fixPathForFile(file->file, true);

    struct stat fst;
    char *buffer = 0;
    int buffer_len = 0;
    {
        int fd;
#if defined(_MSC_VER) && _MSC_VER >= 1400
        if (_sopen_s(&fd, sourceFile.local().toLatin1().constData(),
            _O_RDONLY, _SH_DENYNO, _S_IREAD) != 0)
            fd = -1;
#else
        fd = open(sourceFile.local().toLatin1().constData(), O_RDONLY);
#endif
        if(fd == -1 || fstat(fd, &fst) || S_ISDIR(fst.st_mode))
            return false;
        buffer = getBuffer(fst.st_size);
        for(int have_read = 0;
            (have_read = QT_READ(fd, buffer + buffer_len, fst.st_size - buffer_len));
            buffer_len += have_read) ;
        QT_CLOSE(fd);
    }
    if(!buffer)
        return false;
    if(!file->deps)
        file->deps = new SourceDependChildren;

    int line_count = 1;

    for(int x = 0; x < buffer_len; ++x) {
        bool try_local = true;
        char *inc = 0;
        if(file->type == QMakeSourceFileInfo::TYPE_UI) {
            // skip whitespaces
            while(x < buffer_len && (*(buffer+x) == ' ' || *(buffer+x) == '\t'))
                ++x;
            if(*(buffer + x) == '<') {
                ++x;
                if(buffer_len >= x + 12 && !strncmp(buffer + x, "includehint", 11) &&
                   (*(buffer + x + 11) == ' ' || *(buffer + x + 11) == '>')) {
                    for(x += 11; *(buffer + x) != '>'; ++x) ;
                    int inc_len = 0;
                    for(x += 1 ; *(buffer + x + inc_len) != '<'; ++inc_len) ;
                    *(buffer + x + inc_len) = '\0';
                    inc = buffer + x;
                } else if(buffer_len >= x + 13 && !strncmp(buffer + x, "customwidget", 12) &&
                          (*(buffer + x + 12) == ' ' || *(buffer + x + 12) == '>')) {
                    for(x += 13; *(buffer + x) != '>'; ++x) ; //skip up to >
                    while(x < buffer_len) {
                        for(x++; *(buffer + x) != '<'; ++x) ; //skip up to <
                        x++;
                        if(buffer_len >= x + 7 && !strncmp(buffer+x, "header", 6) &&
                           (*(buffer + x + 6) == ' ' || *(buffer + x + 6) == '>')) {
                            for(x += 7; *(buffer + x) != '>'; ++x) ; //skip up to >
                            int inc_len = 0;
                            for(x += 1 ; *(buffer + x + inc_len) != '<'; ++inc_len) ;
                            *(buffer + x + inc_len) = '\0';
                            inc = buffer + x;
                            break;
                        } else if(buffer_len >= x + 14 && !strncmp(buffer+x, "/customwidget", 13) &&
                                  (*(buffer + x + 13) == ' ' || *(buffer + x + 13) == '>')) {
                            x += 14;
                            break;
                        }
                    }
                } else if(buffer_len >= x + 8 && !strncmp(buffer + x, "include", 7) &&
                          (*(buffer + x + 7) == ' ' || *(buffer + x + 7) == '>')) {
                    for(x += 8; *(buffer + x) != '>'; ++x) {
                        if(buffer_len >= x + 9 && *(buffer + x) == 'i' &&
                           !strncmp(buffer + x, "impldecl", 8)) {
                            for(x += 8; *(buffer + x) != '='; ++x) ;
                            if(*(buffer + x) != '=')
                                continue;
                            for(++x; *(buffer+x) == '\t' || *(buffer+x) == ' '; ++x) ;
                            char quote = 0;
                            if(*(buffer+x) == '\'' || *(buffer+x) == '"') {
                                quote = *(buffer + x);
                                ++x;
                            }
                            int val_len;
                            for(val_len = 0; true; ++val_len) {
                                if(quote) {
                                    if(*(buffer+x+val_len) == quote)
                                        break;
                                } else if(*(buffer + x + val_len) == '>' ||
                                          *(buffer + x + val_len) == ' ') {
                                    break;
                                }
                            }
//?                            char saved = *(buffer + x + val_len);
                            *(buffer + x + val_len) = '\0';
                            if(!strcmp(buffer+x, "in implementation")) {
                                //### do this
                            }
                        }
                    }
                    int inc_len = 0;
                    for(x += 1 ; *(buffer + x + inc_len) != '<'; ++inc_len) ;
                    *(buffer + x + inc_len) = '\0';
                    inc = buffer + x;
                }
            }
            //read past new line now..
            for(; x < buffer_len && !qmake_endOfLine(*(buffer + x)); ++x) ;
            ++line_count;
        } else if(file->type == QMakeSourceFileInfo::TYPE_QRC) {
        } else if(file->type == QMakeSourceFileInfo::TYPE_C) {
            for(int beginning=1; x < buffer_len; ++x) {
                // whitespace comments and line-endings
                for(; x < buffer_len; ++x) {
                    if(*(buffer+x) == ' ' || *(buffer+x) == '\t') {
                        // keep going
                    } else if(*(buffer+x) == '/') {
                        ++x;
                        if(buffer_len >= x) {
                            if(*(buffer+x) == '/') { //c++ style comment
                                for(; x < buffer_len && !qmake_endOfLine(*(buffer + x)); ++x) ;
                                beginning = 1;
                            } else if(*(buffer+x) == '*') { //c style comment
                                for(++x; x < buffer_len; ++x) {
                                    if(*(buffer+x) == '*') {
                                        if(x+1 < buffer_len && *(buffer + (x+1)) == '/') {
                                            ++x;
                                            break;
                                        }
                                    } else if(qmake_endOfLine(*(buffer+x))) {
                                        ++line_count;
                                    }
                                }
                            }
                        }
                    } else if(qmake_endOfLine(*(buffer+x))) {
                        ++line_count;
                        beginning = 1;
                    } else {
                        break;
                    }
                }

                if(x >= buffer_len)
                    break;

                // preprocessor directive
                if(beginning && *(buffer+x) == '#')
                    break;

                // quoted strings
                if(*(buffer+x) == '\'' || *(buffer+x) == '"') {
                    const char term = *(buffer+(x++));
                    for(; x < buffer_len; ++x) {
                        if(*(buffer+x) == term) {
                            ++x;
                            break;
                        } else if(*(buffer+x) == '\\') {
                            ++x;
                        } else if(qmake_endOfLine(*(buffer+x))) {
                            ++line_count;
                        }
                    }
                }
                beginning = 0;
            }
            if(x >= buffer_len)
                break;

            //got a preprocessor symbol
            ++x;
            while(x < buffer_len) {
                if(*(buffer+x) != ' ' && *(buffer+x) != '\t')
                    break;
                ++x;
            }

            int keyword_len = 0;
            const char *keyword = buffer+x;
            while(x+keyword_len < buffer_len) {
                if(((*(buffer+x+keyword_len) < 'a' || *(buffer+x+keyword_len) > 'z')) &&
                   *(buffer+x+keyword_len) != '_') {
                    for(x+=keyword_len; //skip spaces after keyword
                        x < buffer_len && (*(buffer+x) == ' ' || *(buffer+x) == '\t');
                        x++) ;
                    break;
                } else if(qmake_endOfLine(*(buffer+x+keyword_len))) {
                    x += keyword_len-1;
                    keyword_len = 0;
                    break;
                }
                keyword_len++;
            }

            if((keyword_len == 7 && !strncmp(keyword, "include", 7)) // C & Obj-C
               || (keyword_len == 6 && !strncmp(keyword, "import", 6))) { // Obj-C
                char term = *(buffer + x);
                if(term == '<') {
                    try_local = false;
                    term = '>';
                } else if(term != '"') { //wtf?
                    continue;
                }
                x++;

                int inc_len;
                for(inc_len = 0; *(buffer + x + inc_len) != term && !qmake_endOfLine(*(buffer + x + inc_len)); ++inc_len) ;
                *(buffer + x + inc_len) = '\0';
                inc = buffer + x;
                x += inc_len;
            } else if(keyword_len == 13 && !strncmp(keyword, "qmake_warning", keyword_len)) {
                char term = 0;
                if(*(buffer + x) == '"')
                    term = '"';
                if(*(buffer + x) == '\'')
                    term = '\'';
                if(term)
                    x++;

                int msg_len;
                for(msg_len = 0; (term && *(buffer + x + msg_len) != term) &&
                              !qmake_endOfLine(*(buffer + x + msg_len)); ++msg_len) ;
                *(buffer + x + msg_len) = '\0';
                debug_msg(0, "%s:%d %s -- %s", file->file.local().toLatin1().constData(), line_count, keyword, buffer+x);
                x += msg_len;
            } else if(*(buffer+x) == '\'' || *(buffer+x) == '"') {
                const char term = *(buffer+(x++));
                while(x < buffer_len) {
                    if(*(buffer+x) == term)
                        break;
                    if(*(buffer+x) == '\\') {
                        x+=2;
                    } else {
                        if(qmake_endOfLine(*(buffer+x)))
                            ++line_count;
                        ++x;
                    }
                }
            } else {
                --x;
            }
        }

        if(inc) {
            if(!includes)
                includes = new SourceFiles;
            SourceFile *dep = includes->lookupFile(inc);
            if(!dep) {
                bool exists = false;
                QMakeLocalFileName lfn(inc);
                if(QDir::isRelativePath(lfn.real())) {
                    if(try_local) {
                        QDir sourceDir = findFileInfo(sourceFile).dir();
                        QMakeLocalFileName f(sourceDir.absoluteFilePath(lfn.local()));
                        if(findFileInfo(f).exists()) {
                            lfn = fixPathForFile(f);
                            exists = true;
                        }
                    }
                    if(!exists) { //path lookup
                        for(QList<QMakeLocalFileName>::Iterator it = depdirs.begin(); it != depdirs.end(); ++it) {
                            QMakeLocalFileName f((*it).real() + Option::dir_sep + lfn.real());
                            QFileInfo fi(findFileInfo(f));
                            if(fi.exists() && !fi.isDir()) {
                                lfn = fixPathForFile(f);
                                exists = true;
                                break;
                            }
                        }
                    }
                    if(!exists) { //heuristic lookup
                        lfn = findFileForDep(QMakeLocalFileName(inc), file->file);
                        if((exists = !lfn.isNull()))
                            lfn = fixPathForFile(lfn);
                    }
                } else {
                    exists = QFile::exists(lfn.real());
                }
                if(!lfn.isNull()) {
                    dep = files->lookupFile(lfn);
                    if(!dep) {
                        dep = new SourceFile;
                        dep->file = lfn;
                        dep->type = QMakeSourceFileInfo::TYPE_C;
                        files->addFile(dep);
                        includes->addFile(dep, inc, false);
                    }
                    dep->exists = exists;
                }
            }
            if(dep && dep->file != file->file) {
                dep->included_count++;
                if(dep->exists) {
                    debug_msg(5, "%s:%d Found dependency to %s", file->file.real().toLatin1().constData(),
                              line_count, dep->file.local().toLatin1().constData());
                    file->deps->addChild(dep);
                }
            }
        }
    }
    if(dependencyMode() == Recursive) { //done last because buffer is shared
        for(int i = 0; i < file->deps->used_nodes; i++) {
            if(!file->deps->children[i]->deps)
                findDeps(file->deps->children[i]);
        }
    }
    return true;
}

bool QMakeSourceFileInfo::findMocs(SourceFile *file)
{
    if(file->moc_checked)
        return true;
    files_changed = true;
    file->moc_checked = true;

    int buffer_len;
    char *buffer = 0;
    {
        struct stat fst;
        int fd;
#if defined(_MSC_VER) && _MSC_VER >= 1400
        if (_sopen_s(&fd, fixPathForFile(file->file, true).local().toLocal8Bit().constData(),
            _O_RDONLY, _SH_DENYRW, _S_IREAD) != 0)
            fd = -1;
#else
        fd = open(fixPathForFile(file->file, true).local().toLocal8Bit().constData(), O_RDONLY);
#endif
        if(fd == -1 || fstat(fd, &fst) || S_ISDIR(fst.st_mode))
            return false; //shouldn't happen
        buffer = getBuffer(fst.st_size);
        for(int have_read = buffer_len = 0;
            (have_read = QT_READ(fd, buffer + buffer_len, fst.st_size - buffer_len));
            buffer_len += have_read) ;
        QT_CLOSE(fd);
    }

    debug_msg(2, "findMocs: %s", file->file.local().toLatin1().constData());
    int line_count = 1;
    bool ignore_qobject = false, ignore_qgadget = false;
 /* qmake ignore Q_GADGET */
 /* qmake ignore Q_OBJECT */
    for(int x = 0; x < buffer_len; x++) {
        if(*(buffer + x) == '/') {
            ++x;
            if(buffer_len >= x) {
                if(*(buffer + x) == '/') { //c++ style comment
                    for(;x < buffer_len && !qmake_endOfLine(*(buffer + x)); ++x) ;
                } else if(*(buffer + x) == '*') { //c style comment
                    for(++x; x < buffer_len; ++x) {
                        if(*(buffer + x) == 't' || *(buffer + x) == 'q') { //ignore
                            if(buffer_len >= (x + 20) &&
                               !strncmp(buffer + x + 1, "make ignore Q_OBJECT", 20)) {
                                debug_msg(2, "Mocgen: %s:%d Found \"qmake ignore Q_OBJECT\"",
                                          file->file.real().toLatin1().constData(), line_count);
                                x += 20;
                                ignore_qobject = true;
                            } else if(buffer_len >= (x + 20) &&
                                      !strncmp(buffer + x + 1, "make ignore Q_GADGET", 20)) {
                                debug_msg(2, "Mocgen: %s:%d Found \"qmake ignore Q_GADGET\"",
                                          file->file.real().toLatin1().constData(), line_count);
                                x += 20;
                                ignore_qgadget = true;
                            }
                        } else if(*(buffer + x) == '*') {
                            if(buffer_len >= (x+1) && *(buffer + (x+1)) == '/') {
                                ++x;
                                break;
                            }
                        } else if(Option::debug_level && qmake_endOfLine(*(buffer + x))) {
                            ++line_count;
                        }
                    }
                }
            }
        } else if(*(buffer+x) == '\'' || *(buffer+x) == '"') {
            const char term = *(buffer+(x++));
            while(x < buffer_len) {
                if(*(buffer+x) == term)
                    break;
                if(*(buffer+x) == '\\') {
                    x+=2;
                } else {
                    if(qmake_endOfLine(*(buffer+x)))
                        ++line_count;
                    ++x;
                }
            }
        }
        if(Option::debug_level && qmake_endOfLine(*(buffer+x)))
            ++line_count;
        if(((buffer_len > x+2 &&  *(buffer+x+1) == 'Q' && *(buffer+x+2) == '_')
                   ||
            (buffer_len > x+4 &&  *(buffer+x+1) == 'Q' && *(buffer+x+2) == 'O'
                              &&  *(buffer+x+3) == 'M' && *(buffer+x+4) == '_'))
                   &&
                  *(buffer + x) != '_' &&
                  (*(buffer + x) < 'a' || *(buffer + x) > 'z') &&
                  (*(buffer + x) < 'A' || *(buffer + x) > 'Z') &&
                  (*(buffer + x) < '0' || *(buffer + x) > '9')) {
            ++x;
            int match = 0;
            static const char *interesting[] = { "OBJECT", "GADGET",
                                                 "M_OBJECT" };
            for(int interest = 0, m1, m2; interest < 3; ++interest) {
                if(interest == 0 && ignore_qobject)
                    continue;
                else if(interest == 1 && ignore_qgadget)
                    continue;
                for(m1 = 0, m2 = 0; *(interesting[interest]+m1); ++m1) {
                    if(*(interesting[interest]+m1) != *(buffer+x+2+m1)) {
                        m2 = -1;
                        break;
                    }
                    ++m2;
                }
                if(m1 == m2) {
                    match = m2 + 2;
                    break;
                }
            }
            if(match && *(buffer+x+match) != '_' &&
               (*(buffer+x+match) < 'a' || *(buffer+x+match) > 'z') &&
               (*(buffer+x+match) < 'A' || *(buffer+x+match) > 'Z') &&
               (*(buffer+x+match) < '0' || *(buffer+x+match) > '9')) {
                if(Option::debug_level) {
                    *(buffer+x+match) = '\0';
                    debug_msg(2, "Mocgen: %s:%d Found MOC symbol %s", file->file.real().toLatin1().constData(),
                              line_count, buffer+x);
                }
                file->mocable = true;
                return true;
            }
        }
    }
    return true;
}


void QMakeSourceFileInfo::saveCache(const QString &cf)
{
#ifdef QMAKE_USE_CACHE
    if(cf.isEmpty())
        return;

    QFile file(QMakeLocalFileName(cf).local());
    if(file.open(QIODevice::WriteOnly)) {
        QTextStream stream(&file);
        stream << qmake_version() << endl << endl; //version
        { //cache verification
            QMap<QString, QStringList> verify = getCacheVerification();
             stream << verify.count() << endl;
             for(QMap<QString, QStringList>::iterator it = verify.begin();
                 it != verify.end(); ++it) {
                 stream << it.key() << endl << it.value().join(";") << endl;
             }
             stream << endl;
        }
        if(files->nodes) {
            for(int file = 0; file < files->num_nodes; ++file) {
                for(SourceFiles::SourceFileNode *node = files->nodes[file]; node; node = node->next) {
                    stream << node->file->file.local() << endl; //source
                    stream << node->file->type << endl; //type

                    //depends
                    stream << ";";
                    if(node->file->deps) {
                        for(int depend = 0; depend < node->file->deps->used_nodes; ++depend) {
                            if(depend)
                                stream << ";";
                            stream << node->file->deps->children[depend]->file.local();
                        }
                    }
                    stream << endl;

                    stream << node->file->mocable << endl; //mocable
                    stream << endl; //just for human readability
                }
            }
        }
        stream.flush();
        file.close();
    }
#else
    Q_UNUSED(cf);
#endif
}

void QMakeSourceFileInfo::loadCache(const QString &cf)
{
    if(cf.isEmpty())
        return;

#ifdef QMAKE_USE_CACHE
    QMakeLocalFileName cache_file(cf);
    int fd = open(QMakeLocalFileName(cf).local().toLatin1(), O_RDONLY);
    if(fd == -1)
        return;
    QFileInfo cache_fi = findFileInfo(cache_file);
    if(!cache_fi.exists() || cache_fi.isDir())
        return;

    QFile file;
    if(!file.open(QIODevice::ReadOnly, fd))
        return;
    QTextStream stream(&file);

    if(stream.readLine() == qmake_version()) { //version check
        stream.skipWhiteSpace();

        bool verified = true;
        { //cache verification
            QMap<QString, QStringList> verify;
            int len = stream.readLine().toInt();
            for(int i = 0; i < len; ++i) {
                QString var = stream.readLine();
                QString val = stream.readLine();
                verify.insert(var, val.split(';', QString::SkipEmptyParts));
            }
            verified = verifyCache(verify);
        }
        if(verified) {
            stream.skipWhiteSpace();
            if(!files)
                files = new SourceFiles;
            while(!stream.atEnd()) {
                QString source = stream.readLine();
                QString type = stream.readLine();
                QString depends = stream.readLine();
                QString mocable = stream.readLine();
                stream.skipWhiteSpace();

                QMakeLocalFileName fn(source);
                QFileInfo fi = findFileInfo(fn);

                SourceFile *file = files->lookupFile(fn);
                if(!file) {
                    file = new SourceFile;
                    file->file = fn;
                    files->addFile(file);
                    file->type = (SourceFileType)type.toInt();
                    file->exists = fi.exists();
                }
                if(fi.exists() && fi.lastModified() < cache_fi.lastModified()) {
                    if(!file->dep_checked) { //get depends
                        if(!file->deps)
                            file->deps = new SourceDependChildren;
                        file->dep_checked = true;
                        QStringList depend_list = depends.split(";", QString::SkipEmptyParts);
                        for(int depend = 0; depend < depend_list.size(); ++depend) {
                            QMakeLocalFileName dep_fn(depend_list.at(depend));
                            QFileInfo dep_fi(findFileInfo(dep_fn));
                            SourceFile *dep = files->lookupFile(dep_fn);
                            if(!dep) {
                                dep = new SourceFile;
                                dep->file = dep_fn;
                                dep->exists = dep_fi.exists();
                                dep->type = QMakeSourceFileInfo::TYPE_UNKNOWN;
                                files->addFile(dep);
                            }
                            dep->included_count++;
                            file->deps->addChild(dep);
                        }
                    }
                    if(!file->moc_checked) { //get mocs
                        file->moc_checked = true;
                        file->mocable = mocable.toInt();
                    }
                }
            }
        }
    }
#endif
}

QMap<QString, QStringList> QMakeSourceFileInfo::getCacheVerification()
{
    return QMap<QString, QStringList>();
}

bool QMakeSourceFileInfo::verifyCache(const QMap<QString, QStringList> &v)
{
    return v == getCacheVerification();
}

QT_END_NAMESPACE
