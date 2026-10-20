/*
 * QuickJS C library
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
#ifndef QUICKJSI_LIBC_H
#define QUICKJSI_LIBC_H

#include <stdio.h>
#include <stdlib.h>

#include "quickjs_i.h"

#ifdef __cplusplus
extern "C" {
#endif

JSModuleDef *jsi_init_module_std(JSIContext *ctx, const char *module_name);
JSModuleDef *jsi_init_module_os(JSIContext *ctx, const char *module_name);
void jsi_std_add_helpers(JSIContext *ctx, int argc, char **argv);
void jsi_std_loop(JSIContext *ctx);
JSIValue jsi_std_await(JSIContext *ctx, JSIValue obj);
void jsi_std_init_handlers(JSIRuntime *rt);
void jsi_std_free_handlers(JSIRuntime *rt);
void jsi_std_dump_error(JSIContext *ctx);
uint8_t *jsi_load_file(JSIContext *ctx, size_t *pbuf_len, const char *filename);
int jsi_module_set_import_meta(JSIContext *ctx, JSIValueConst func_val,
                              JSI_BOOL use_realpath, JSI_BOOL is_main);
int jsi_module_test_json(JSIContext *ctx, JSIValueConst attributes);
int jsi_module_check_attributes(JSIContext *ctx, void *opaque, JSIValueConst attributes);
JSModuleDef *jsi_module_loader(JSIContext *ctx,
                              const char *module_name, void *opaque,
                              JSIValueConst attributes);
void jsi_std_eval_binary(JSIContext *ctx, const uint8_t *buf, size_t buf_len,
                        int flags);
void jsi_std_eval_binary_json_module(JSIContext *ctx,
                                    const uint8_t *buf, size_t buf_len,
                                    const char *module_name);
void jsi_std_promise_rejection_tracker(JSIContext *ctx, JSIValueConst promise,
                                      JSIValueConst reason,
                                      JSI_BOOL is_handled, void *opaque);
void jsi_std_set_worker_new_context_func(JSIContext *(*func)(JSIRuntime *rt));

#ifdef __cplusplus
} /* extern "C" { */
#endif

#endif /* QUICKJSI_LIBC_H */
