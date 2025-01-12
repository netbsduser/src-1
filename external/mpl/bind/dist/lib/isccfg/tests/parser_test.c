/*	$NetBSD: parser_test.c,v 1.8 2021/04/05 11:27:04 rillig Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#if HAVE_CMOCKA

#include <sched.h> /* IWYU pragma: keep */
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/buffer.h>
#include <isc/lex.h>
#include <isc/log.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/string.h>
#include <isc/types.h>
#include <isc/util.h>

#include <isccfg/cfg.h>
#include <isccfg/grammar.h>
#include <isccfg/namedconf.h>

#define CHECK(r)                             \
	do {                                 \
		result = (r);                \
		if (result != ISC_R_SUCCESS) \
			goto cleanup;        \
	} while (0)

isc_mem_t *mctx = NULL;
isc_log_t *lctx = NULL;
static isc_logcategory_t categories[] = { { "", 0 },
					  { "client", 0 },
					  { "network", 0 },
					  { "update", 0 },
					  { "queries", 0 },
					  { "unmatched", 0 },
					  { "update-security", 0 },
					  { "query-errors", 0 },
					  { NULL, 0 } };

static void
cleanup() {
	if (lctx != NULL) {
		isc_log_setcontext(NULL);
		isc_log_destroy(&lctx);
	}
	if (mctx != NULL) {
		isc_mem_destroy(&mctx);
	}
}

static isc_result_t
setup() {
	isc_result_t result;

	isc_mem_debugging |= ISC_MEM_DEBUGRECORD;
	isc_mem_create(&mctx);

	isc_logdestination_t destination;
	isc_logconfig_t *logconfig = NULL;

	isc_log_create(mctx, &lctx, &logconfig);
	isc_log_registercategories(lctx, categories);
	isc_log_setcontext(lctx);

	destination.file.stream = stderr;
	destination.file.name = NULL;
	destination.file.versions = ISC_LOG_ROLLNEVER;
	destination.file.maximum_size = 0;
	isc_log_createchannel(logconfig, "stderr", ISC_LOG_TOFILEDESC,
			      ISC_LOG_DYNAMIC, &destination, 0);
	CHECK(isc_log_usechannel(logconfig, "stderr", NULL, NULL));

	return (ISC_R_SUCCESS);

cleanup:
	cleanup();
	return (result);
}

static int
_setup(void **state) {
	isc_result_t result;

	UNUSED(state);

	result = setup();
	assert_int_equal(result, ISC_R_SUCCESS);

	return (0);
}

static int
_teardown(void **state) {
	UNUSED(state);

	cleanup();

	return (0);
}

/* mimic calling nzf_append() */
static void
append(void *arg, const char *str, int len) {
	char *buf = arg;
	size_t l = strlen(buf);
	snprintf(buf + l, 1024 - l, "%.*s", len, str);
}

static void
addzoneconf(void **state) {
	isc_result_t result;
	isc_buffer_t b;
	cfg_parser_t *p = NULL;
	const char *tests[] = {
		"zone \"test4.baz\" { type master; file \"e.db\"; };",
		"zone \"test/.baz\" { type master; file \"e.db\"; };",
		"zone \"test\\\".baz\" { type master; file \"e.db\"; };",
		"zone \"test\\.baz\" { type master; file \"e.db\"; };",
		"zone \"test\\\\.baz\" { type master; file \"e.db\"; };",
		"zone \"test\\032.baz\" { type master; file \"e.db\"; };",
		"zone \"test\\010.baz\" { type master; file \"e.db\"; };"
	};
	char buf[1024];

	UNUSED(state);

	/* Parse with default line numbering */
	result = cfg_parser_create(mctx, lctx, &p);
	assert_int_equal(result, ISC_R_SUCCESS);

#define ARRAYSIZE(x) (sizeof(x) / sizeof(x[0]))

	for (size_t i = 0; i < ARRAYSIZE(tests); i++) {
		cfg_obj_t *conf = NULL;
		const cfg_obj_t *obj = NULL, *zlist = NULL;

		isc_buffer_constinit(&b, tests[i], strlen(tests[i]));
		isc_buffer_add(&b, strlen(tests[i]));

		result = cfg_parse_buffer(p, &b, "text1", 0,
					  &cfg_type_namedconf, 0, &conf);
		assert_int_equal(result, ISC_R_SUCCESS);

		/*
		 * Mimic calling nzf_append() from bin/named/server.c
		 * and check that the output matches the input.
		 */
		result = cfg_map_get(conf, "zone", &zlist);
		assert_int_equal(result, ISC_R_SUCCESS);

		obj = cfg_listelt_value(cfg_list_first(zlist));
		assert_ptr_not_equal(obj, NULL);

		strlcpy(buf, "zone ", sizeof(buf));
		cfg_printx(obj, CFG_PRINTER_ONELINE, append, buf);
		strlcat(buf, ";", sizeof(buf));
		assert_string_equal(tests[i], buf);

		cfg_obj_destroy(p, &conf);
		cfg_parser_reset(p);
	}

	cfg_parser_destroy(&p);
}

