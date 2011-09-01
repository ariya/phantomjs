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
from setuptools import setup, find_packages

from pyphantomjs import __version__


def read(fname):
    return open(os.path.join(os.path.dirname(__file__), fname)).read()


README = read('README.md')
INSTALL = read('INSTALL.md')

index = README.find('LICENSING')
if index > -1:
    README = README[:index] + INSTALL + '\n\n' + README[index:]
else:
    README += INSTALL

try:
    import PyQt4
except ImportError:
    print ('\nWARNING: PyPhantomJS requires PyQt4, which was not found!\n'
           '           Refer to the README or INSTALL file for installation information.\n')


setup(
    name='PyPhantomJS',
    version=__version__,
    description='Minimalistic, headless, WebKit-based, JavaScript-driven tool',
    long_description=README,
    url = 'https://github.com/Roejames12/phantomjs',
    license = 'GNU General Public License (GPL)',
    author='James Roe',
    author_email='roejames12@hotmail.com',
    packages=find_packages(),
    include_package_data=True,
    install_requires=['argparse'],
    scripts = ['pyphantomjs/tools/pyphantomjs'],
    classifiers = [
        'Development Status :: 5 - Production/Stable',
        'Environment :: Console',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: GNU General Public License (GPL)',
        'Operating System :: OS Independent',
        'Programming Language :: Python',
        'Topic :: Internet :: WWW/HTTP :: Browsers'
    ],
    zip_safe=False
)
