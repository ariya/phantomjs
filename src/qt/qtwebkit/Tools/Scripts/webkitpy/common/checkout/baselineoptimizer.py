# Copyright (C) 2011, Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import copy
import logging


_log = logging.getLogger(__name__)


# Yes, it's a hypergraph.
# FIXME: Should this function live with the ports somewhere?
# Perhaps this should move onto PortFactory?
def _baseline_search_hypergraph(host, port_names):
    hypergraph = {}

    # These edges in the hypergraph aren't visible on build.webkit.org,
    # but they impose constraints on how we optimize baselines.
    hypergraph.update(_VIRTUAL_PORTS)

    # FIXME: Should we get this constant from somewhere?
    fallback_path = ['LayoutTests']

    port_factory = host.port_factory
    for port_name in port_names:
        port = port_factory.get(port_name)
        webkit_base = port.webkit_base()
        search_path = port.baseline_search_path()
        if search_path:
            hypergraph[port_name] = [host.filesystem.relpath(path, webkit_base) for path in search_path] + fallback_path
    return hypergraph


_VIRTUAL_PORTS = {
    'mac-future': ['LayoutTests/platform/mac-future', 'LayoutTests/platform/mac', 'LayoutTests'],
    'win-future': ['LayoutTests/platform/win-future', 'LayoutTests/platform/win', 'LayoutTests'],
    'qt-unknown': ['LayoutTests/platform/qt-unknown', 'LayoutTests/platform/qt', 'LayoutTests'],
}


# FIXME: Should this function be somewhere more general?
def _invert_dictionary(dictionary):
    inverted_dictionary = {}
    for key, value in dictionary.items():
        if inverted_dictionary.get(value):
            inverted_dictionary[value].append(key)
        else:
            inverted_dictionary[value] = [key]
    return inverted_dictionary


