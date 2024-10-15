/*
 * fy-valgrind.h - valgrind auto option handling
 *
 * Copyright (c) 2019 Pantelis Antoniou <pantelis.antoniou@konsulko.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef FY_VALGRIND_H
#define FY_VALGRIND_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <unistd.h>
#include <limits.h>
#include <stdio.h>

enum fy_valgrind_tool {
	fyvt_none,
	fyvt_valgrind,
	fyvt_callgrind,
	fyvt_massif,
};

static inline void fy_valgrind_check(int *argcp, char ***argvp)
{
	int argc = *argcp, va_argc;
	char **argv = *argvp, **va_argv;
	int i;
#ifdef __linux__
	char exe[PATH_MAX];
	ssize_t ret;
#endif
	char *argv0 = NULL;
	char *valgrind;
	enum fy_valgrind_tool tool = fyvt_none;

	/* check for environment variables at first */
	if (getenv("USE_VALGRIND")) {
		tool = fyvt_valgrind;
		goto do_valgrind_no_opt;
	}
	if (getenv("USE_CALLGRIND") || getenv("USE_CACHEGRIND")) {
		tool = fyvt_callgrind;
		goto do_valgrind_no_opt;
	}
	if (getenv("USE_MASSIF")) {
		tool = fyvt_callgrind;
		goto do_valgrind_no_opt;
	}
	for (i = 1; i < argc; i++) {
		/* -- is end of options */
		if (!strcmp(argv[i], "--"))
			break;

		if (!strcmp(argv[i], "--valgrind")) {
			tool = fyvt_valgrind;
			break;
		}
		if (!strcmp(argv[i], "--callgrind") || !strcmp(argv[i], "--cachegrind")) {
			tool = fyvt_callgrind;
			break;
		}
		if (!strcmp(argv[i], "--massif")) {
			tool = fyvt_massif;
			break;
		}
	}

	if (tool == fyvt_none)
		return;

	/* remove --valgrind/--callgrind/--massif from the option list */
	memmove(argv + i, argv + i + 1, (argc - i) * sizeof(*argv));
	(*argcp)--;
	argc--;

do_valgrind_no_opt:
	/* clear those environment variables in any case */
	unsetenv("USE_VALGRIND");
	unsetenv("USE_CALLGRIND");
	unsetenv("USE_MASSIF");

#ifdef __linux__
	/* it's a Linuxism! but it should fail gracefully */
	ret = readlink("/proc/self/exe", exe, sizeof(exe));
	if (ret == 0)
		argv0 = exe;
#endif
	if (!argv0)
		argv0 = argv[0];

	valgrind = getenv("VALGRIND");
	if (valgrind)
		unsetenv("VALGRIND");
	else
		valgrind = "valgrind";

	switch (tool) {

	case fyvt_valgrind:
		va_argc = 1 + 4 + argc - 1;
		va_argv = alloca(sizeof(*va_argv) * (va_argc + 1));
		va_argv[0] = valgrind;
		va_argv[1] = "--leak-check=full";
		va_argv[2] = "--track-origins=yes";
		va_argv[3] = "--error-exitcode=5";
		va_argv[4] = argv0;
		memcpy(va_argv + 1 + 4, argv + 1, argc * sizeof(*va_argv));
		break;

	case fyvt_callgrind:
		va_argc = 1 + 5 + argc - 1;
		va_argv = alloca(sizeof(*va_argv) * (va_argc + 1));
		va_argv[0] = valgrind;
		va_argv[1] = "--tool=callgrind";
		va_argv[2] = "--dump-instr=yes";
		va_argv[3] = "--simulate-cache=yes";
		va_argv[4] = "--collect-jumps=yes";
		va_argv[5] = argv0;
		memcpy(va_argv + 1 + 5, argv + 1, argc * sizeof(*va_argv));
		break;

	case fyvt_massif:
		va_argc = 1 + 2 + argc - 1;
		va_argv = alloca(sizeof(*va_argv) * (va_argc + 1));
		va_argv[0] = valgrind;
		va_argv[1] = "--tool=massif";
		va_argv[2] = argv0;
		memcpy(va_argv + 1 + 2, argv + 1, argc * sizeof(*va_argv));
		break;

	default:
		abort();	/* should never get here */
		break;
	}

	assert(va_argv[va_argc] == NULL);

	for (i = 0; i < va_argc; i++)
		fprintf(stderr, "[%d]: %s\n", i, va_argv[i]);
	fprintf(stderr, "[%d]: %s\n", i, va_argv[i]);

	setenv("FY_VALGRIND", "1", 1);

	execvp("valgrind", va_argv);

	fprintf(stderr, "warning: failed to start valgrind, continue without");
}

#endif
