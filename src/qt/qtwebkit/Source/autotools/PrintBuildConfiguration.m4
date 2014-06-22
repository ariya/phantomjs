AC_OUTPUT

echo "
WebKit was configured with the following options:

Build configuration:
 Enable debugging (slow)                                  : $enable_debug
 Compile with debug symbols (slow)                        : $enable_debug_symbols
 Enable GCC build optimization                            : $enable_optimizations
 Code coverage support                                    : $enable_coverage
 Optimized memory allocator                               : $enable_fast_malloc
 Accelerated rendering backend                            : $acceleration_description

Features:
=======
 WebKit1 support                                          : $enable_webkit1
 WebKit2 support                                          : $enable_webkit2
 Accelerated Compositing                                  : $enable_accelerated_compositing
 Accelerated 2D canvas                                    : $enable_accelerated_canvas
 Battery API support                                      : $enable_battery_status
 Gamepad support                                          : $enable_gamepad
 Geolocation support                                      : $enable_geolocation
 HTML5 video element support                              : $enable_video
 JIT compilation                                          : $enable_jit
 Opcode stats                                             : $enable_opcode_stats
 SVG fonts support                                        : $enable_svg_fonts
 SVG support                                              : $enable_svg
 Spellcheck support                                       : $enable_spellcheck
 Credential storage support                               : $enable_credential_storage
 Web Audio support                                        : $enable_web_audio
 WebGL                                                    : $enable_webgl


GTK+ configuration:
 GTK+ version                                             : $with_gtk
 GDK target                                               : $with_target
 Introspection support                                    : $enable_introspection
 Generate documentation                                   : $enable_gtk_doc
"
