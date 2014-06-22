# Copyright (C) 2010 Google Inc. All rights reserved.
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

# FIXME: Is this the right way to do this?
from webkitpy.tool.steps.addsvnmimetypeforpng import AddSvnMimetypeForPng
from webkitpy.tool.steps.applypatch import ApplyPatch
from webkitpy.tool.steps.applypatchwithlocalcommit import ApplyPatchWithLocalCommit
from webkitpy.tool.steps.applywatchlist import ApplyWatchList
from webkitpy.tool.steps.attachtobug import AttachToBug
from webkitpy.tool.steps.build import Build
from webkitpy.tool.steps.checkstyle import CheckStyle
from webkitpy.tool.steps.cleanworkingdirectory import CleanWorkingDirectory
from webkitpy.tool.steps.closebug import CloseBug
from webkitpy.tool.steps.closebugforlanddiff import CloseBugForLandDiff
from webkitpy.tool.steps.closepatch import ClosePatch
from webkitpy.tool.steps.commit import Commit
from webkitpy.tool.steps.confirmdiff import ConfirmDiff
from webkitpy.tool.steps.createbug import CreateBug
from webkitpy.tool.steps.discardlocalchanges import DiscardLocalChanges
from webkitpy.tool.steps.editchangelog import EditChangeLog
from webkitpy.tool.steps.ensurebugisopenandassigned import EnsureBugIsOpenAndAssigned
from webkitpy.tool.steps.ensurelocalcommitifneeded import EnsureLocalCommitIfNeeded
from webkitpy.tool.steps.haslanded import HasLanded
from webkitpy.tool.steps.obsoletepatches import ObsoletePatches
from webkitpy.tool.steps.options import Options
from webkitpy.tool.steps.postdiff import PostDiff
from webkitpy.tool.steps.postdiffforcommit import PostDiffForCommit
from webkitpy.tool.steps.postdiffforrevert import PostDiffForRevert
from webkitpy.tool.steps.preparechangelog import PrepareChangeLog
from webkitpy.tool.steps.preparechangelogforrevert import PrepareChangeLogForRevert
from webkitpy.tool.steps.promptforbugortitle import PromptForBugOrTitle
from webkitpy.tool.steps.reopenbugafterrollout import ReopenBugAfterRollout
from webkitpy.tool.steps.revertrevision import RevertRevision
from webkitpy.tool.steps.runtests import RunTests
from webkitpy.tool.steps.suggestreviewers import SuggestReviewers
from webkitpy.tool.steps.update import Update
from webkitpy.tool.steps.updatechangelogswithreviewer import UpdateChangeLogsWithReviewer
from webkitpy.tool.steps.validatechangelogs import ValidateChangeLogs
from webkitpy.tool.steps.validatereviewer import ValidateReviewer
