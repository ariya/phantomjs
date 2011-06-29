'''
  This file is part of the PyPhantomJS project.

  Copyright (C) 2011 James Roe <roejames12@hotmail.com>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
'''

import os
import codecs
import shutil

from PyQt4.QtCore import pyqtSlot, pyqtProperty, QObject, qDebug


class File(QObject):
    def __init__(self, openfile, parent=None):
        QObject.__init__(self, parent)

        self.m_file = openfile

    def __del__(self):
        self.m_file.close()

    @pyqtSlot(result=bool)
    def atEnd(self):
        # Note: On Windows, tell() can return illegal values (after an fgets())
        # when reading files with Unix-style line-endings. Use binary mode ('rb')
        # to circumvent this problem.
        currentPos = self.m_file.tell()

        try:
            line = self.m_file.readline()
            self.m_file.seek(currentPos)
        except IOError as e:
            qDebug('File.atEnd - %s: \'%s\'' % (e, self.m_file.name))
            return False

        # test for EOF
        if line == '':
            return True
        return False

    @pyqtSlot()
    def close(self):
        self.m_file.flush()
        self.m_file.close()
        del self.m_file
        self.deleteLater()

    @pyqtSlot()
    def flush(self):
        self.m_file.flush()

    @pyqtSlot(result=str)
    def read(self):
        try:
            return self.m_file.read()
        except IOError as e:
            qDebug('File.read - %s: \'%s\'' % (e, self.m_file.name))
            return ''

    @pyqtSlot(result=str)
    def readLine(self):
        try:
            return self.m_file.readline()
        except IOError as e:
            qDebug('File.readLine - %s: \'%s\'' % (e, self.m_file.name))
            return ''

    @pyqtSlot(str, result=bool)
    def write(self, data):
        try:
            self.m_file.write(data)
            return True
        except IOError as e:
            qDebug('File.write - %s: \'%s\'' % (e, self.m_file.name))
            return False

    @pyqtSlot(str, result=bool)
    def writeLine(self, data):
        try:
            self.m_file.write(data + '\n')
            return True
        except IOError as e:
            qDebug('File.writeLine - %s: \'%s\'' % (e, self.m_file.name))
            return False


