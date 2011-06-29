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

    @pyqtSlot(str, str, result=File)
    def _open(self, path, mode):
        try:
            f = codecs.open(path, mode, encoding='utf-8')
            return File(f, self)
        except (IOError, ValueError) as (t, e):
            qDebug('FileSystem.open - %s: \'%s\'' % (e, path))
            return

    @pyqtSlot(str, result=bool)
    def changeWorkingDirectory(self, path):
        try:
            os.chdir(path)
            return True
        except OSError as e:
            qDebug('FileSystem.changeWorkingDirectory - %s: \'%s\'' % (e, path))
            return False

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

    @pyqtSlot(str, name='list', result='QStringList')
    def _list(self, path):
        try:
            p = os.listdir(path)
            p[0:2] = ('.', '..')
            return p
        except OSError as e:
            qDebug('FileSystem.list - %s: \'%s\'' % (e, path))
            return

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
    def remove(self, path):
        try:
            os.remove(path)
            return True
        except OSError as e:
            qDebug('FileSystem.remove - %s: \'%s\'' % (e, path))
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

    @pyqtProperty(str)
    def separator(self):
        return os.sep

    @pyqtProperty(str)
    def workingDirectory(self):
        return os.getcwdu()
