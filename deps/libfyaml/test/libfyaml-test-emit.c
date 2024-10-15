/*
 * libfyaml-test-emit.c - libfyaml test public emitter interface
 *
 * Copyright (c) 2021 Pantelis Antoniou <pantelis.antoniou@konsulko.com>
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

struct test_emitter_data {
	struct fy_emitter *emit;
	struct fy_emitter_cfg cfg;
	size_t alloc;
	size_t count;
	char *buf;
};

static int collect_output(struct fy_emitter *emit, enum fy_emitter_write_type type,
			  const char *str, int len, void *userdata)
{
	struct test_emitter_data *data = userdata;
	char *newbuf;
	size_t alloc, need;

	need = data->count + len + 1;
	alloc = data->alloc;
	if (!alloc)
		alloc = 512;	/* start at 512 bytes and double */
	while (need > alloc)
		alloc <<= 1;

	if (alloc > data->alloc) {
		newbuf = realloc(data->buf, alloc);
		if (!newbuf)
			return -1;
		data->buf = newbuf;
		data->alloc = alloc;
	}
	assert(data->alloc >= need);
	memcpy(data->buf + data->count, str, len);
	data->count += len;
	*(char *)(data->buf + data->count) = '\0';
	data->count++;

	return len;
}

struct fy_emitter *setup_test_emitter(struct test_emitter_data *data)
{
	memset(data, 0, sizeof(*data));
	data->cfg.output = collect_output;
	data->cfg.userdata = data;
	data->cfg.flags = FYECF_DEFAULT;
	data->emit = fy_emitter_create(&data->cfg);
	return data->emit;

}

static void cleanup_test_emitter(struct test_emitter_data *data)
{
	if (data->emit)
		fy_emitter_destroy(data->emit);
	if (data->buf)
		free(data->buf);
}

START_TEST(emit_simple)
{
	struct test_emitter_data data;
	struct fy_emitter *emit;
	int rc;

	emit = setup_test_emitter(&data);
	ck_assert_ptr_ne(emit, NULL);

	rc = fy_emit_event(emit, fy_emit_event_create(emit, FYET_STREAM_START));
	ck_assert_int_eq(rc, 0);

	rc = fy_emit_event(emit, fy_emit_event_create(emit, FYET_DOCUMENT_START, true, NULL, NULL));
	ck_assert_int_eq(rc, 0);

	rc = fy_emit_event(emit, fy_emit_event_create(emit, FYET_SCALAR, FYSS_PLAIN, "simple", FY_NT, NULL, NULL));
	ck_assert_int_eq(rc, 0);

	rc = fy_emit_event(emit, fy_emit_event_create(emit, FYET_DOCUMENT_END, true, NULL, NULL));
	ck_assert_int_eq(rc, 0);

	rc = fy_emit_event(emit, fy_emit_event_create(emit, FYET_STREAM_END));
	ck_assert_int_eq(rc, 0);

	ck_assert_ptr_ne(data.buf, NULL);

	/* the contents must be 'simple' (without a newline) */
	ck_assert_str_eq(data.buf, "simple");

	cleanup_test_emitter(&data);
}
END_TEST

TCase *libfyaml_case_emit(void)
{
	TCase *tc;

	tc = tcase_create("emit");

	tcase_add_test(tc, emit_simple);

	return tc;
}
