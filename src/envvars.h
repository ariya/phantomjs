#ifndef ENVVARS_H
#define ENVVARS_H

#define ENVVAR(x)                           (getenv("PHANTOMJS_" x))

#define ENVVAR_OUTPUT_ENCODING              ENVVAR("OUTPUT_ENCODING")
#define ENVVAR_SCRIPT_ENCODING              ENVVAR("SCRIPT_ENCODING")

#endif // ENVVARS_H
