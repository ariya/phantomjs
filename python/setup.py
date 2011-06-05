import os
from phantom import __version__
from setuptools import setup, find_packages

def read(fname):
    return open(os.path.join(os.path.dirname(__file__), fname)).read()

README = read('README')
README += '''
INSTALLING
-------------------
%s''' % read('INSTALL')

setup(name='pyphantomjs',
      version=__version__,
      description='Minimalistic, headless, WebKit-based, JavaScript-driven tool',
      long_description=README,
      author='Author name',
      author_email='author-email',
      packages=find_packages(),
      include_package_data=True,
      install_requires=['argparse',]
      )
