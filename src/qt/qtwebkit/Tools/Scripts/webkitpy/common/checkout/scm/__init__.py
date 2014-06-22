# Required for Python to search this directory for module files

# We only export public API here.
from .commitmessage import CommitMessage
from .detection import SCMDetector
from .git import Git, AmbiguousCommitError
from .scm import SCM, AuthenticationError, CheckoutNeedsUpdate
from .svn import SVN
