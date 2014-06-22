#!/bin/sh
# Create a tarball from the subversion repository.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

tmp=$$
cd /tmp
svn checkout http://iexploder.googlecode.com/svn/trunk/ iexploder-$$
version=`grep '^\$VERSION' iexploder-$$/src/version.rb | cut -d\" -f2`
echo "Version: $version"
mv iexploder-$$ iexploder-$version
cd iexploder-$version
svn log > ChangeLog.txt
find . -name "*.pyc" -delete
find . -name ".svn" -exec rm -Rf {} \; 2>/dev/null
cd ..
GZIP="-9" tar -zcvf iexploder-${version}.tgz iexploder-${version}/
rm -Rf iexploder-${version}