/* test cfg_parse_buffer() */
static void
parse_buffer_test(void **state) {
	isc_result_t result;
	unsigned char text[] = "options\n{\nrecursion yes;\n};\n";
	isc_buffer_t buf1, buf2;
	cfg_parser_t *p1 = NULL, *p2 = NULL;
	cfg_obj_t *c1 = NULL, *c2 = NULL;

	UNUSED(state);

	isc_buffer_init(&buf1, &text[0], sizeof(text) - 1);
	isc_buffer_add(&buf1, sizeof(text) - 1);

	/* Parse with default line numbering */
	result = cfg_parser_create(mctx, lctx, &p1);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = cfg_parse_buffer(p1, &buf1, "text1", 0, &cfg_type_namedconf, 0,
				  &c1);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(p1->line, 5);

	isc_buffer_init(&buf2, &text[0], sizeof(text) - 1);
	isc_buffer_add(&buf2, sizeof(text) - 1);

	/* Parse with changed line number */
	result = cfg_parser_create(mctx, lctx, &p2);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = cfg_parse_buffer(p2, &buf2, "text2", 100, &cfg_type_namedconf,
				  0, &c2);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(p2->line, 104);

	cfg_obj_destroy(p1, &c1);
	cfg_obj_destroy(p2, &c2);

	cfg_parser_destroy(&p1);
	cfg_parser_destroy(&p2);
}

/* test cfg_map_firstclause() */
static void
cfg_map_firstclause_test(void **state) {
	const char *name = NULL;
	const void *clauses = NULL;
	unsigned int idx;

	UNUSED(state);

	name = cfg_map_firstclause(&cfg_type_zoneopts, &clauses, &idx);
	assert_non_null(name);
	assert_non_null(clauses);
	assert_int_equal(idx, 0);
}

/* test cfg_map_nextclause() */
static void
cfg_map_nextclause_test(void **state) {
	const char *name = NULL;
	const void *clauses = NULL;
	unsigned int idx;

	UNUSED(state);

	name = cfg_map_firstclause(&cfg_type_zoneopts, &clauses, &idx);
	assert_non_null(name);
	assert_non_null(clauses);
	assert_int_equal(idx, ISC_R_SUCCESS);

	do {
		name = cfg_map_nextclause(&cfg_type_zoneopts, &clauses, &idx);
		if (name != NULL) {
			assert_non_null(clauses);
		} else {
			assert_null(clauses);
			assert_int_equal(idx, 0);
		}
	} while (name != NULL);
}

int
main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(addzoneconf),
		cmocka_unit_test(parse_buffer_test),
		cmocka_unit_test(cfg_map_firstclause_test),
		cmocka_unit_test(cfg_map_nextclause_test),
	};

	return (cmocka_run_group_tests(tests, _setup, _teardown));
}

#else /* HAVE_CMOCKA */

#include <stdio.h>

int
main(void) {
	printf("1..0 # Skipped: cmocka not available\n");
	return (0);
}

#endif /* if HAVE_CMOCKA */
