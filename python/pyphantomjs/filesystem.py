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

import codecs
import os
import shutil
import stat
try:
    from grp import getgrgid, getgrnam
    from pwd import getpwuid, getpwnam
except ImportError:
    pass

from PyQt4.QtCore import (pyqtProperty, pyqtSlot, QDateTime, qDebug,
                          QFileInfo, QObject)

from plugincontroller import do_action


class File(QObject):
    def __init__(self, parent, openfile):
        super(File, self).__init__(parent)

        self.m_file = openfile

        do_action('FileInit')

    def __del__(self):
        self.m_file.close()

    @pyqtSlot(result=bool)
    def atEnd(self):
        # :WARNING: On Windows, tell() can return illegal values (after an fgets())
        # when reading files with Unix-style line-endings. Use binary mode ('rb')
        # to circumvent this problem.
        currentPos = self.m_file.tell()

        try:
            line = self.m_file.readline()
            self.m_file.seek(currentPos)
        except IOError as e:
            qDebug("File.atEnd - %s: '%s'" % (e, self.m_file.name))
            return False

        # test for EOF
        if line == '':
            return True
        return False

    @pyqtSlot()
    def close(self):
        self.m_file.flush()
        self.m_file.close()
        self.deleteLater()

    @pyqtSlot()
    def flush(self):
        self.m_file.flush()

    @pyqtSlot(result=str)
    def read(self):
        try:
            return self.m_file.read()
        except IOError as e:
            qDebug("File.read - %s: '%s'" % (e, self.m_file.name))
            return ''

    @pyqtSlot(result=str)
    def readLine(self):
        try:
            return self.m_file.readline()
        except IOError as e:
            qDebug("File.readLine - %s: '%s'" % (e, self.m_file.name))
            return ''

    @pyqtSlot(str, result=bool)
    def write(self, data):
        try:
            self.m_file.write(data)
            return True
        except IOError as e:
            qDebug("File.write - %s: '%s'" % (e, self.m_file.name))
            return False

    @pyqtSlot(str, result=bool)
    def writeLine(self, data):
        try:
            self.m_file.write(data + '\n')
            return True
        except IOError as e:
            qDebug("File.writeLine - %s: '%s'" % (e, self.m_file.name))
            return False

    do_action('File')


