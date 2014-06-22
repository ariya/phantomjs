# Copyright (C) 2011 Igalia S.L.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

import errno
import logging
import os
import os.path
import subprocess
import sys


class GTKDoc(object):

    """Class that controls a gtkdoc run.

    Each instance of this class represents one gtkdoc configuration
    and set of documentation. The gtkdoc package is a series of tools
    run consecutively which converts inline C/C++ documentation into
    docbook files and then into HTML. This class is suitable for
    generating documentation or simply verifying correctness.

    Keyword arguments:
    output_dir         -- The path where gtkdoc output should be placed. Generation
                          may overwrite file in this directory. Required.
    module_name        -- The name of the documentation module. For libraries this
                          is typically the library name. Required if not library path
                          is given.
    source_dirs        -- A list of paths to the source code to be scanned. Required.
    ignored_files      -- A list of filenames to ignore in the source directory. It is
                          only necessary to provide the basenames of these files.
                          Typically it is important to provide an updated list of
                          ignored files to prevent warnings about undocumented symbols.
    decorator          -- If a decorator is used to unhide certain symbols in header
                          files this parameter is required for successful scanning.
                          (default '')
    deprecation_guard  -- gtkdoc tries to ensure that symbols marked as deprecated
                          are encased in this C preprocessor define. This is required
                          to avoid gtkdoc warnings. (default '')
    cflags             -- This parameter specifies any preprocessor flags necessary for
                          building the scanner binary during gtkdoc-scanobj. Typically
                          this includes all absolute include paths necessary to resolve
                          all header dependencies. (default '')
    ldflags            -- This parameter specifies any linker flags necessary for
                          building the scanner binary during gtkdoc-scanobj. Typically
                          this includes "-lyourlibraryname". (default '')
    library_path       -- This parameter specifies the path to the directory where you
                          library resides used for building the scanner binary during
                          gtkdoc-scanobj. (default '')

    doc_dir            -- The path to other documentation files necessary to build
                          the documentation. This files in this directory as well as
                          the files in the 'html' subdirectory will be copied
                          recursively into the output directory. (default '')
    main_sgml_file     -- The path or name (if a doc_dir is given) of the SGML file
                          that is the considered the main page of your documentation.
                          (default: <module_name>-docs.sgml)
    version            -- The version number of the module. If this is provided,
                          a version.xml file containing the version will be created
                          in the output directory during documentation generation.

    interactive        -- Whether or not errors or warnings should prompt the user
                          to continue or not. When this value is false, generation
                          will continue despite warnings. (default False)

    virtual_root       -- A temporary installation directory which is used as the root
                          where the actual installation prefix lives; this is mostly
                          useful for packagers, and should be set to what is given to
                          make install as DESTDIR.
    """

    def __init__(self, args):

        # Parameters specific to scanning.
        self.module_name = ''
        self.source_dirs = []
        self.ignored_files = []
        self.decorator = ''
        self.deprecation_guard = ''

        # Parameters specific to gtkdoc-scanobj.
        self.cflags = ''
        self.ldflags = ''
        self.library_path = ''

        # Parameters specific to generation.
        self.output_dir = ''
        self.doc_dir = ''
        self.main_sgml_file = ''

        # Parameters specific to gtkdoc-fixxref.
        self.cross_reference_deps = []

        self.interactive = False

        self.logger = logging.getLogger('gtkdoc')

        for key, value in iter(args.items()):
            setattr(self, key, value)

        def raise_error_if_not_specified(key):
            if not getattr(self, key):
                raise Exception('%s not specified.' % key)

        raise_error_if_not_specified('output_dir')
        raise_error_if_not_specified('source_dirs')
        raise_error_if_not_specified('module_name')

        # Make all paths absolute in case we were passed relative paths, since
        # we change the current working directory when executing subcommands.
        self.output_dir = os.path.abspath(self.output_dir)
        self.source_dirs = [os.path.abspath(x) for x in self.source_dirs]
        if self.library_path:
            self.library_path = os.path.abspath(self.library_path)

        if not self.main_sgml_file:
            self.main_sgml_file = self.module_name + "-docs.sgml"

    def generate(self, html=True):
        self.saw_warnings = False

        self._copy_doc_files_to_output_dir(html)
        self._write_version_xml()
        self._run_gtkdoc_scan()
        self._run_gtkdoc_scangobj()
        self._run_gtkdoc_mktmpl()
        self._run_gtkdoc_mkdb()

        if not html:
            return

        self._run_gtkdoc_mkhtml()
        self._run_gtkdoc_fixxref()

    def _delete_file_if_exists(self, path):
        if not os.access(path, os.F_OK | os.R_OK):
            return
        self.logger.debug('deleting %s', path)
        os.unlink(path)

    def _create_directory_if_nonexistent(self, path):
        try:
            os.makedirs(path)
        except OSError as error:
            if error.errno != errno.EEXIST:
                raise

    def _raise_exception_if_file_inaccessible(self, path):
        if not os.path.exists(path) or not os.access(path, os.R_OK):
            raise Exception("Could not access file at: %s" % path)

    def _output_has_warnings(self, outputs):
        for output in outputs:
            if output and output.find('warning'):
                return True
        return False

    def _ask_yes_or_no_question(self, question):
        if not self.interactive:
            return True

        question += ' [y/N] '
        answer = None
        while answer != 'y' and answer != 'n' and answer != '':
            answer = raw_input(question).lower()
        return answer == 'y'

    def _run_command(self, args, env=None, cwd=None, print_output=True, ignore_warnings=False):
        if print_output:
            self.logger.info("Running %s", args[0])
        self.logger.debug("Full command args: %s", str(args))

        process = subprocess.Popen(args, env=env, cwd=cwd,
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
        stdout, stderr = [b.decode("utf-8") for b in process.communicate()]

        if print_output:
            if stdout:
                sys.stdout.write(stdout)
            if stderr:
                sys.stderr.write(stderr)

        if process.returncode != 0:
            raise Exception('%s produced a non-zero return code %i'
                             % (args[0], process.returncode))

        if not ignore_warnings and ('warning' in stderr or 'warning' in stdout):
            self.saw_warnings = True
            if not self._ask_yes_or_no_question('%s produced warnings, '
                                                'try to continue?' % args[0]):
                raise Exception('%s step failed' % args[0])

        return stdout.strip()

    def _copy_doc_files_to_output_dir(self, html=True):
        if not self.doc_dir:
            self.logger.info('Not copying any files from doc directory,'
                             ' because no doc directory given.')
            return

        def copy_file_replacing_existing(src, dest):
            if os.path.isdir(src):
                self.logger.debug('skipped directory %s',  src)
                return
            if not os.access(src, os.F_OK | os.R_OK):
                self.logger.debug('skipped unreadable %s', src)
                return

            self._delete_file_if_exists(dest)

            self.logger.debug('created %s', dest)
            os.link(src, dest)

        def copy_all_files_in_directory(src, dest):
            for path in os.listdir(src):
                copy_file_replacing_existing(os.path.join(src, path),
                                             os.path.join(dest, path))

        self.logger.info('Copying template files to output directory...')
        self._create_directory_if_nonexistent(self.output_dir)
        copy_all_files_in_directory(self.doc_dir, self.output_dir)

        if not html:
            return

        self.logger.info('Copying HTML files to output directory...')
        html_src_dir = os.path.join(self.doc_dir, 'html')
        html_dest_dir = os.path.join(self.output_dir, 'html')
        self._create_directory_if_nonexistent(html_dest_dir)

        if os.path.exists(html_src_dir):
            copy_all_files_in_directory(html_src_dir, html_dest_dir)

    def _write_version_xml(self):
        if not self.version:
            self.logger.info('No version specified, so not writing version.xml')
            return

        version_xml_path = os.path.join(self.output_dir, 'version.xml')
        src_version_xml_path = os.path.join(self.doc_dir, 'version.xml')

        # Don't overwrite version.xml if it was in the doc directory.
        if os.path.exists(version_xml_path) and \
           os.path.exists(src_version_xml_path):
            return

        output_file = open(version_xml_path, 'w')
        output_file.write(self.version)
        output_file.close()

    def _ignored_files_basenames(self):
        return ' '.join([os.path.basename(x) for x in self.ignored_files])

    def _run_gtkdoc_scan(self):
        args = ['gtkdoc-scan',
                '--module=%s' % self.module_name,
                '--rebuild-types']

        # Each source directory should be have its own "--source-dir=" prefix.
        args.extend(['--source-dir=%s' % path for path in self.source_dirs])

        if self.decorator:
            args.append('--ignore-decorators=%s' % self.decorator)
        if self.deprecation_guard:
            args.append('--deprecated-guards=%s' % self.deprecation_guard)
        if self.output_dir:
            args.append('--output-dir=%s' % self.output_dir)

        # gtkdoc-scan wants the basenames of ignored headers, so strip the
        # dirname. Different from "--source-dir", the headers should be
        # specified as one long string.
        ignored_files_basenames = self._ignored_files_basenames()
        if ignored_files_basenames:
            args.append('--ignore-headers=%s' % ignored_files_basenames)

        self._run_command(args)

    def _run_gtkdoc_scangobj(self):
        env = os.environ
        ldflags = self.ldflags
        if self.library_path:
            ldflags = ' "-L%s" ' % self.library_path + ldflags
            current_ld_library_path = env.get('LD_LIBRARY_PATH')
            if current_ld_library_path:
                env['RUN'] = 'LD_LIBRARY_PATH="%s:%s" ' % (self.library_path, current_ld_library_path)
            else:
                env['RUN'] = 'LD_LIBRARY_PATH="%s" ' % self.library_path

        if ldflags:
            env['LDFLAGS'] = '%s %s' % (ldflags, env.get('LDFLAGS', ''))
        if self.cflags:
            env['CFLAGS'] = '%s %s' % (self.cflags, env.get('CFLAGS', ''))

        if 'CFLAGS' in env:
            self.logger.debug('CFLAGS=%s', env['CFLAGS'])
        if 'LDFLAGS' in env:
            self.logger.debug('LDFLAGS %s', env['LDFLAGS'])
        if 'RUN' in env:
            self.logger.debug('RUN=%s', env['RUN'])
        self._run_command(['gtkdoc-scangobj', '--module=%s' % self.module_name],
                          env=env, cwd=self.output_dir)

    def _run_gtkdoc_mktmpl(self):
        args = ['gtkdoc-mktmpl', '--module=%s' % self.module_name]
        self._run_command(args, cwd=self.output_dir)

    def _run_gtkdoc_mkdb(self):
        sgml_file = os.path.join(self.output_dir, self.main_sgml_file)
        self._raise_exception_if_file_inaccessible(sgml_file)

        args = ['gtkdoc-mkdb',
                '--module=%s' % self.module_name,
                '--main-sgml-file=%s' % sgml_file,
                '--source-suffixes=h,c,cpp,cc',
                '--output-format=xml',
                '--sgml-mode']

        ignored_files_basenames = self._ignored_files_basenames()
        if ignored_files_basenames:
            args.append('--ignore-files=%s' % ignored_files_basenames)

        # Each directory should be have its own "--source-dir=" prefix.
        args.extend(['--source-dir=%s' % path for path in self.source_dirs])
        self._run_command(args, cwd=self.output_dir)

    def _run_gtkdoc_mkhtml(self):
        html_dest_dir = os.path.join(self.output_dir, 'html')
        if not os.path.isdir(html_dest_dir):
            raise Exception("%s is not a directory, could not generate HTML"
                            % html_dest_dir)
        elif not os.access(html_dest_dir, os.X_OK | os.R_OK | os.W_OK):
            raise Exception("Could not access %s to generate HTML"
                            % html_dest_dir)

        # gtkdoc-mkhtml expects the SGML path to be absolute.
        sgml_file = os.path.join(os.path.abspath(self.output_dir),
                                 self.main_sgml_file)
        self._raise_exception_if_file_inaccessible(sgml_file)

        self._run_command(['gtkdoc-mkhtml', self.module_name, sgml_file],
                          cwd=html_dest_dir)

    def _run_gtkdoc_fixxref(self):
        args = ['gtkdoc-fixxref',
                '--module-dir=html',
                '--html-dir=html']
        args.extend(['--extra-dir=%s' % extra_dir for extra_dir in self.cross_reference_deps])
        self._run_command(args, cwd=self.output_dir, ignore_warnings=True)

    def rebase_installed_docs(self):
        if not os.path.isdir(self.output_dir):
            raise Exception("Tried to rebase documentation before generating it.")
        html_dir = os.path.join(self.virtual_root + self.prefix, 'share', 'gtk-doc', 'html', self.module_name)
        if not os.path.isdir(html_dir):
            return
        args = ['gtkdoc-rebase',
                '--relative',
                '--html-dir=%s' % html_dir]
        args.extend(['--other-dir=%s' % extra_dir for extra_dir in self.cross_reference_deps])
        if self.virtual_root:
            args.extend(['--dest-dir=%s' % self.virtual_root])
        self._run_command(args, cwd=self.output_dir)

    def api_missing_documentation(self):
        unused_doc_file = os.path.join(self.output_dir, self.module_name + "-unused.txt")
        if not os.path.exists(unused_doc_file) or not os.access(unused_doc_file, os.R_OK):
            return []
        return open(unused_doc_file).read().splitlines()

class PkgConfigGTKDoc(GTKDoc):

    """Class reads a library's pkgconfig file to guess gtkdoc parameters.

    Some gtkdoc parameters can be guessed by reading a library's pkgconfig
    file, including the cflags, ldflags and version parameters. If you
    provide these parameters as well, they will be appended to the ones
    guessed via the pkgconfig file.

    Keyword arguments:
      pkg_config_path -- Path to the pkgconfig file for the library. Required.
    """

    def __init__(self, pkg_config_path, args):
        super(PkgConfigGTKDoc, self).__init__(args)

        pkg_config = os.environ.get('PKG_CONFIG', 'pkg-config')

        if not os.path.exists(pkg_config_path):
            raise Exception('Could not find pkg-config file at: %s'
                            % pkg_config_path)

        self.cflags += " " + self._run_command([pkg_config,
                                                pkg_config_path,
                                                '--cflags'], print_output=False)
        self.ldflags += " " + self._run_command([pkg_config,
                                                pkg_config_path,
                                                '--libs'], print_output=False)
        self.version = self._run_command([pkg_config,
                                          pkg_config_path,
                                          '--modversion'], print_output=False)
        self.prefix = self._run_command([pkg_config,
                                         pkg_config_path,
                                         '--variable=prefix'], print_output=False)
