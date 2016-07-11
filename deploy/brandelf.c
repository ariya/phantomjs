/*-
 * Copyright (c) 2000, 2001 David O'Brien
 * Copyright (c) 1996 Søren Schmidt
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in this position and unchanged.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
//NOTE: commented out to make it compile on linux
// __FBSDID("$FreeBSD: src/usr.bin/brandelf/brandelf.c,v 1.25.22.2 2012/03/16 03:22:37 eadler Exp $");

#include <sys/types.h>
//NOTE: changed path to make it compile on linux
#include <elf.h>
#include <sys/errno.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int elftype(const char *);
static const char *iselftype(int);
static void printelftypes(void);
static void usage(void);

struct ELFtypes {
	const char *str;
	int value;
};
/* XXX - any more types? */
static struct ELFtypes elftypes[] = {
	{ "FreeBSD",	ELFOSABI_FREEBSD },
	{ "Linux",	ELFOSABI_LINUX },
	{ "Solaris",	ELFOSABI_SOLARIS },
	{ "SVR4",	ELFOSABI_SYSV }
};

int
main(int argc, char **argv)
{

	const char *strtype = "FreeBSD";
	int type = ELFOSABI_FREEBSD;
	int retval = 0;
	int ch, change = 0, force = 0, listed = 0;

	while ((ch = getopt(argc, argv, "f:lt:v")) != -1)
		switch (ch) {
		case 'f':
			if (change)
				errx(1, "f option incompatible with t option");
			force = 1;
			type = atoi(optarg);
			if (errno == ERANGE || type < 0 || type > 255) {
				warnx("invalid argument to option f: %s",
				    optarg);
				usage();
			}
			break;
		case 'l':
			printelftypes();
			listed = 1;
			break;
		case 'v':
			/* does nothing */
			break;
		case 't':
			if (force)
				errx(1, "t option incompatible with f option");
			change = 1;
			strtype = optarg;
			break;
		default:
			usage();
	}
	argc -= optind;
	argv += optind;
	if (!argc) {
		if (listed)
			exit(0);
		else {
			warnx("no file(s) specified");
			usage();
		}
	}

	if (!force && (type = elftype(strtype)) == -1) {
		warnx("invalid ELF type '%s'", strtype);
		printelftypes();
		usage();
	}

	while (argc) {
		int fd;
		char buffer[EI_NIDENT];

		if ((fd = open(argv[0], change || force ? O_RDWR : O_RDONLY, 0)) < 0) {
			warn("error opening file %s", argv[0]);
			retval = 1;
			goto fail;
		}
		if (read(fd, buffer, EI_NIDENT) < EI_NIDENT) {
			warnx("file '%s' too short", argv[0]);
			retval = 1;
			goto fail;
		}
		if (buffer[0] != ELFMAG0 || buffer[1] != ELFMAG1 ||
		    buffer[2] != ELFMAG2 || buffer[3] != ELFMAG3) {
			warnx("file '%s' is not ELF format", argv[0]);
			retval = 1;
			goto fail;
		}
		if (!change && !force) {
			fprintf(stdout,
				"File '%s' is of brand '%s' (%u).\n",
				argv[0], iselftype(buffer[EI_OSABI]),
				buffer[EI_OSABI]);
			if (!iselftype(type)) {
				warnx("ELF ABI Brand '%u' is unknown",
				      type);
				printelftypes();
			}
		}
		else {
			buffer[EI_OSABI] = type;
			lseek(fd, 0, SEEK_SET);
			if (write(fd, buffer, EI_NIDENT) != EI_NIDENT) {
				warn("error writing %s %d", argv[0], fd);
				retval = 1;
				goto fail;
			}
		}
fail:
		close(fd);
		argc--;
		argv++;
	}

	return retval;
}

static void
usage(void)
{
	(void)fprintf(stderr,
	    "usage: brandelf [-lv] [-f ELF_ABI_number] [-t string] file ...\n");
	exit(1);
}

static const char *
iselftype(int etype)
{
	size_t elfwalk;

	for (elfwalk = 0;
	     elfwalk < sizeof(elftypes)/sizeof(elftypes[0]);
	     elfwalk++)
		if (etype == elftypes[elfwalk].value)
			return elftypes[elfwalk].str;
	return 0;
}

static int
elftype(const char *elfstrtype)
{
	size_t elfwalk;

	for (elfwalk = 0;
	     elfwalk < sizeof(elftypes)/sizeof(elftypes[0]);
	     elfwalk++)
		if (strcasecmp(elfstrtype, elftypes[elfwalk].str) == 0)
			return elftypes[elfwalk].value;
	return -1;
}

static void
printelftypes(void)
{
	size_t elfwalk;

	fprintf(stderr, "known ELF types are: ");
	for (elfwalk = 0;
	     elfwalk < sizeof(elftypes)/sizeof(elftypes[0]);
	     elfwalk++)
		fprintf(stderr, "%s(%u) ", elftypes[elfwalk].str,
			elftypes[elfwalk].value);
	fprintf(stderr, "\n");
}