class BaselineOptimizer(object):
    def __init__(self, host, port_names):
        self._host = host
        self._filesystem = self._host.filesystem
        self._scm = self._host.scm()
        self._hypergraph = _baseline_search_hypergraph(host, port_names)
        self._directories = reduce(set.union, map(set, self._hypergraph.values()))

    def read_results_by_directory(self, baseline_name):
        results_by_directory = {}
        for directory in self._directories:
            path = self._filesystem.join(self._scm.checkout_root, directory, baseline_name)
            if self._filesystem.exists(path):
                results_by_directory[directory] = self._filesystem.sha1(path)
        return results_by_directory

    def _results_by_port_name(self, results_by_directory):
        results_by_port_name = {}
        for port_name, search_path in self._hypergraph.items():
            for directory in search_path:
                if directory in results_by_directory:
                    results_by_port_name[port_name] = results_by_directory[directory]
                    break
        return results_by_port_name

    def _most_specific_common_directory(self, port_names):
        paths = [self._hypergraph[port_name] for port_name in port_names]
        common_directories = reduce(set.intersection, map(set, paths))

        def score(directory):
            return sum([path.index(directory) for path in paths])

        _, directory = sorted([(score(directory), directory) for directory in common_directories])[0]
        return directory

    def _filter_port_names_by_result(self, predicate, port_names_by_result):
        filtered_port_names_by_result = {}
        for result, port_names in port_names_by_result.items():
            filtered_port_names = filter(predicate, port_names)
            if filtered_port_names:
                filtered_port_names_by_result[result] = filtered_port_names
        return filtered_port_names_by_result

    def _place_results_in_most_specific_common_directory(self, port_names_by_result, results_by_directory):
        for result, port_names in port_names_by_result.items():
            directory = self._most_specific_common_directory(port_names)
            results_by_directory[directory] = result

    def _find_optimal_result_placement(self, baseline_name):
        results_by_directory = self.read_results_by_directory(baseline_name)
        results_by_port_name = self._results_by_port_name(results_by_directory)
        port_names_by_result = _invert_dictionary(results_by_port_name)

        new_results_by_directory = self._optimize_by_most_specific_common_directory(results_by_directory, results_by_port_name, port_names_by_result)
        if not new_results_by_directory:
            new_results_by_directory = self._optimize_by_pushing_results_up(results_by_directory, results_by_port_name, port_names_by_result)

        return results_by_directory, new_results_by_directory

    def _optimize_by_most_specific_common_directory(self, results_by_directory, results_by_port_name, port_names_by_result):
        new_results_by_directory = {}
        unsatisfied_port_names_by_result = port_names_by_result
        while unsatisfied_port_names_by_result:
            self._place_results_in_most_specific_common_directory(unsatisfied_port_names_by_result, new_results_by_directory)
            new_results_by_port_name = self._results_by_port_name(new_results_by_directory)

            def is_unsatisfied(port_name):
                return results_by_port_name[port_name] != new_results_by_port_name[port_name]

            new_unsatisfied_port_names_by_result = self._filter_port_names_by_result(is_unsatisfied, port_names_by_result)

            if len(new_unsatisfied_port_names_by_result.values()) >= len(unsatisfied_port_names_by_result.values()):
                return {}  # Frowns. We do not appear to be converging.
            unsatisfied_port_names_by_result = new_unsatisfied_port_names_by_result

        return new_results_by_directory

    def _optimize_by_pushing_results_up(self, results_by_directory, results_by_port_name, port_names_by_result):
        try:
            results_by_directory = results_by_directory
            best_so_far = results_by_directory
            while True:
                new_results_by_directory = copy.copy(best_so_far)
                for port_name in self._hypergraph.keys():
                    fallback_path = self._hypergraph[port_name]
                    current_index, current_directory = self._find_in_fallbackpath(fallback_path, results_by_port_name[port_name], best_so_far)
                    current_result = results_by_port_name[port_name]
                    for index in range(current_index + 1, len(fallback_path)):
                        new_directory = fallback_path[index]
                        if not new_directory in new_results_by_directory:
                            new_results_by_directory[new_directory] = current_result
                            if current_directory in new_results_by_directory:
                                del new_results_by_directory[current_directory]
                        elif new_results_by_directory[new_directory] == current_result:
                            if current_directory in new_results_by_directory:
                                del new_results_by_directory[current_directory]
                        else:
                            # The new_directory contains a different result, so stop trying to push results up.
                            break

                if len(new_results_by_directory) >= len(best_so_far):
                    # We've failed to improve, so give up.
                    break
                best_so_far = new_results_by_directory

            return best_so_far
        except KeyError as e:
            # FIXME: KeyErrors get raised if we're missing baselines. We should handle this better.
            return {}

    def _find_in_fallbackpath(self, fallback_path, current_result, results_by_directory):
        for index, directory in enumerate(fallback_path):
            if directory in results_by_directory and (results_by_directory[directory] == current_result):
                return index, directory
        assert False, "result %s not found in fallback_path %s, %s" % (current_result, fallback_path, results_by_directory)

    def _filtered_results_by_port_name(self, results_by_directory):
        results_by_port_name = self._results_by_port_name(results_by_directory)
        for port_name in _VIRTUAL_PORTS.keys():
            if port_name in results_by_port_name:
                del results_by_port_name[port_name]
        return results_by_port_name

    def _platform(self, filename):
        platform_dir = 'LayoutTests' + self._filesystem.sep + 'platform' + self._filesystem.sep
        if filename.startswith(platform_dir):
            return filename.replace(platform_dir, '').split(self._filesystem.sep)[0]
        platform_dir = self._filesystem.join(self._scm.checkout_root, platform_dir)
        if filename.startswith(platform_dir):
            return filename.replace(platform_dir, '').split(self._filesystem.sep)[0]
        return '(generic)'

    def _move_baselines(self, baseline_name, results_by_directory, new_results_by_directory):
        data_for_result = {}
        for directory, result in results_by_directory.items():
            if not result in data_for_result:
                source = self._filesystem.join(self._scm.checkout_root, directory, baseline_name)
                data_for_result[result] = self._filesystem.read_binary_file(source)

        file_names = []
        for directory, result in results_by_directory.items():
            if new_results_by_directory.get(directory) != result:
                file_names.append(self._filesystem.join(self._scm.checkout_root, directory, baseline_name))
        if file_names:
            _log.debug("    Deleting:")
            for platform_dir in sorted(self._platform(filename) for filename in file_names):
                _log.debug("      " + platform_dir)
            self._scm.delete_list(file_names)
        else:
            _log.debug("    (Nothing to delete)")

        file_names = []
        for directory, result in new_results_by_directory.items():
            if results_by_directory.get(directory) != result:
                destination = self._filesystem.join(self._scm.checkout_root, directory, baseline_name)
                self._filesystem.maybe_make_directory(self._filesystem.split(destination)[0])
                self._filesystem.write_binary_file(destination, data_for_result[result])
                file_names.append(destination)
        if file_names:
            _log.debug("    Adding:")
            for platform_dir in sorted(self._platform(filename) for filename in file_names):
                _log.debug("      " + platform_dir)
            self._scm.add_list(file_names)
        else:
            _log.debug("    (Nothing to add)")

    def directories_by_result(self, baseline_name):
        results_by_directory = self.read_results_by_directory(baseline_name)
        return _invert_dictionary(results_by_directory)

    def write_by_directory(self, results_by_directory, writer, indent):
        for path in sorted(results_by_directory):
            writer("%s%s: %s" % (indent, self._platform(path), results_by_directory[path][0:6]))

    def optimize(self, baseline_name):
        basename = self._filesystem.basename(baseline_name)
        results_by_directory, new_results_by_directory = self._find_optimal_result_placement(baseline_name)
        self.new_results_by_directory = new_results_by_directory
        if new_results_by_directory == results_by_directory:
            if new_results_by_directory:
                _log.debug("  %s: (already optimal)" % basename)
                self.write_by_directory(results_by_directory, _log.debug, "    ")
            else:
                _log.debug("  %s: (no baselines found)" % basename)
            return True
        if self._filtered_results_by_port_name(results_by_directory) != self._filtered_results_by_port_name(new_results_by_directory):
            _log.warning("  %s: optimization failed" % basename)
            self.write_by_directory(results_by_directory, _log.warning, "      ")
            return False

        _log.debug("  %s:" % basename)
        _log.debug("    Before: ")
        self.write_by_directory(results_by_directory, _log.debug, "      ")
        _log.debug("    After: ")
        self.write_by_directory(new_results_by_directory, _log.debug, "      ")

        self._move_baselines(baseline_name, results_by_directory, new_results_by_directory)
        return True
