import os
from pyphantomjs import __version__
from setuptools import setup, find_packages

def read(fname):
    return open(os.path.join(os.path.dirname(__file__), fname)).read()

README = read('README.md')
INSTALL = '''
INSTALLING
-------------------
%s''' % read('INSTALL.md')

index = README.find('LICENSING')
if index > -1:
    README = README[:index] + INSTALL + '\n\n' + README[index:]
else:
    README += INSTALL

print README

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
