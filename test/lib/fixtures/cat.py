# Platform-agnostic replacement for 'cat' with no arguments.
# Avoids 'print' to achieve Python 2/3 agnosticism as well.
import sys
sys.stdout.write(sys.stdin.read())
