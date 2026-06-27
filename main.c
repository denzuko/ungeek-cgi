/*
 * main.c — Geek Code v3 decoder CGI
 *
 * Routes:
 *   GET  /              — form page
 *   POST /?code=...     — htmx partial: SVG badge grid
 *
 * Design: Da Planet terminal aesthetic
 *   Share Tech Mono + Rajdhani, neon green (#39ff14) primary
 *   Per-section colour theming via geek.h tokens
 *
 * Stack: kcgi/kcgihtml, tsoding arena.h/sv.h, htmx, Alpine.js
 *
 * SPDX-License-Identifier: BSD-2-Clause
 * Da Planet Security / denzuko <denzuko@dapla.net>
 */

#define ARENA_IMPLEMENTATION
#include "arena.h"
#define SV_IMPLEMENTATION
#include "sv.h"
#include "matrix_id.h"
#include "geek.h"

#include <sys/types.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <kcgi.h>
#include <kcgihtml.h>

/* ── Constants ───────────────────────────────────────────────────────────── */

#define MAX_CODE_LEN   2048
#define MAX_TOKENS     64
#define BADGE_W        260
#define BADGE_H        90

#define HTMX_CDN   "https://unpkg.com/htmx.org@1.9.12"
#define ALPINE_CDN "https://unpkg.com/alpinejs@3.13.3/dist/cdn.min.js"
#define GFONTS_URL "https://fonts.googleapis.com/css2?family=Share+Tech+Mono&family=Rajdhani:wght@600&display=swap"

/* ── CSS tokens ──────────────────────────────────────────────────────────── */

#define CSS_BODY      "background:#0a0a0a;color:#39ff14;min-height:100vh;font-family:'Share Tech Mono',monospace;padding:1rem"
#define CSS_CONTAINER "max-width:900px;margin:0 auto"
#define CSS_TITLE     "font-family:Rajdhani,sans-serif;font-size:2rem;font-weight:600;color:#39ff14;margin-bottom:1.5rem"
#define CSS_TEXTAREA  "width:100%;background:#0d1117;border:1px solid #1f2937;color:#39ff14;padding:.75rem;font-family:'Share Tech Mono',monospace;font-size:.85rem;resize:vertical"
#define CSS_BTN       "background:#39ff14;color:#000;padding:.5rem 1.5rem;font-weight:700;font-family:Rajdhani,sans-serif;border:none;cursor:pointer;margin-top:.5rem"
#define CSS_GRID      "display:flex;flex-wrap:wrap;gap:.5rem;margin-top:1.5rem"
#define CSS_SECTION   "width:100%;font-family:Rajdhani,sans-serif;font-size:.9rem;color:#6b7280;margin:.75rem 0 .25rem"
#define CSS_ERROR     "color:#ff1744;font-family:'Share Tech Mono',monospace"

/* ── kcgi keys ───────────────────────────────────────────────────────────── */

enum Key { KEY_CODE = 0, KEY__MAX };

static const struct kvalid KEYS[KEY__MAX] = {
    { kvalid_stringne, "code" },
};

/* ── Token type ──────────────────────────────────────────────────────────── */

typedef struct {
    const GeekCat *cat;
    char           val[32];   /* raw value with modifiers */
} Token;

/* ── Tokeniser ───────────────────────────────────────────────────────────── */

/*
 * Parse a Geek Code string into tokens.
 * Returns number of tokens found (capped at MAX_TOKENS).
 * Skips the "Version:" header line if present.
 */
