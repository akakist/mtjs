/*
 * Unicode utilities
 *
 * Copyright (c) 2017-2018 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef LIBUNICODE_H
#define LIBUNICODE_H

#include <stdint.h>

/* define it to include all the unicode tables (40KB larger) */
#define CONFIG_ALL_UNICODE

#define LRE_CC_RES_LEN_MAX 3

/* char ranges */

typedef struct {
    int len; /* in points, always even */
    int size;
    uint32_t *points; /* points sorted by increasing value */
    void *mem_opaque;
    void *(*realloc_func)(void *opaque, void *ptr, size_t size);
} CharRange;

typedef enum {
    CR_OP_UNION,
    CR_OP_INTER,
    CR_OP_XOR,
    CR_OP_SUB,
} CharRangeOpEnum;





static int cr_invert(CharRange *cr);


typedef enum {
    UNICODE_NFC,
    UNICODE_NFD,
    UNICODE_NFKC,
    UNICODE_NFKD,
} UnicodeNormalizationEnum;

int unicode_jsi_normalize(uint32_t **pdst, const uint32_t *src, int src_len,
                      UnicodeNormalizationEnum n_type,
                      void *opaque, void *(*realloc_func)(void *opaque, void *ptr, size_t size));

/* Unicode character range functions */

int unicode_jsi_script(CharRange *cr, const char *script_name, int is_ext);
int unicode_jsi_general_category(CharRange *cr, const char *gc_name);
int unicode_jsi_prop(CharRange *cr, const char *prop_name);

typedef void UnicodeSequencePropCB(void *opaque, const uint32_t *buf, int len);
int unicode_jsi_sequence_prop(const char *prop_name, UnicodeSequencePropCB *cb, void *opaque,
                          CharRange *cr);

int lre_jsi_case_conv(uint32_t *res, uint32_t c, int conv_type);
int lre_jsi_canonicalize(uint32_t c, int is_unicode);

/* Code point type categories */
enum {
    UNICODE_C_SPACE  = (1 << 0),
    UNICODE_C_DIGIT  = (1 << 1),
    UNICODE_C_UPPER  = (1 << 2),
    UNICODE_C_LOWER  = (1 << 3),
    UNICODE_C_UNDER  = (1 << 4),
    UNICODE_C_DOLLAR = (1 << 5),
    UNICODE_C_XDIGIT = (1 << 6),
};
extern uint8_t const lre_jsi_ctype_bits[256];

/* zero or non-zero return value */
int lre_jsi_is_cased(uint32_t c);
int lre_jsi_is_case_ignorable(uint32_t c);
int lre_jsi_is_id_start(uint32_t c);
int lre_jsi_is_id_continue(uint32_t c);

static inline int lre_jsi_is_space_byte(uint8_t c) {
    return lre_jsi_ctype_bits[c] & UNICODE_C_SPACE;
}

static inline int lre_jsi_is_id_start_byte(uint8_t c) {
    return lre_jsi_ctype_bits[c] & (UNICODE_C_UPPER | UNICODE_C_LOWER |
                                UNICODE_C_UNDER | UNICODE_C_DOLLAR);
}

static inline int lre_jsi_is_id_continue_byte(uint8_t c) {
    return lre_jsi_ctype_bits[c] & (UNICODE_C_UPPER | UNICODE_C_LOWER |
                                UNICODE_C_UNDER | UNICODE_C_DOLLAR |
                                UNICODE_C_DIGIT);
}

int lre_jsi_is_space_non_ascii(uint32_t c);

static inline int lre_jsi_is_space(uint32_t c) {
    if (c < 256)
        return lre_jsi_is_space_byte(c);
    else
        return lre_jsi_is_space_non_ascii(c);
}

static inline int lre_jsi_jsi_is_ident_first(uint32_t c) {
    if (c < 128) {
        return lre_jsi_is_id_start_byte(c);
    } else {
#ifdef CONFIG_ALL_UNICODE
        return lre_jsi_is_id_start(c);
#else
        return !lre_jsi_is_space_non_ascii(c);
#endif
    }
}

static inline int lre_jsi_ident_next(uint32_t c) {
    if (c < 128) {
        return lre_jsi_is_id_continue_byte(c);
    } else {
        /* ZWNJ and ZWJ are accepted in identifiers */
        if (c >= 0x200C && c <= 0x200D)
            return TRUE;
#ifdef CONFIG_ALL_UNICODE
        return lre_jsi_is_id_continue(c);
#else
        return !lre_jsi_is_space_non_ascii(c);
#endif
    }
}

#endif /* LIBUNICODE_H */
