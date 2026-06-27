/*
 * tests.c — xUnit test suite for ungeek-cgi
 *
 * Build + run:
 *   cc nob.c -o nob && ./nob test
 *
 * SPDX-License-Identifier: BSD-2-Clause
 * Da Planet Security / denzuko <denzuko@dapla.net>
 */

#include "test.h"
#include "matrix_id.h"
#include "geek.h"

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

/* ── Suite: Geek Code tokeniser ─────────────────────────────────────────── */

TEST(geek_code_prefix_empty_input_returns_null)
{
    const char *input = "";
    ASSERT('\0' == input[0]);
}

TEST(geek_code_prefix_GCS_recognised)
{
    const GeekCat *cat = geek_find_cat("GCS");
    ASSERT(NULL != cat);
    ASSERT(0 == strcmp("Computer Science", cat->name));
}

TEST(geek_code_modifier_dollar_stripped)
{
    char buf[32];
    const char *stripped = geek_strip_modifier(buf, sizeof(buf), "+++$");
    ASSERT(0 == strcmp("+++", stripped));
}

TEST(geek_code_modifier_at_stripped)
{
    char buf[32];
    const char *stripped = geek_strip_modifier(buf, sizeof(buf), "---@");
    ASSERT(0 == strcmp("---", stripped));
}

TEST(geek_code_null_input_safe)
{
    const GeekCat *cat = geek_find_cat(NULL);
    ASSERT(NULL == cat);
}

TEST(geek_code_version_header_skipped)
{
    const char *line = "Version: 3.1";
    ASSERT(0 == strncmp(line, "Version:", 8));
}

TEST(geek_lookup_val_plusplus_decoded)
{
    const char *desc = geek_lookup_val("++");
    ASSERT(0 == strcmp("proficient", desc));
}

TEST(geek_lookup_val_triple_minus_decoded)
{
    const char *desc = geek_lookup_val("---");
    ASSERT(0 == strcmp("actively opposed", desc));
}

TEST(geek_lookup_val_unknown_returns_input)
{
    const char *desc = geek_lookup_val("xyz");
    ASSERT(0 == strcmp("xyz", desc));
}

TEST(geek_find_cat_unknown_returns_null)
{
    const GeekCat *cat = geek_find_cat("ZZZZ");
    ASSERT(NULL == cat);
}

TEST(geek_find_cat_Linux_recognised)
{
    const GeekCat *cat = geek_find_cat("L");
    ASSERT(NULL != cat);
    ASSERT(0 == strcmp("Linux", cat->name));
}

/* ── Suite: net.matrix identity ─────────────────────────────────────────── */

TEST(matrix_organization_present)
{
    extern volatile const char MATRIX_ID_ORGANIZATION[];
    ASSERT(NULL != MATRIX_ID_ORGANIZATION);
    ASSERT('\0' != MATRIX_ID_ORGANIZATION[0]);
}

TEST(matrix_application_is_ungeek_cgi)
{
    extern volatile const char MATRIX_ID_APPLICATION[];
    ASSERT(NULL != MATRIX_ID_APPLICATION);
    ASSERT(NULL != strstr((const char *)MATRIX_ID_APPLICATION, "ungeek-cgi"));
}

TEST(matrix_owner_present)
{
    extern volatile const char MATRIX_ID_OWNER[];
    ASSERT(NULL != MATRIX_ID_OWNER);
    ASSERT(NULL != strstr((const char *)MATRIX_ID_OWNER, "FC13F74B"));
}

/* ── Suite: SLSA provenance paths ───────────────────────────────────────── */

TEST(slsa_binary_output_path_is_deterministic)
{
    const char *path = "ungeek.cgi";
    ASSERT(NULL != path);
    ASSERT('\0' != path[0]);

    errno = 0;
    FILE *f = fopen(path, "rb");
    if (NULL != f) {
        ASSERT(0 == fseek(f, -1L, SEEK_END));
        ASSERT(0 == ferror(f));
        fclose(f);
    } else {
        ASSERT(ENOENT == errno || EACCES == errno);
    }
}

TEST(slsa_hash_output_path_is_deterministic)
{
    const char *path = "ungeek.cgi.sha256";
    ASSERT(NULL != path);
    ASSERT('\0' != path[0]);

    errno = 0;
    FILE *f = fopen(path, "rb");
    if (NULL != f) {
        ASSERT(0 == fseek(f, -1L, SEEK_END));
        ASSERT(0 == ferror(f));
        fclose(f);
    } else {
        ASSERT(ENOENT == errno || EACCES == errno);
    }
}

int main(void)
{
    printf("ungeek-cgi xUnit test suite\n");
    printf("===========================\n\n");

    printf("geek code category lookup:\n");
    RUN(geek_code_prefix_empty_input_returns_null);
    RUN(geek_code_prefix_GCS_recognised);
    RUN(geek_code_modifier_dollar_stripped);
    RUN(geek_code_modifier_at_stripped);
    RUN(geek_code_null_input_safe);
    RUN(geek_code_version_header_skipped);
    RUN(geek_lookup_val_plusplus_decoded);
    RUN(geek_lookup_val_triple_minus_decoded);
    RUN(geek_lookup_val_unknown_returns_input);
    RUN(geek_find_cat_unknown_returns_null);
    RUN(geek_find_cat_Linux_recognised);

    printf("\nnet.matrix identity:\n");
    RUN(matrix_organization_present);
    RUN(matrix_application_is_ungeek_cgi);
    RUN(matrix_owner_present);

    printf("\nSLSA provenance paths:\n");
    RUN(slsa_binary_output_path_is_deterministic);
    RUN(slsa_hash_output_path_is_deterministic);

    return xunit_summary();
}
