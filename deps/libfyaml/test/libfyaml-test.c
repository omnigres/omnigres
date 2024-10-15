/*
 * libfyaml-test.c - C API testing harness for libyaml
 *
 * Copyright (c) 2019 Pantelis Antoniou <pantelis.antoniou@konsulko.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <getopt.h>
#include <stdbool.h>

#include <check.h>

#include "fy-valgrind.h"

#define QUIET_DEFAULT			false

static struct option lopts[] = {
	{"quiet",		no_argument,		0,	'q' },
	{"help",		no_argument,		0,	'h' },
	{0,			0,              	0,	 0  },
};

static void display_usage(FILE *fp, char *progname)
{
	fprintf(fp, "Usage: %s [options] [files]\n", progname);
	fprintf(fp, "\nOptions:\n\n");
	fprintf(fp, "\t--quiet, -q              : Quiet operation, do not "
						"output messages (default %s)\n",
						QUIET_DEFAULT ? "true" : "false");
	fprintf(fp, "\t--help, -h               : Display  help message\n");
	fprintf(fp, "\ne.g. %s\n", progname);
}

#if defined(HAVE_STATIC) && HAVE_STATIC
extern TCase *libfyaml_case_private(void);
#endif
extern TCase *libfyaml_case_core(void);
extern TCase *libfyaml_case_meta(void);
extern TCase *libfyaml_case_emit(void);

Suite *libfyaml_suite(void)
{
	Suite *s;

	s = suite_create("libfyaml");

#if defined(HAVE_STATIC) && HAVE_STATIC
	suite_add_tcase(s, libfyaml_case_private());
#endif
	suite_add_tcase(s, libfyaml_case_core());
	suite_add_tcase(s, libfyaml_case_meta());
	suite_add_tcase(s, libfyaml_case_emit());

	return s;
}

int main(int argc, char *argv[])
{
	int exitcode = EXIT_FAILURE, opt, lidx;
	bool quiet = QUIET_DEFAULT;
	int number_failed;
	Suite *s;
	SRunner *sr;

	fy_valgrind_check(&argc, &argv);

	while ((opt = getopt_long_only(argc, argv, "qh", lopts, &lidx)) != -1) {
		switch (opt) {
		case 'q':
			quiet = true;
			break;
		case 'h' :
		default:
			if (opt != 'h')
				fprintf(stderr, "Unknown option\n");
			display_usage(opt == 'h' ? stdout : stderr, argv[0]);
			return EXIT_SUCCESS;
		}
	}

	s = libfyaml_suite();
	sr = srunner_create(s);
	srunner_set_tap(sr, "-");
	srunner_run_all(sr, quiet ? CK_SILENT : CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	exitcode = !number_failed ? EXIT_SUCCESS : EXIT_FAILURE;

	return exitcode;
}
