/*
 * libfyaml-test-meta.c - libfyaml meta testing harness
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

START_TEST(meta_basic)
{
	struct fy_document *fyd;
	struct fy_node *fyn_root;
	void *meta;
	int meta_value;
	int ret;

	/* build document */
	fyd = fy_document_build_from_string(NULL, "100", FY_NT);
	ck_assert_ptr_ne(fyd, NULL);

	/* check that root exist */
	fyn_root = fy_document_root(fyd);
	ck_assert_ptr_ne(fyn_root, NULL);

	/* compare with expected result */
	ck_assert_str_eq(fy_node_get_scalar0(fyn_root), "100");

	/* check that no meta at start */
	meta = fy_node_get_meta(fyn_root);
	ck_assert_ptr_eq(meta, NULL);

	/* set a simple value meta */
	ret = fy_node_set_meta(fyn_root, (void *)(uintptr_t)100);
	ck_assert_int_eq(ret, 0);

	/* retrieve it and verify */
	meta = fy_node_get_meta(fyn_root);
	meta_value = (int)(uintptr_t)meta;
	ck_assert_int_eq(meta_value, 100);

	/* clear the meta */
	fy_node_clear_meta(fyn_root);

	/* check that no meta exists now */
	meta = fy_node_get_meta(fyn_root);
	ck_assert_ptr_eq(meta, NULL);

	/* setting new meta now should work */
	ret = fy_node_set_meta(fyn_root, (void *)(uintptr_t)200);
	ck_assert_int_eq(ret, 0);

	/* retrieve it and verify */
	meta = fy_node_get_meta(fyn_root);
	meta_value = (int)(uintptr_t)meta;
	ck_assert_int_eq(meta_value, 200);

	/* setting new meta override */
	ret = fy_node_set_meta(fyn_root, (void *)(uintptr_t)201);
	ck_assert_int_eq(ret, 0);

	/* retrieve it and verify */
	meta = fy_node_get_meta(fyn_root);
	meta_value = (int)(uintptr_t)meta;
	ck_assert_int_eq(meta_value, 201);

	fy_document_destroy(fyd);
}
END_TEST

static void test_meta_clear_cb(struct fy_node *fyn, void *meta, void *user)
{
	int meta_value = (int)(uintptr_t)meta;
	int *clear_counter_p = user;

	*clear_counter_p += meta_value;
}

START_TEST(meta_clear_cb)
{
	struct fy_document *fyd;
	struct fy_node *fyn_root, *fyn_0, *fyn_1;
	int ret;
	int clear_counter;

	/* build document */
	fyd = fy_document_build_from_string(NULL, "[ 100, 101 ]", FY_NT);
	ck_assert_ptr_ne(fyd, NULL);

	/* check that root exist */
	fyn_root = fy_document_root(fyd);
	ck_assert_ptr_ne(fyn_root, NULL);

	/* register the meta clear callback */
	clear_counter = 0;
	ret = fy_document_register_meta(fyd, test_meta_clear_cb, &clear_counter);

	/* setting new meta */
	ret = fy_node_set_meta(fyn_root, (void *)(uintptr_t)1000);
	ck_assert_int_eq(ret, 0);

	/* the clear counter must be unchanged */
	ck_assert_int_eq(clear_counter, 0);

	/* get the two items of the sequence and assign meta */
	fyn_0 = fy_node_by_path(fyn_root, "/0", (size_t)-1, FYNWF_DONT_FOLLOW);
	ck_assert_ptr_ne(fyn_0, NULL);
	ret = fy_node_set_meta(fyn_root, (void *)(uintptr_t)100);
	ck_assert_int_eq(ret, 0);

	fyn_1 = fy_node_by_path(fyn_root, "/1", (size_t)-1, FYNWF_DONT_FOLLOW);
	ck_assert_ptr_ne(fyn_1, NULL);
	ret = fy_node_set_meta(fyn_root, (void *)(uintptr_t)200);
	ck_assert_int_eq(ret, 0);

	/* destroy the document */
	fy_document_destroy(fyd);

	/* the end result of the counter must be the sum of all */
	ck_assert_int_eq(clear_counter, 1000 + 100 + 200);
}
END_TEST

START_TEST(meta_unregister)
{
	struct fy_document *fyd;
	struct fy_node *fyn_root, *fyn_0, *fyn_1;
	int ret;
	int clear_counter;

	/* build document */
	fyd = fy_document_build_from_string(NULL, "[ 100, 101 ]", FY_NT);
	ck_assert_ptr_ne(fyd, NULL);

	/* check that root exist */
	fyn_root = fy_document_root(fyd);
	ck_assert_ptr_ne(fyn_root, NULL);

	/* register the meta clear callback */
	clear_counter = 0;
	ret = fy_document_register_meta(fyd, test_meta_clear_cb, &clear_counter);

	/* setting new meta */
	ret = fy_node_set_meta(fyn_root, (void *)(uintptr_t)1000);
	ck_assert_int_eq(ret, 0);

	/* the clear counter must be unchanged */
	ck_assert_int_eq(clear_counter, 0);

	/* get the two items of the sequence and assign meta */
	fyn_0 = fy_node_by_path(fyn_root, "/0", (size_t)-1, FYNWF_DONT_FOLLOW);
	ck_assert_ptr_ne(fyn_0, NULL);
	ret = fy_node_set_meta(fyn_root, (void *)(uintptr_t)100);
	ck_assert_int_eq(ret, 0);

	fyn_1 = fy_node_by_path(fyn_root, "/1", (size_t)-1, FYNWF_DONT_FOLLOW);
	ck_assert_ptr_ne(fyn_1, NULL);
	ret = fy_node_set_meta(fyn_root, (void *)(uintptr_t)200);
	ck_assert_int_eq(ret, 0);

	/* unregister */
	fy_document_unregister_meta(fyd);

	/* the counter must be the sum of all after unregistering */
	ck_assert_int_eq(clear_counter, 1000 + 100 + 200);

	/* destroy the document */
	fy_document_destroy(fyd);

}
END_TEST

TCase *libfyaml_case_meta(void)
{
	TCase *tc;

	tc = tcase_create("meta");

	tcase_add_test(tc, meta_basic);
	tcase_add_test(tc, meta_clear_cb);
	tcase_add_test(tc, meta_unregister);

	return tc;
}
