/*
 * libfyaml-test-private.c - libfyaml private API test harness
 *
 * Copyright (c) 2019 Pantelis Antoniou <pantelis.antoniou@konsulko.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <unistd.h>
#include <limits.h>

#include <check.h>

#include <libfyaml.h>
#include "fy-parse.h"

static const struct fy_parse_cfg default_parse_cfg = {
	.search_path = "",
	.flags = FYPCF_QUIET,
};

START_TEST(parser_setup)
{
	struct fy_parser ctx, *fyp = &ctx;
	const struct fy_parse_cfg *cfg = &default_parse_cfg;
	int rc;

	/* setup */
	rc = fy_parse_setup(fyp, cfg);
	ck_assert_int_eq(rc, 0);

	/* cleanup */
	fy_parse_cleanup(fyp);
}
END_TEST

START_TEST(scan_simple)
{
	struct fy_parser ctx, *fyp = &ctx;
	const struct fy_parse_cfg *cfg = &default_parse_cfg;
	static const struct fy_input_cfg fyic = {
		.type		= fyit_memory,
		.memory.data	= "42",
		.memory.size	= 2,
	};
	struct fy_token *fyt;
	int rc;

	/* setup */
	rc = fy_parse_setup(fyp, cfg);
	ck_assert_int_eq(rc, 0);

	/* add the input */
	rc = fy_parse_input_append(fyp, &fyic);
	ck_assert_int_eq(rc, 0);

	/* STREAM_START */
	fyt = fy_scan(fyp);
	ck_assert_ptr_ne(fyt, NULL);
	ck_assert(fyt->type == FYTT_STREAM_START);
	fy_token_unref(fyt);

	/* SCALAR */
	fyt = fy_scan(fyp);
	ck_assert_ptr_ne(fyt, NULL);
	ck_assert(fyt->type == FYTT_SCALAR);
	ck_assert(fyt->scalar.style == FYSS_PLAIN);
	ck_assert_str_eq(fy_token_get_text0(fyt), "42");
	fy_token_unref(fyt);

	/* STREAM_END */
	fyt = fy_scan(fyp);
	ck_assert_ptr_ne(fyt, NULL);
	ck_assert(fyt->type == FYTT_STREAM_END);
	fy_token_unref(fyt);

	/* EOF */
	fyt = fy_scan(fyp);
	ck_assert_ptr_eq(fyt, NULL);

	/* cleanup */
	fy_parse_cleanup(fyp);
}
END_TEST

START_TEST(parse_simple)
{
	struct fy_parser ctx, *fyp = &ctx;
	const struct fy_parse_cfg *cfg = &default_parse_cfg;
	static const struct fy_input_cfg fyic = {
		.type		= fyit_memory,
		.memory.data	= "42",
		.memory.size	= 2,
	};
	struct fy_eventp *fyep;
	int rc;

	/* setup */
	rc = fy_parse_setup(fyp, cfg);
	ck_assert_int_eq(rc, 0);

	/* add the input */
	rc = fy_parse_input_append(fyp, &fyic);
	ck_assert_int_eq(rc, 0);

	/* STREAM_START */
	fyep = fy_parse_private(fyp);
	ck_assert_ptr_ne(fyep, NULL);
	ck_assert(fyep->e.type == FYET_STREAM_START);
	fy_parse_eventp_recycle(fyp, fyep);

	/* DOCUMENT_START */
	fyep = fy_parse_private(fyp);
	ck_assert_ptr_ne(fyep, NULL);
	ck_assert(fyep->e.type == FYET_DOCUMENT_START);
	fy_parse_eventp_recycle(fyp, fyep);

	/* SCALAR */
	fyep = fy_parse_private(fyp);
	ck_assert_ptr_ne(fyep, NULL);
	ck_assert(fyep->e.type == FYET_SCALAR);
	ck_assert_str_eq(fy_token_get_text0(fyep->e.scalar.value), "42");
	fy_parse_eventp_recycle(fyp, fyep);

	/* DOCUMENT_END */
	fyep = fy_parse_private(fyp);
	ck_assert_ptr_ne(fyep, NULL);
	ck_assert(fyep->e.type == FYET_DOCUMENT_END);
	fy_parse_eventp_recycle(fyp, fyep);

	/* STREAM_END */
	fyep = fy_parse_private(fyp);
	ck_assert_ptr_ne(fyep, NULL);
	ck_assert(fyep->e.type == FYET_STREAM_END);
	fy_parse_eventp_recycle(fyp, fyep);

	/* EOF */
	fyep = fy_parse_private(fyp);
	ck_assert_ptr_eq(fyep, NULL);

	/* cleanup */
	fy_parse_cleanup(fyp);
}
END_TEST

TCase *libfyaml_case_private(void)
{
	TCase *tc;

	tc = tcase_create("private");

	tcase_add_test(tc, parser_setup);
	tcase_add_test(tc, scan_simple);
	tcase_add_test(tc, parse_simple);

	return tc;
}
