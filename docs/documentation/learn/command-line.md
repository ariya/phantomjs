---
layout: post
title: Command Line Interface
categories: docs docs-learn
permalink: api/command-line.html
---

_**As the codebase is updated, we hope to keep these documents updated as well. Unless otherwise stated, this documentation currently applies to the latest PhantomJS release:** PhantomJS 2.1.1_

Assuming PhantomJS is [built](http://phantomjs.org/build.html) and its executable is place somewhere in the PATH, it can be invoked as follows:

```bash
phantomjs [options] somescript.js [arg1 [arg2 [...]]]
```

The script code will be executed as if it running in a web browser with an empty page. Since PhantomJS is headless, there will not be anything visible shown up on the screen.

If PhantomJS is invoked without any argument, it will enter the interactive mode (REPL).

## Command-line Options

Supported command-line options are:

 * `--help` or `-h` lists all possible command-line options. _Halts immediately, will not run a script passed as argument._
 * `--version` or `-v` prints out the version of PhantomJS. _Halts immediately, will not run a script passed as argument._
 * `--debug=[true|false]` prints additional warning and debug message, default is `false`. Also accepted: `[yes|no]`.
 * `--config` specifies JSON-formatted configuration file (see below).
 * `--cookies-file=/path/to/cookies.txt` specifies the file name to store the persistent Cookies.
 * `--disk-cache=[true|false]` enables disk cache (at desktop services cache storage location, default is `false`). Also accepted: `[yes|no]`.
 * `--disk-cache-path` specifies the location for the disk cache.
 * `--ignore-ssl-errors=[true|false]` ignores SSL errors, such as expired or self-signed certificate errors (default is `false`). Also accepted: `[yes|no]`.
 * `--load-images=[true|false]` load all inlined images (default is `true`). Also accepted: `[yes|no]`.
 * `--local-storage-path=/some/path` path to save LocalStorage content and WebSQL content.
 * `--local-storage-quota=number` maximum size to allow for data.
 * `--local-url-access` allows use of 'file:///' URLs (default is 'true').
 * `--local-to-remote-url-access=[true|false]` allows local content to access remote URL (default is `false`). Also accepted: `[yes|no]`.
 * `--max-disk-cache-size=size` limits the size of disk cache (in KB).
 * `--offline-storage-path` specifies the location for offline storage.
 * `--offline-storage-quota` sets the maximum size of the offline storage (in KB).
 * `--output-encoding=encoding` sets the encoding used for terminal output (default is `utf8`).
 *  `--remote-debugger-port` starts the script in a debug harness and listens on the specified port
 *  `--remote-debugger-autorun` runs the script in the debugger immediately: 'yes' or 'no' (default)
 * `--proxy=address:port` specifies the proxy server to use (e.g. `--proxy=192.168.1.42:8080`).
 * `--proxy-type=[http|socks5|none]` specifies the type of the proxy server (default is `http`).
 * `--proxy-auth` specifies the authentication information for the proxy, e.g. `--proxy-auth=username:password`).
 * `--script-encoding=encoding` sets the encoding used for the starting script (default is `utf8`).
 * `--script-language` sets the script language instead of detecting it: 'javascript'.
 * `--ssl-protocol=[sslv3|sslv2|tlsv1|tlsv1.1|tlsv1.2|any']` sets the SSL protocol for secure connections (default is `SSLv3`). Not all values may be supported, depending on the system OpenSSL library.
 * `--ssl-certificates-path=<val>` sets the location for custom CA certificates (if none set, uses system default).
 * `--ssl-client-certificate-file` sets the location of a client certificate.
 * `--ssl-client-key-file` sets the location of a clients' private key.
 * `--ssl-client-key-passphrase` sets the passphrase for the clients' private key.
 * `--ssl-ciphers` sets supported TLS/SSL ciphers. Argument is a colon-separated list of OpenSSL cipher names (macros like ALL, kRSA, etc. may not be used). Default matches modern browsers.
 * `--web-security=[true|false]` enables web security and forbids cross-domain XHR (default is `true`). Also accepted: `[yes|no]`.
 * `--webdriver` starts in 'Remote WebDriver mode' (embedded GhostDriver): '[[<IP>:]<PORT>]' (default '127.0.0.1:8910')
 * `--webdriver-selenium-grid-hub` URL to the Selenium Grid HUB: 'URL_TO_HUB' (default 'none') (NOTE: works only together with '--webdriver')
 * `--webdriver-logfile` file where to write the WebDriver's Log (default 'none') (NOTE: needs '--webdriver')
 * `--webdriver-loglevel` WebDriver Logging Level: (supported: 'ERROR', 'WARN', 'INFO', 'DEBUG') (default 'INFO') (NOTE: needs '--webdriver')

Alternatively, since PhantomJS 1.3, you can also utilize a JavaScript Object Notation (JSON) configuration file instead of passing in multiple command-line options:

```bash
--config=/path/to/config.json
```

The contents of `config.json` should be a standalone JavaScript object. Keys are de-dashed, camel-cased equivalents of the other supported command-line options (excluding `--version`/`-v` and `--help`/`-h`).  Values are their JavaScript equivalents: 'true'/'false' (or 'yes'/'no') values translate into `true`/`false` Boolean values, numbers remain numbers, strings remain strings. For example:

```js
{
  /* Same as: --ignore-ssl-errors=true */
  "ignoreSslErrors": true,

  /* Same as: --max-disk-cache-size=1000 */
  "maxDiskCacheSize": 1000,

  /* Same as: --output-encoding=utf8 */
  "outputEncoding": "utf8"

  /* etc. */
}

There are some keys that do not translate directly:

 * --disk-cache => diskCacheEnabled
 * --load-images => autoLoadImages
 * --local-storage-path => offlineStoragePath
 * --local-storage-quota => offlineStorageDefaultQuota
 * --local-to-remote-url-access => localToRemoteUrlAccessEnabled
 * --web-security => webSecurityEnabled
 * --debug => printDebugMessages

```