static int tokenise(const char *code, Token *out, int max)
{
    if (NULL == code || '\0' == code[0]) return 0;

    int n = 0;
    const char *p = code;

    /* Skip "Version: X.Y" header line */
    if (0 == strncmp(p, "Version:", 8)) {
        while (*p && '\n' != *p) p++;
        if ('\n' == *p) p++;
    }
    /* Also skip "GCx" type header if present */
    if ('G' == *p && isupper((unsigned char)p[1])) {
        /* geek type prefix — record it then advance past spaces */
    }

    while (*p && n < max) {
        /* Skip whitespace */
        while (*p && isspace((unsigned char)*p)) p++;
        if ('\0' == *p) break;

        /* Try longest matching category prefix */
        const GeekCat *best = NULL;
        size_t best_len = 0;

        for (size_t i = 0; NULL != GEEK_CATS[i].prefix; i++) {
            size_t plen = strlen(GEEK_CATS[i].prefix);
            if (0 == strncmp(p, GEEK_CATS[i].prefix, plen) &&
                plen > best_len) {
                best     = &GEEK_CATS[i];
                best_len = plen;
            }
        }

        if (NULL == best) {
            /* Unknown token — advance past this word */
            while (*p && !isspace((unsigned char)*p)) p++;
            continue;
        }

        p += best_len;  /* advance past category prefix */

        /* Collect value characters: +, -, 0, !, ?, *, $, @ */
        size_t vlen = 0;
        char   vbuf[32];
        while (*p && !isspace((unsigned char)*p) &&
               vlen < sizeof(vbuf) - 1) {
            vbuf[vlen++] = *p++;
        }
        vbuf[vlen] = '\0';

        if (vlen > 0) {
            out[n].cat = best;
            strncpy(out[n].val, vbuf, sizeof(out[n].val) - 1);
            out[n].val[sizeof(out[n].val) - 1] = '\0';
            n++;
        }
    }
    return n;
}

/* ── SVG badge emission ──────────────────────────────────────────────────── */

/*
 * Emit one inline SVG badge for a token.
 * Layout: accent bar left (6px) | category pill top | value raw | decoded desc
 */
static void emit_badge(struct khtmlreq *html, const Token *t)
{
    const char *colour  = t->cat->colour;
    const char *catname = t->cat->name;
    const char *decoded = geek_lookup_val(t->val);

    /* Build SVG as a string via snprintf into a stack buffer */
    char svg[1024];
    snprintf(svg, sizeof(svg),
        "<svg xmlns='http://www.w3.org/2000/svg' "
             "width='%d' height='%d' "
             "style='background:#0d1117;border:1px solid #1f2937'>"
          /* accent bar */
          "<rect x='0' y='0' width='6' height='%d' fill='%s'/>"
          /* category label */
          "<text x='14' y='18' "
                "font-family='Rajdhani,sans-serif' "
                "font-size='11' font-weight='600' fill='%s'>%s</text>"
          /* raw value */
          "<text x='14' y='42' "
                "font-family='Share Tech Mono,monospace' "
                "font-size='20' fill='%s'>%s</text>"
          /* decoded description */
          "<text x='14' y='72' "
                "font-family='Share Tech Mono,monospace' "
                "font-size='11' fill='#6b7280'>%s</text>"
        "</svg>",
        BADGE_W, BADGE_H, BADGE_H,
        colour, colour, catname,
        colour, t->val,
        decoded);

    /*
     * Emit SVG badge entirely via khttp_puts on the underlying kreq.
     * khtml does not provide a raw-string emit path; we bypass it here
     * only for the SVG payload. The surrounding flex container is also
     * emitted raw so element-depth tracking inside khtml is unaffected.
     * (Reopening khtml mid-stream loses depth state — avoided entirely.)
     */
    khttp_puts(html->req, "<div style=\"display:inline-block\">");
    khttp_puts(html->req, svg);
    khttp_puts(html->req, "</div>");
}

/* Emit section divider label — raw emit to match badge path */
static void emit_section(struct khtmlreq *html, const char *section)
{
    khttp_puts(html->req,
        "<div style=\"" CSS_SECTION "\">");
    khttp_puts(html->req, section);
    khttp_puts(html->req, "</div>");
}

/* Emit the full badge grid for a set of tokens */
static void emit_grid(struct khtmlreq *html, const Token *toks, int n)
{
    /* Open grid container raw — badges are also raw, keeps depth clean */
    khttp_puts(html->req,
        "<div id=\"result\" style=\"" CSS_GRID "\">");

    if (0 == n) {
        khttp_puts(html->req,
            "<p style=\"" CSS_ERROR "\">"
            "no recognised Geek Code tokens found</p>");
        khttp_puts(html->req, "</div>");
        return;
    }

    const char *cur_section = "";
    for (int i = 0; i < n; i++) {
        if (0 != strcmp(toks[i].cat->section, cur_section)) {
            cur_section = toks[i].cat->section;
            emit_section(html, cur_section);
        }
        emit_badge(html, &toks[i]);
    }

    khttp_puts(html->req, "</div>"); /* /div#result */
}

/* ── Render helpers ──────────────────────────────────────────────────────── */

