# Platform-agnostic replacement for 'echo -n'.
# Avoids 'print' to achieve Python 2/3 agnosticism as well.
import sys
sys.stdout.write(" ".join(sys.argv[1:]))