class FileSystem(QObject):
    _instance = None
    def __new__(cls, *args, **kwargs):
        if cls._instance is None:
            cls._instance = super(FileSystem, cls).__new__(cls, *args, **kwargs)
        return cls._instance

    def __init__(self, parent):
        super(FileSystem, self).__init__(parent)

        do_action('FileSystemInit')

    ##
    # Attributes
    ##

    @pyqtSlot(str, result=int)
    def _size(self, path):
        try:
            return os.path.getsize(path)
        except OSError as (t, e):
            qDebug("FileSystem.size - %s: '%s'" % (e, path))
            return -1

    @pyqtSlot(str, result=QDateTime)
    def lastModified(self, path):
        fi = QFileInfo(path)
        if fi.exists():
            return fi.lastModified()
        else:
            err = 'No such file or directory'

        qDebug("FileSystem.lastModified - %s: '%s'" % (err, path))
        return QDateTime()

    ##
    # Files / Directories
    ##

    @pyqtSlot(str, str, result=bool)
    def _copy(self, source, target):
        try:
            shutil.copy2(source, target)
            return True
        except IOError as (t, e):
            qDebug("FileSystem.copy - %s: '%s' -> '%s'" % (e, source, target))
            return False

    @pyqtSlot(str, str, result=bool)
    def rename(self, source, target):
        try:
            os.rename(source, target)
            return True
        except OSError as (t, e):
            qDebug("FileSystem.rename - %s: '%s' -> '%s'" % (e, source, target))
            return False

    ##
    # Directories
    ##

    @pyqtSlot(str, str, result=bool)
    def _copyTree(self, source, target):
        try:
            shutil.copytree(source, target)
            return True
        except IOError as (t, e):
            qDebug("FileSystem.copyTree - %s: '%s' -> '%s'" % (e, source, target))
            return False

    @pyqtSlot(str, result=bool)
    def _removeDirectory(self, path):
        try:
            os.rmdir(path)
            return True
        except OSError as e:
            qDebug("FileSystem.removeDirectory - %s: '%s'" % (e, path))
            return False

    @pyqtSlot(str, result=bool)
    def _removeTree(self, path):
        try:
            shutil.rmtree(path)
            return True
        except OSError as e:
            qDebug("FileSystem.removeTree - %s: '%s'" % (e, path))
            return False

    @pyqtSlot(str, str, result=bool)
    def copyLinkTree(self, source, target):
        try:
            shutil.copytree(source, target, True)
            return True
        except IOError as (t, e):
            qDebug("FileSystem.copyLinkTree - %s: '%s' -> '%s'" % (e, source, target))
            return False

    @pyqtSlot(str, result=bool)
    def makeDirectory(self, path):
        try:
            os.mkdir(path)
            return True
        except OSError as e:
            qDebug("FileSystem.makeDirectory - %s: '%s'" % (e, path))
            return False

    @pyqtSlot(str, result=bool)
    def makeTree(self, path):
        try:
            os.makedirs(path)
            return True
        except OSError as e:
            qDebug("FileSystem.makeTree - %s: '%s'" % (e, path))
            return False

    ##
    # Files
    ##

    @pyqtSlot(str, str, result=File)
    def _open(self, path, mode):
        try:
            f = codecs.open(path, mode, encoding='utf-8')
            return File(self, f)
        except (IOError, ValueError) as (t, e):
            qDebug("FileSystem.open - %s: '%s'" % (e, path))
            return

    @pyqtSlot(str, result=bool)
    def _remove(self, path):
        try:
            os.remove(path)
            return True
        except OSError as e:
            qDebug("FileSystem.remove - %s: '%s'" % (e, path))
            return False

    @pyqtProperty(str)
    def newline(self):
        return os.linesep

    ##
    # Listing
    ##

    @pyqtSlot(str, name='list', result='QStringList')
    def list_(self, path):
        try:
            p = os.listdir(path)
            p[0:2] = ('.', '..')
            return p
        except OSError as e:
            qDebug("FileSystem.list - %s: '%s'" % (e, path))
            return

    @pyqtSlot(str, result='QStringList')
    def listTree(self, path):
        try:
            listing = []
            for root, dirs, files in os.walk(path):
                for file_ in files:
                    listing.append(os.path.join(root, file_))
            return listing
        except OSError as (t, e):
            qDebug("FileSystem.listTree - %s: '%s'" % (e, path))
            return

    @pyqtSlot(str, result='QStringList')
    def listDirectoryTree(self, path):
        try:
            listing = []
            for root, dirs, files in os.walk(path):
                for dir_ in dirs:
                    listing.append(os.path.join(root, dir_))
            return listing
        except OSError as (t, e):
            qDebug("FileSystem.listDirectoryTree - %s: '%s'" % (e, path))
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
            qDebug("FileSystem.symbolicLink - %s: '%s' -> '%s'" % (e, source, target))
            return False

    @pyqtSlot(str, str, result=bool)
    def hardLink(self, source, target):
        try:
            os.link(source, target)
            return True
        except OSError as (t, e):
            qDebug("FileSystem.hardLink - %s: '%s' -> '%s'" % (e, source, target))
            return False

    @pyqtSlot(str, result=str)
    def readLink(self, path):
        try:
            return os.readlink(path)
        except OSError as (t, e):
            qDebug("FileSystem.readLink - %s: '%s'" % (e, path))
            return ''

    ##
    # Paths
    ##

    @pyqtSlot(str, result=str)
    def absolute(self, path):
        return os.path.abspath(path)

    @pyqtSlot(str, result=str)
    @pyqtSlot(str, str, result=str)
    def base(self, path, ext=None):
        if ext is None:
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
            qDebug("FileSystem.changeWorkingDirectory - %s: '%s'" % (e, path))
            return False

    @pyqtSlot(str, result=str)
    def directory(self, path):
        return os.path.dirname(path)

    @pyqtSlot(str, result=str)
    def extension(self, path):
        return os.path.splitext(os.path.basename(path))[1][1:]

    @pyqtSlot('QStringList', result=str)
    def join(self, paths):
        return os.path.join(*paths)

    @pyqtSlot(str, result=str)
    def normal(self, path):
        return os.path.normpath(path)

    @pyqtSlot(str, result=str)
    def normalCase(self, path):
        return os.path.normcase(path)

    @pyqtProperty(str)
    def workingDirectory(self):
        return os.getcwdu()

    @pyqtSlot(str, result=str)
    @pyqtSlot(str, str, result=str)
    def relative(self, source, target=None):
        if target is None:
            return os.path.relpath(source)
        else:
            return os.path.relpath(source, target)

    @pyqtProperty(str)
    def separator(self):
        return os.sep

    @pyqtSlot(str, result='QStringList')
    def split(self, path):
        if os.sep in path:
            return path.split(os.sep)
        else:
            return path.split(os.altsep)

    ##
    # Permissions
    ##

    @pyqtSlot(str, str, result=bool)
    @pyqtSlot(str, int, result=bool)
    def changeGroup(self, path, group):
        try:
            if isinstance(group, int):
                os.chown(path, -1, group)
            else:
                os.chown(path, -1, getgrnam(group).gr_gid)
            return True
        except OSError as (t, e):
            qDebug("FileSystem.changeGroup - %s: '%s':'%s'" % (e, owner, path))
        except KeyError as e:
            qDebug("FileSystem.changeGroup - %s: '%s'" % (e.args[0], path))
        return False

    @pyqtSlot(str, str, result=bool)
    @pyqtSlot(str, int, result=bool)
    def changeLinkGroup(self, path, group):
        try:
            if isinstance(group, int):
                os.lchown(path, -1, group)
            else:
                os.lchown(path, -1, getgrnam(group).gr_gid)
            return True
        except OSError as (t, e):
            qDebug("FileSystem.changeLinkGroup - %s: '%s':'%s'" % (e, owner, path))
        except KeyError as e:
            qDebug("FileSystem.changeLinkGroup - %s: '%s'" % (e.args[0], path))
        return False

    @pyqtSlot(str, str, result=bool)
    @pyqtSlot(str, int, result=bool)
    def changeLinkOwner(self, path, owner):
        try:
            if isinstance(owner, int):
                os.lchown(path, owner, -1)
            else:
                os.lchown(path, getpwnam(owner).pw_uid, -1)
            return True
        except OSError as (t, e):
            qDebug("FileSystem.changeLinkOwner - %s: '%s':'%s'" % (e, owner, path))
        except KeyError as e:
            qDebug("FileSystem.changeLinkOwner - %s: '%s'" % (e.args[0], path))
        return False

    @pyqtSlot(str, int, result=bool)
    @pyqtSlot(str, 'QVariantMap', result=bool)
    def changeLinkPermissions(self, path, permissions):
        # permissions uses an object in 4 types: owner, group, others, special
        # owner,group,others each has 3 types, read,write,executable, contained in an array
        # special uses setuid,setgid,sticky
        #
        # In order to turn values on or off, just use true or false values.
        #
        # Permissions can alternatively be a numeric mode to chmod too.

        keys = {
            'owner': {'read': 'S_IRUSR', 'write': 'S_IWUSR', 'executable': 'S_IXUSR'},
            'group': {'read': 'S_IRGRP', 'write': 'S_IWGRP', 'executable': 'S_IXGRP'},
            'others': {'read': 'S_IROTH', 'write': 'S_IWOTH', 'executable': 'S_IXOTH'},
            'special': {'setuid': 'S_ISUID', 'setgid': 'S_ISGID', 'sticky': 'S_ISVTX'}
        }

        try:
            if isinstance(permissions, int):
                os.lchmod(path, permissions)
            else:
                bitnum = os.lstat(path).st_mode
                for section in permissions:
                    for key in permissions[section]:
                        try:
                            if permissions[section][key] is True:
                                bitnum = bitnum | stat.__dict__[keys[section][key]]
                            elif permissions[section][key] is False:
                                bitnum = bitnum & ~stat.__dict__[keys[section][key]]
                        except KeyError:
                            pass
                os.lchmod(path, bitnum)
            return True
        except OSError as (t, e):
            qDebug("FileSystem.changeLinkPermissions - %s: '%s'" % (e, path))
            return False

    @pyqtSlot(str, str, result=bool)
    @pyqtSlot(str, int, result=bool)
    def changeOwner(self, path, owner):
        try:
            if isinstance(owner, int):
                os.chown(path, owner, -1)
            else:
                os.chown(path, getpwnam(owner).pw_uid, -1)
            return True
        except OSError as (t, e):
            qDebug("FileSystem.changeOwner - %s: '%s':'%s'" % (e, owner, path))
        except KeyError as e:
            qDebug("FileSystem.changeOwner - %s: '%s'" % (e.args[0], path))
        return False

    @pyqtSlot(str, int, result=bool)
    @pyqtSlot(str, 'QVariantMap', result=bool)
    def changePermissions(self, path, permissions):
        # permissions uses an object in 4 types: owner, group, others, special
        # owner,group,others each has 3 types, read,write,executable, contained in an array
        # special uses setuid,setgid,sticky
        #
        # In order to turn values on or off, just use true or false values.
        #
        # Permissions can alternatively be a numeric mode to chmod too.

        keys = {
            'owner': {'read': 'S_IRUSR', 'write': 'S_IWUSR', 'executable': 'S_IXUSR'},
            'group': {'read': 'S_IRGRP', 'write': 'S_IWGRP', 'executable': 'S_IXGRP'},
            'others': {'read': 'S_IROTH', 'write': 'S_IWOTH', 'executable': 'S_IXOTH'},
            'special': {'setuid': 'S_ISUID', 'setgid': 'S_ISGID', 'sticky': 'S_ISVTX'}
        }

        try:
            if isinstance(permissions, int):
                os.chmod(path, permissions)
            else:
                bitnum = os.stat(path).st_mode
                for section in permissions:
                    for key in permissions[section]:
                        try:
                            if permissions[section][key] is True:
                                bitnum = bitnum | stat.__dict__[keys[section][key]]
                            elif permissions[section][key] is False:
                                bitnum = bitnum & ~stat.__dict__[keys[section][key]]
                        except KeyError:
                            pass
                os.chmod(path, bitnum)
            return True
        except OSError as (t, e):
            qDebug("FileSystem.changePermissions - %s: '%s'" % (e, path))
            return False

    @pyqtSlot(str, result='QVariant')
    def group(self, path):
        try:
            finfo = os.stat(path)
            return {
                'name': getgrgid(finfo.st_gid).gr_name,
                'uid': finfo.st_gid
            }
        except OSError as (t, e):
            qDebug("FileSystem.group - %s: '%s'" % (e, path))
            return

    @pyqtSlot(str, result='QVariant')
    def owner(self, path):
        try:
            finfo = os.stat(path)
            return {
                'name': getpwuid(finfo.st_uid).pw_name,
                'uid': finfo.st_uid
            }
        except OSError as (t, e):
            qDebug("FileSystem.owner - %s: '%s'" % (e, path))
            return

    @pyqtSlot(str, result='QVariant')
    def linkGroup(self, path):
        try:
            finfo = os.lstat(path)
            return {
                'name': getgrgid(finfo.st_gid).gr_name,
                'uid': finfo.st_gid
            }
        except OSError as (t, e):
            qDebug("FileSystem.linkGroup - %s: '%s'" % (e, path))
            return

    @pyqtSlot(str, result='QVariant')
    def linkOwner(self, path):
        try:
            finfo = os.lstat(path)
            return {
                'name': getpwuid(finfo.st_uid).pw_name,
                'uid': finfo.st_uid
            }
        except OSError as (t, e):
            qDebug("FileSystem.linkOwner - %s: '%s'" % (e, path))
            return

    @pyqtSlot(str, result='QVariant')
    def linkPermissions(self, path):
        # returns an object in 4 types: owner, group, others, special
        # owner,group,others each has 3 types, read,write,executable
        # special will return setuid,setgid,sticky

        try:
            finfo = os.lstat(path).st_mode

            # owner
            isOwnRd = bool(finfo & stat.S_IRUSR)
            isOwnWr = bool(finfo & stat.S_IWUSR)
            isOwnEx = bool(finfo & stat.S_IXUSR)
            # group
            isGrpRd = bool(finfo & stat.S_IRGRP)
            isGrpWr = bool(finfo & stat.S_IWGRP)
            isGrpEx = bool(finfo & stat.S_IXGRP)
            # others
            isOthRd = bool(finfo & stat.S_IROTH)
            isOthWr = bool(finfo & stat.S_IWOTH)
            isOthEx = bool(finfo & stat.S_IXOTH)
            # s*id
            isSUid = bool(finfo & stat.S_ISUID)
            isSGid = bool(finfo & stat.S_ISGID)
            # sticky
            isStick = bool(finfo & stat.S_ISVTX)

            return {
                'mode': oct(finfo)[-4:],
                'owner': {'read': isOwnRd, 'write': isOwnWr, 'executable': isOwnEx},
                'group': {'read': isGrpRd, 'write': isGrpWr, 'executable': isGrpEx},
                'others': {'read': isOthRd, 'write': isOthWr, 'executable': isOthEx},
                'special': {'setuid': isSUid, 'setgid': isSGid, 'sticky': isStick}
            }
        except OSError as (t, e):
            qDebug("FileSystem.linkPermissions - %s: '%s'" % (e, path))
            return

    @pyqtSlot(str, result='QVariant')
    def permissions(self, path):
        # returns an object in 4 types: owner, group, others, special
        # owner,group,others each has 3 types, read,write,executable
        # special will return setuid,setgid,sticky

        try:
            finfo = os.stat(path).st_mode

            # owner
            isOwnRd = bool(finfo & stat.S_IRUSR)
            isOwnWr = bool(finfo & stat.S_IWUSR)
            isOwnEx = bool(finfo & stat.S_IXUSR)
            # group
            isGrpRd = bool(finfo & stat.S_IRGRP)
            isGrpWr = bool(finfo & stat.S_IWGRP)
            isGrpEx = bool(finfo & stat.S_IXGRP)
            # others
            isOthRd = bool(finfo & stat.S_IROTH)
            isOthWr = bool(finfo & stat.S_IWOTH)
            isOthEx = bool(finfo & stat.S_IXOTH)
            # s*id
            isSUid = bool(finfo & stat.S_ISUID)
            isSGid = bool(finfo & stat.S_ISGID)
            # sticky
            isStick = bool(finfo & stat.S_ISVTX)

            return {
                'mode': oct(finfo)[-4:],
                'owner': {'read': isOwnRd, 'write': isOwnWr, 'executable': isOwnEx},
                'group': {'read': isGrpRd, 'write': isGrpWr, 'executable': isGrpEx},
                'others': {'read': isOthRd, 'write': isOthWr, 'executable': isOthEx},
                'special': {'setuid': isSUid, 'setgid': isSGid, 'sticky': isStick}
            }
        except OSError as (t, e):
            qDebug("FileSystem.permissions - %s: '%s'" % (e, path))
            return

    ##
    # Tests
    ##

    @pyqtSlot(str, result=bool)
    def exists(self, path):
        return os.path.exists(path)

    @pyqtSlot(str, result=bool)
    def isAbsolute(self, path):
        return os.path.isabs(os.path.splitdrive(path)[1])

    @pyqtSlot(str, result=bool)
    def isDirectory(self, path):
        return os.path.isdir(path)

    @pyqtSlot(str, result=bool)
    def isExecutable(self, path):
        return os.access(path, os.X_OK)

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
    def linkExists(self, path):
        return os.path.lexists(path)

    @pyqtSlot(str, result=bool)
    def same(self, pathA, pathB):
        return os.path.samefile(pathA, pathB)

    do_action('FileSystem')
