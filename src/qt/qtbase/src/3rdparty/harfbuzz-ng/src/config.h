#ifndef HB_CONFIG_H
#define HB_CONFIG_H

#define HAVE_OT
#define HAVE_ATEXIT

#define HB_NO_UNICODE_FUNCS

#define HB_DISABLE_DEPRECATED

// because strdup() is not part of strict Posix, declare it here
extern "C" char *strdup(const char *src);

#endif /* HB_CONFIG_H */