class FileSystem(QObject):
    def __init__(self, parent=None):
        QObject.__init__(self, parent)

    ##
    # Attributes
    ##

    @pyqtSlot(str, result=int)
    def size(self, path):
        try:
            return os.path.getsize(path)
        except OSError as (t, e):
            qDebug('FileSystem.size - %s: \'%s\'' % (e, path))
            return 0

    @pyqtSlot(str, result=int)
    def lastModified(self, path):
        try:
            return os.path.getmtime(path)
        except OSError as (t, e):
            qDebug('FileSystem.lastModified - %s: \'%s\'' % (e, path))
            return 0

    ##
    # Files / Directories
    ##

    @pyqtSlot(str, str, result=bool)
    def copy(self, source, target):
        try:
            shutil.copy2(source, target)
            return True
        except IOError as (t, e):
            qDebug('FileSystem.copy - %s: \'%s\' -> \'%s\'' % (e, source, target))
            return False

    @pyqtSlot(str, str, result=bool)
    def move(self, source, target):
        try:
            shutil.move(source, target)
            return True
        except IOError as (t, e):
            qDebug('FileSystem.move - %s: \'%s\' -> \'%s\'' % (e, source, target))
            return False

    @pyqtSlot(str, result=bool)
    def remove(self, path):
        try:
            os.remove(path)
            return True
        except OSError as e:
            qDebug('FileSystem.remove - %s: \'%s\'' % (e, path))
            return False

    @pyqtSlot(str, str, result=bool)
    def rename(self, source, target):
        try:
            os.rename(source, target)
            return True
        except OSError as (t, e):
            qDebug('FileSystem.rename - %s: \'%s\' -> \'%s\'' % (e, source, target))
            return False

    ##
    # Directories
    ##

    @pyqtSlot(str, str, result=bool)
    def copyTree(self, source, target):
        try:
            shutil.copytree(source, target)
            return True
        except IOError as (t, e):
            qDebug('FileSystem.copyTree - %s: \'%s\' -> \'%s\'' % (e, source, target))
            return False

    @pyqtSlot(str, result=bool)
    def makeDirectory(self, path):
        try:
            os.mkdir(path)
            return True
        except OSError as e:
            qDebug('FileSystem.makeDirectory - %s: \'%s\'' % (e, path))
            return False

    @pyqtSlot(str, result=bool)
    def makeTree(self, path):
        try:
            os.makedirs(path)
            return True
        except OSError as e:
            qDebug('FileSystem.makeTree - %s: \'%s\'' % (e, path))
            return False

    @pyqtSlot(str, result=bool)
    def removeDirectory(self, path):
        try:
            os.rmdir(path)
            return True
        except OSError as e:
            qDebug('FileSystem.removeDirectory - %s: \'%s\'' % (e, path))
            return False

    @pyqtSlot(str, result=bool)
    def removeTree(self, path):
        try:
            os.removedirs(path)
            return True
        except OSError as e:
            qDebug('FileSystem.removeTree - %s: \'%s\'' % (e, path))
            return False

    ##
    # Files
    ##

    @pyqtSlot(str, str, result=File)
    def _open(self, path, mode):
        try:
            f = codecs.open(path, mode, encoding='utf-8')
            return File(f, self)
        except (IOError, ValueError) as (t, e):
            qDebug('FileSystem.open - %s: \'%s\'' % (e, path))
            return

    @pyqtSlot(str, result=str)
    def read(self, path):
        try:
            with codecs.open(path, 'r', encoding='utf-8') as f:
                content = f.read()
            return content
        except IOError as (t, e):
            qDebug('FileSystem.read - %s: \'%s\'' % (e, path))
            return

    @pyqtSlot(str, str, result=bool)
    @pyqtSlot(str, str, str, result=bool)
    def write(self, path, data, mode='w'):
        try:
            with codecs.open(path, mode, encoding='utf-8') as f:
                f.write(data)
            return True
        except IOError as (t, e):
            qDebug('FileSystem.write - %s: \'%s\'' % (e, path))
            return False

    ##
    # Listing
    ##

    @pyqtSlot(str, name='list', result='QStringList')
    def _list(self, path):
        try:
            p = os.listdir(path)
            p[0:2] = ('.', '..')
            return p
        except OSError as e:
            qDebug('FileSystem.list - %s: \'%s\'' % (e, path))
            return

    @pyqtSlot(str, result='QStringList')
    def listTree(self, path):
        try:
            listing = []
            for root, dirs, files in os.walk(path):
                for _file in files:
                    listing.append(os.path.join(root, _file))
            return listing
        except OSError as (t, e):
            qDebug('FileSystem.listTree - %s: \'%s\'' % (e, path))
            return

    @pyqtSlot(str, result='QStringList')
    def listDirectoryTree(self, path):
        try:
            listing = []
            for root, dirs, files in os.walk(path):
                for _dir in dirs:
                    listing.append(os.path.join(root, _dir))
            return listing
        except OSError as (t, e):
            qDebug('FileSystem.listDirectoryTree - %s: \'%s\'' % (e, path))
            return

    ##
    # Links
    ##

    @pyqtSlot(str, str, result=bool)
    def symbolicLink(self, source, target):
        try:
            os.symlink(source, target)
            return True
        except OSError as (t, e):
            qDebug('FileSystem.symbolicLink - %s: \'%s\' -> \'%s\'' % (e, source, target))
            return False

    @pyqtSlot(str, str, result=bool)
    def hardLink(self, source, target):
        try:
            os.link(source, target)
            return True
        except OSError as (t, e):
            qDebug('FileSystem.hardLink - %s: \'%s\' -> \'%s\'' % (e, source, target))
            return False

    @pyqtSlot(str, result=str)
    def readLink(self, path):
        try:
            return os.readlink(path)
        except OSError as (t, e):
            qDebug('FileSystem.readLink - %s: \'%s\'' % (e, path))
            return

    ##
    # Paths
    ##

    @pyqtSlot(str, result=str)
    def absolute(self, path):
        return os.path.abspath(path)

    @pyqtSlot(str, result=str)
    @pyqtSlot(str, str, result=str)
    def base(self, path, ext=None):
        if not ext:
            return os.path.basename(path)
        else:
            base = os.path.splitext(os.path.basename(path))
            if ext == base[1][1:]:
                return base[0]
            return ''.join(base)

    @pyqtSlot(str, result=str)
    def canonical(self, path):
        return os.path.realpath(path)

    @pyqtSlot(str, result=bool)
    def changeWorkingDirectory(self, path):
        try:
            os.chdir(path)
            return True
        except OSError as e:
            qDebug('FileSystem.changeWorkingDirectory - %s: \'%s\'' % (e, path))
            return False

    @pyqtSlot(str, result=str)
    def directory(self, path):
        return os.path.dirname(path)

    @pyqtSlot(str, result=str)
    def extension(self, path):
        return os.path.splitext(os.path.basename(path))[1][1:]

    @pyqtSlot(str, str, result=str)
    def join(self, pathA, pathB):
        return os.path.join(pathA, pathB)

    @pyqtSlot(str, result=str)
    def normal(self, path):
        return os.path.normpath(path)

    @pyqtProperty(str)
    def workingDirectory(self):
        return os.getcwdu()

    @pyqtSlot(str, result=str)
    @pyqtSlot(str, str, result=str)
    def relative(self, source, target=None):
        if not target:
            return os.path.relpath(source)
        else:
            return os.path.relpath(source, target)

    @pyqtProperty(str)
    def separator(self):
        return os.sep

    @pyqtSlot(str, result='QStringList')
    def split(self, path):
        spli = path.split(os.sep)
        if not spli:
            spli = path.split(os.altsep)
        return spli

    ##
    # Tests
    ##

    @pyqtSlot(str, result=bool)
    def exists(self, path):
        return os.path.exists(path)

    @pyqtSlot(str, result=bool)
    def isDirectory(self, path):
        return os.path.isdir(path)

    @pyqtSlot(str, result=bool)
    def isFile(self, path):
        return os.path.isfile(path)

    @pyqtSlot(str, result=bool)
    def isLink(self, path):
        return os.path.islink(path)

    @pyqtSlot(str, result=bool)
    def isMount(self, path):
        return os.path.ismount(path)

    @pyqtSlot(str, result=bool)
    def isReadable(self, path):
        return os.access(path, os.R_OK)

    @pyqtSlot(str, result=bool)
    def isWritable(self, path):
        return os.access(path, os.W_OK)

    @pyqtSlot(str, result=bool)
    def isExecutable(self, path):
        return os.access(path, os.X_OK)

    @pyqtSlot(str, result=bool)
    def same(self, pathA, pathB):
        return os.path.samefile(pathA, pathB)
