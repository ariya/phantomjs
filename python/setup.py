from setuptools import setup, find_packages
from phantom import __version__

README = open('README').read()

setup(name='phantom.py',
      version=__version__,
      description='Headless WebKit with Python API',
      long_description=README,
      author='Author name',
      author_email='author-email',
      packages=find_packages(),
      include_package_data=True,
      install_requires=['argparse',]
      )