static void render_head(struct khtmlreq *html)
{
    khtml_elem(html, KELEM_HEAD);
    khtml_attr(html, KELEM_META,
        KATTR_NAME, "viewport",
        KATTR_CONTENT, "width=device-width,initial-scale=1",
        KATTR__MAX);
    khtml_attr(html, KELEM_LINK,
        KATTR_REL, "stylesheet",
        KATTR_HREF, GFONTS_URL,
        KATTR__MAX);
    khtml_attr(html, KELEM_SCRIPT,
        KATTR_SRC, HTMX_CDN, KATTR__MAX);
    khtml_closeelem(html, 1);
    khtml_attr(html, KELEM_SCRIPT,
        KATTR_SRC, ALPINE_CDN,
        KATTR_DEFER, "", KATTR__MAX);
    khtml_closeelem(html, 1);
    khtml_elem(html, KELEM_TITLE);
    khtml_puts(html, "ungeek — Geek Code decoder");
    khtml_closeelem(html, 2); /* /title /head */
}

static void render_form(struct khtmlreq *html, const char *prefill)
{
    khtml_attr(html, KELEM_FORM,
        KATTR_ENCTYPE, "application/x-www-form-urlencoded",
        KATTR_HXPOST, "",
        KATTR_HXTARGET, "#result",
        KATTR_HXSWAP, "outerHTML",
        KATTR__MAX);

    khtml_attr(html, KELEM_TEXTAREA,
        KATTR_NAME, "code",
        KATTR_ROWS, "6",
        KATTR_PLACEHOLDER,
            "Paste your Geek Code here...\n"
            "e.g. GCS d-- s+: a-- C++ UL++ P+ L++ E--- W+++ N+ o-- K- w---",
        KATTR_STYLE, CSS_TEXTAREA,
        KATTR__MAX);
    if (NULL != prefill)
        khtml_puts(html, prefill);
    khtml_closeelem(html, 1); /* /textarea */

    khtml_attr(html, KELEM_BUTTON,
        KATTR_TYPE, "submit",
        KATTR_STYLE, CSS_BTN,
        KATTR__MAX);
    khtml_puts(html, "decode");
    khtml_closeelem(html, 2); /* /button /form */
}

/* ── Entry point ─────────────────────────────────────────────────────────── */

int main(void)
{
    struct kreq    r    = {0};
    struct khtmlreq html = {0};
    Arena          a    = {0};

    if (KCGI_OK != khttp_parse(&r, KEYS, KEY__MAX, NULL, 0, 0))
        return 1;

    int is_htmx = (NULL != r.reqmap[KREQU_HX_REQUEST]);

    khttp_head(&r, kresps[KRESP_STATUS],   "%s", khttps[KHTTP_200]);
    khttp_head(&r, kresps[KRESP_CONTENT_TYPE], "%s", kmimetypes[KMIME_TEXT_HTML]);
    khttp_body(&r);

    khtml_open(&html, &r, KHTML_PRETTY);

    if (!is_htmx) {
        khtml_elem(&html, KELEM_DOCTYPE);
        khtml_elem(&html, KELEM_HTML);
        render_head(&html);
        khtml_attr(&html, KELEM_BODY, KATTR_STYLE, CSS_BODY, KATTR__MAX);
        khtml_attr(&html, KELEM_DIV,  KATTR_STYLE, CSS_CONTAINER, KATTR__MAX);
        khtml_attr(&html, KELEM_H1,   KATTR_STYLE, CSS_TITLE, KATTR__MAX);
        khtml_puts(&html, "ungeek");
        khtml_closeelem(&html, 1);
    }

    const char *code_raw = (NULL != r.fieldmap[KEY_CODE])
        ? r.fieldmap[KEY_CODE]->val : NULL;

    render_form(&html, code_raw);

    if (NULL != code_raw && '\0' != code_raw[0]) {
        /* Sanitise length */
        char *code = arena_alloc(&a, MAX_CODE_LEN + 1);
        strncpy(code, code_raw, MAX_CODE_LEN);
        code[MAX_CODE_LEN] = '\0';

        Token toks[MAX_TOKENS];
        int n = tokenise(code, toks, MAX_TOKENS);
        emit_grid(&html, toks, n);
    } else {
        khttp_puts(&r, "<div id=\"result\"></div>");
    }

    if (!is_htmx)
        khtml_closeelem(&html, 4); /* /div /body /html */

    khtml_close(&html);
    khttp_free(&r);
    arena_free(&a);

    return 0;
}
