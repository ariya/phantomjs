# libxkbcommon

xkbcommon is a keymap compiler and support library which processes a
reduced subset of keymaps as defined by the XKB specification.  Primarily,
a keymap is created from a set of Rules/Model/Layout/Variant/Options names,
processed through an XKB ruleset, and compiled into a struct xkb_keymap,
which is the base type for all xkbcommon operations.

From an xkb_keymap, an xkb_state object is created which holds the current
state of all modifiers, groups, LEDs, etc, relating to that keymap.  All
key events must be fed into the xkb_state object using xkb_state_update_key().
Once this is done, the xkb_state object will be properly updated, and the
keysyms to use can be obtained with xkb_state_key_get_syms().

libxkbcommon does not distribute a dataset itself, other than for testing
purposes.  The most common dataset is xkeyboard-config, as used by all
current distributions for their X11 XKB data.  More information on
xkeyboard-config is available here:
    http://www.freedesktop.org/wiki/Software/XKeyboardConfig

## Quick Guide

See [Quick Guide](doc/quick-guide.md).

## API

While xkbcommon's API is somewhat derived from the classic XKB API as found
in X11/extensions/XKB.h and friends, it has been substantially reworked to
expose fewer internal details to clients.  The supported API is available
in the xkbcommon/xkbcommon-*.h files.  Additional support is provided for
X11 (XCB) clients, in the xkbcommon-x11 library, xkbcommon/xkbcommon-x11.h.

The xkbcommon API and ABI are stable. We will attempt to not break ABI during
a minor release series, so applications written against 0.1.0 should be
completely compatible with 0.5.3, but not necessarily with 1.0.0.  However, new
symbols may be introduced in any release.  Thus, anyone packaging xkbcommon
should make sure any package depending on it depends on a release greater than
or equal to the version it was built against (or earlier, if it doesn't use
any newly-introduced symbols), but less than the next major release.

## Relation to X11

Relative to the XKB 1.1 specification implemented in current X servers,
xkbcommon has removed support for some parts of the specification which
introduced unnecessary complications.  Many of these removals were in fact
not implemented, or half-implemented at best, as well as being totally
unused in the standard dataset.

Notable removals:
- geometry support
  + there were very few geometry definitions available, and while
    xkbcommon was responsible for parsing this insanely complex format,
    it never actually did anything with it
  + hopefully someone will develop a companion library which supports
    keyboard geometries in a more useful format
- KcCGST (keycodes/compat/geometry/symbols/types) API
  + use RMLVO instead; KcCGST is now an implementation detail
  + including pre-defined keymap files
- XKM support
  + may come in an optional X11 support/compatibility library
- around half of the interpret actions
  + pointer device, message and redirect actions in particular
- non-virtual modifiers
  + core and virtual modifiers have been collapsed into the same
    namespace, with a 'significant' flag that largely parallels the
    core/virtual split
- radio groups
  + completely unused in current keymaps, never fully implemented
- overlays
  + almost completely unused in current keymaps
- key behaviors
  + used to implement radio groups and overlays, and to deal with things
    like keys that physically lock; unused in current keymaps
- indicator behaviours such as LED-controls-key
  + the only supported LED behaviour is key-controls-LED; again this
    was never really used in current keymaps

Notable additions:
- 32-bit keycodes
- extended number of modifiers
- extended number of groups
- multiple keysyms per level
  + this requires incompatible dataset changes, such that X11 would
    not be able to parse these

## Development

An extremely rudimentary homepage can be found at
    http://xkbcommon.org

xkbcommon is maintained in git at
    https://github.com/xkbcommon/libxkbcommon

Patches are always welcome, and may be sent to either
    <xorg-devel@lists.x.org> or <wayland-devel@lists.freedesktop.org>

Bugs are also welcome, and may be reported either at
    Bugzilla https://bugs.freedesktop.org/describecomponents.cgi?product=libxkbcommon
or
    Github https://github.com/xkbcommon/libxkbcommon/issues

The maintainers are
- Daniel Stone <daniel@fooishbar.org>
- Ran Benita <ran234@gmail.com>

## Credits

Many thanks are due to Dan Nicholson for his heroic work in getting xkbcommon
off the ground initially.
