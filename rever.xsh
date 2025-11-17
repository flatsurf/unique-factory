#*********************************************************************
#  This file is part of unique-factory.
#
#        Copyright (C) 2025 Julian Rüth
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
#********************************************************************/

from rever.activity import activity
from rever.activities.command import command


try:
  input("Are you sure you are on the master branch which is identical to origin/master and the only pending changes are a version_info bump in configure.ac? [ENTER]")
except KeyboardInterrupt:
  sys.exit(1)

command('pixi', 'pixi install --manifest-path "$PWD/pyproject.toml" -e dev')

@activity
def dist():
    r"""
    Run make dist and collect the resulting tarball.
    """
    from tempfile import TemporaryDirectory
    from xonsh.dirstack import DIRSTACK
    with TemporaryDirectory() as tmp:
        ./bootstrap
        pushd @(tmp)
        @(DIRSTACK[-1])/configure
        make dist
        mv *.tar.gz @(DIRSTACK[-1])
        popd
    return True

$PROJECT = 'unique-factory'

$ACTIVITIES = [
    'version_bump',
    'changelog',
    'dist',
    'tag',
    'push_tag',
    'ghrelease',
]

$VERSION_BUMP_PATTERNS = [
    ('configure.ac', r'AC_INIT', r'AC_INIT([unique-factory], [$VERSION], [julian.rueth@fsfe.org])'),
]

$CHANGELOG_FILENAME = 'ChangeLog'
$CHANGELOG_TEMPLATE = 'TEMPLATE.rst'
$CHANGELOG_NEWS = 'doc/news'
$CHANGELOG_CATEGORIES = ('Added', 'Changed', 'Deprecated', 'Removed', 'Fixed', 'Performance')
$PUSH_TAG_REMOTE = 'git@github.com:flatsurf/unique-factory.git'

$GITHUB_ORG = 'flatsurf'
$GITHUB_REPO = 'unique-factory'

$GHRELEASE_ASSETS = ['unique-factory-' + $VERSION + '.tar.gz']
