/*
 * geek.h — Geek Code v3 category/value table
 *
 * Category strings and decoded descriptions written independently
 * from the Geek Code v3.12 specification (Robert Hayden, 1996).
 * Not derived from any GPL source.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 * Da Planet Security / denzuko <denzuko@dapla.net>
 */

#ifndef GEEK_H
#define GEEK_H

#include <stddef.h>
#include <string.h>

/* ── Section colour tokens (Da Planet design system) ─────────────────────── */

#define COL_COMPUTERS  "#39ff14"   /* neon green  */
#define COL_POLITICS   "#a855f7"   /* purple       */
#define COL_ENTERTAIN  "#f97316"   /* orange       */
#define COL_APPEARANCE "#3b82f6"   /* blue         */
#define COL_LIFESTYLE  "#22c55e"   /* forest green */
#define COL_OTHER      "#6b7280"   /* grey         */

typedef struct {
    const char *prefix;   /* e.g. "GCS", "d", "p" */
    const char *name;     /* human-readable name   */
    const char *section;  /* section label         */
    const char *colour;   /* CSS hex colour token  */
} GeekCat;

typedef struct {
    const char *code;     /* raw value, e.g. "+++" */
    const char *desc;     /* decoded description   */
} GeekVal;

/* ── Category table ──────────────────────────────────────────────────────── */

static const GeekCat GEEK_CATS[] = {
    /* Computers */
    { "GCS",  "Computer Science",          "Computers",    COL_COMPUTERS  },
    { "GE",   "Engineering",               "Computers",    COL_COMPUTERS  },
    { "GMU",  "Music",                     "Computers",    COL_COMPUTERS  },
    { "GU",   "Unix",                      "Computers",    COL_COMPUTERS  },
    { "GB",   "Biology",                   "Computers",    COL_COMPUTERS  },
    { "GA",   "Architecture",              "Computers",    COL_COMPUTERS  },
    { "GH",   "Humanities",               "Computers",    COL_COMPUTERS  },
    { "GLS",  "Law",                       "Computers",    COL_COMPUTERS  },
    { "GL",   "Literature",                "Computers",    COL_COMPUTERS  },
    { "GM",   "Math",                      "Computers",    COL_COMPUTERS  },
    { "GMD",  "Medicine",                  "Computers",    COL_COMPUTERS  },
    { "GP",   "Physics",                   "Computers",    COL_COMPUTERS  },
    { "GS",   "Science (general)",         "Computers",    COL_COMPUTERS  },
    { "GO",   "Other",                     "Computers",    COL_OTHER      },
    /* Skills */
    { "d",    "Dress",                     "Appearance",   COL_APPEARANCE },
    { "e",    "Education",                 "Lifestyle",    COL_LIFESTYLE  },
    { "p",    "Politics",                  "Politics",     COL_POLITICS   },
    { "r",    "Religion",                  "Lifestyle",    COL_LIFESTYLE  },
    { "s",    "Sex",                       "Lifestyle",    COL_LIFESTYLE  },
    { "a",    "Age",                       "Appearance",   COL_APPEARANCE },
    { "y",    "Net years",                 "Computers",    COL_COMPUTERS  },
    /* Technical */
    { "C",    "C proficiency",             "Computers",    COL_COMPUTERS  },
    { "C++",  "C++ proficiency",           "Computers",    COL_COMPUTERS  },
    { "CM",   "CMU",                       "Computers",    COL_COMPUTERS  },
    { "E",    "Emacs",                     "Computers",    COL_COMPUTERS  },
    { "G",    "GIMP",                      "Computers",    COL_COMPUTERS  },
    { "H",    "Hardware",                  "Computers",    COL_COMPUTERS  },
    { "I",    "Internet",                  "Computers",    COL_COMPUTERS  },
    { "K",    "Kibo",                      "Computers",    COL_COMPUTERS  },
    { "L",    "Linux",                     "Computers",    COL_COMPUTERS  },
    { "MC",   "MicroComputers",            "Computers",    COL_COMPUTERS  },
    { "MUD",  "MUD/MUSH",                  "Entertain",    COL_ENTERTAIN  },
    { "O",    "OS proficiency",            "Computers",    COL_COMPUTERS  },
    { "P",    "Perl proficiency",          "Computers",    COL_COMPUTERS  },
    { "PE",   "People/Dating skills",      "Lifestyle",    COL_LIFESTYLE  },
    { "PS",   "Philosophy/Psych",          "Politics",     COL_POLITICS   },
    { "PGP",  "PGP",                       "Computers",    COL_COMPUTERS  },
    { "R",    "Role-Playing Games",        "Entertain",    COL_ENTERTAIN  },
    { "T",    "TV",                        "Entertain",    COL_ENTERTAIN  },
    { "TLA",  "TLA fluency",               "Computers",    COL_COMPUTERS  },
    { "U",    "Unix",                      "Computers",    COL_COMPUTERS  },
    { "UB",   "Usenet Bandwidth",          "Computers",    COL_COMPUTERS  },
    { "VB",   "Visual Basic",              "Computers",    COL_COMPUTERS  },
    { "W",    "World Wide Web",            "Computers",    COL_COMPUTERS  },
    { "X",    "X Windows",                 "Computers",    COL_COMPUTERS  },
    { NULL,   NULL,                        NULL,           NULL           }
};

/* ── Value table (generic — applies across most categories) ──────────────── */

static const GeekVal GEEK_VALS[] = {
    { "+++",  "guru / expert" },
    { "++",   "proficient" },
    { "+",    "competent" },
    { "0",    "no opinion / neutral" },
    { "-",    "avoid" },
    { "--",   "strongly avoid" },
    { "---",  "actively opposed" },
    { "!",    "refuse to participate" },
    { "?",    "no knowledge" },
    { "*",    "varies" },
    { NULL,   NULL }
};

/* ── Lookup helpers ──────────────────────────────────────────────────────── */

/* Strip trailing modifier characters ($, @) from value before lookup. */
static const char *geek_strip_modifier(char *buf, size_t bufsz,
                                        const char *val)
{
    size_t n = strlen(val);
    if (n >= bufsz) n = bufsz - 1;
    strncpy(buf, val, n);
    buf[n] = '\0';
    while (n > 0 && ('$' == buf[n-1] || '@' == buf[n-1]))
        buf[--n] = '\0';
    return buf;
}

/* Return pointer to matching GeekCat, or NULL. */
static const GeekCat *geek_find_cat(const char *prefix)
{
    if (NULL == prefix) return NULL;
    for (size_t i = 0; NULL != GEEK_CATS[i].prefix; i++)
        if (0 == strcmp(prefix, GEEK_CATS[i].prefix))
            return &GEEK_CATS[i];
    return NULL;
}

/* Return decoded description for value, or val itself if not found. */
static const char *geek_lookup_val(const char *val)
{
    char buf[32];
    const char *stripped = geek_strip_modifier(buf, sizeof(buf), val);
    for (size_t i = 0; NULL != GEEK_VALS[i].code; i++)
        if (0 == strcmp(stripped, GEEK_VALS[i].code))
            return GEEK_VALS[i].desc;
    return val;
}

#endif /* GEEK_H */
