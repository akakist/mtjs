/*
 * QuickJS Javascript Engine
 *
 * Copyright (c) 2017-2021 Fabrice Bellard
 * Copyright (c) 2017-2021 Charlie Gordon
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
#ifndef QUICKJSI_H
#define QUICKJSI_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__GNUC__) || defined(__clang__)
#define jsi_likely(x)          __builtin_expect(!!(x), 1)
#define jsi_unlikely(x)        __builtin_expect(!!(x), 0)
#define jsi_force_inline       inline __attribute__((always_inline))
#define __jsi_printf_like(f, a)   __attribute__((format(printf, f, a)))
#else
#define jsi_likely(x)     (x)
#define jsi_unlikely(x)   (x)
#define jsi_force_inline  inline
#define __jsi_printf_like(a, b)
#endif

#define JSI_BOOL int

typedef struct JSRuntime JSRuntime;
typedef struct JSContext JSContext;
typedef struct JSClass JSClass;
typedef uint32_t JSClassID;
typedef uint32_t JSAtom;

#if INTPTR_MAX >= INT64_MAX
#define JSI_PTR64
#define JSI_PTR64_DEF(a) a
#else
#define JSI_PTR64_DEF(a)
#endif

#ifndef JSI_PTR64
#define JSI_NAN_BOXING
#endif

#if defined(__SIZEOF_INT128__) && (INTPTR_MAX >= INT64_MAX)
#define JSI_LIMB_BITS 64
#else
#define JSI_LIMB_BITS 32
#endif

#define JSI_SHORT_BIG_INT_BITS JSI_LIMB_BITS
    
enum {
    /* all tags with a reference count are negative */
    JSI_TAG_FIRST       = -9, /* first negative tag */
    // JSI_TAG_BIG_INT     = -9,
    JSI_TAG_SYMBOL      = -8,
    JSI_TAG_STRING      = -7,
    JSI_TAG_STRING_ROPE = -6,
    JSI_TAG_MODULE      = -3, /* used internally */
    JSI_TAG_FUNCTION_BYTECODE = -2, /* used internally */
    JSI_TAG_OBJECT      = -1,

    JSI_TAG_INT         = 0,
    JSI_TAG_BOOL        = 1,
    JSI_TAG_NULL        = 2,
    JSI_TAG_UNDEFINED   = 3,
    JSI_TAG_UNINITIALIZED = 4,
    JSI_TAG_CATCH_OFFSET = 5,
    JSI_TAG_EXCEPTION   = 6,
    // JSI_TAG_SHORT_BIG_INT = 7,
    // JSI_TAG_FLOAT64     = 8,
    /* any larger tag is FLOAT64 if JSI_NAN_BOXING */
};

typedef struct JSRefCountHeader {
    int ref_count;
} JSRefCountHeader;

#define JSI_FLOAT64_NAN NAN

#ifdef CONFIG_CHECK_JSVALUE
/* JSValue consistency : it is not possible to run the code in this
   mode, but it is useful to detect simple reference counting
   errors. It would be interesting to modify a static C analyzer to
   handle specific annotations (clang has such annotations but only
   for objective C) */
typedef struct __JSValue *JSValue;
typedef const struct __JSValue *JSValueConst;

#define JSI_VALUE_GET_TAG(v) (int)((uintptr_t)(v) & 0xf)
/* same as JSI_VALUE_GET_TAG, but return JSI_TAG_FLOAT64 with NaN boxing */
#define JSI_VALUE_GET_NORM_TAG(v) JSI_VALUE_GET_TAG(v)
#define JSI_VALUE_GET_INT(v) (int)((intptr_t)(v) >> 4)
#define JSI_VALUE_GET_BOOL(v) JSI_VALUE_GET_INT(v)
#define JSI_VALUE_GET_FLOAT64(v) (double)JSI_VALUE_GET_INT(v)
#define JSI_VALUE_GET_SHORT_BIG_INT(v) JSI_VALUE_GET_INT(v)
#define JSI_VALUE_GET_PTR(v) (void *)((intptr_t)(v) & ~0xf)

#define JSI_MKVAL(tag, val) (JSValue)(intptr_t)(((val) << 4) | (tag))
#define JSI_MKPTR(tag, p) (JSValue)((intptr_t)(p) | (tag))

#define JSI_TAG_IS_FLOAT64(tag) ((unsigned)(tag) == JSI_TAG_FLOAT64)

#define JSI_NAN JSI_MKVAL(JSI_TAG_FLOAT64, 1)

static inline JSValue __JSI_NewFloat64(JSContext *ctx, double d)
{
    return JSI_MKVAL(JSI_TAG_FLOAT64, (int)d);
}

static inline JSI_BOOL JSI_VALUE_IS_NAN(JSValue v)
{
    return 0;
}

static inline JSValue __JSI_NewShortBigInt(JSContext *ctx, int32_t d)
{
    return JSI_MKVAL(JSI_TAG_SHORT_BIG_INT, d);
}

#elif defined(JSI_NAN_BOXING)

typedef uint64_t JSValue;

#define JSValueConst JSValue

#define JSI_VALUE_GET_TAG(v) (int)((v) >> 32)
#define JSI_VALUE_GET_INT(v) (int)(v)
#define JSI_VALUE_GET_BOOL(v) (int)(v)
#define JSI_VALUE_GET_SHORT_BIG_INT(v) (int)(v)
#define JSI_VALUE_GET_PTR(v) (void *)(intptr_t)(v)

#define JSI_MKVAL(tag, val) (((uint64_t)(tag) << 32) | (uint32_t)(val))
#define JSI_MKPTR(tag, ptr) (((uint64_t)(tag) << 32) | (uintptr_t)(ptr))

#define JSI_FLOAT64_TAG_ADDEND (0x7ff80000 - JSI_TAG_FIRST + 1) /* quiet NaN encoding */

static inline double JSI_VALUE_GET_FLOAT64(JSValue v)
{
    union {
        JSValue v;
        double d;
    } u;
    u.v = v;
    u.v += (uint64_t)JSI_FLOAT64_TAG_ADDEND << 32;
    return u.d;
}

#define JSI_NAN (0x7ff8000000000000 - ((uint64_t)JSI_FLOAT64_TAG_ADDEND << 32))

static inline JSValue __JSI_NewFloat64(JSContext *ctx, double d)
{
    union {
        double d;
        uint64_t u64;
    } u;
    JSValue v;
    u.d = d;
    /* normalize NaN */
    if (jsi_unlikely((u.u64 & 0x7fffffffffffffff) > 0x7ff0000000000000))
        v = JSI_NAN;
    else
        v = u.u64 - ((uint64_t)JSI_FLOAT64_TAG_ADDEND << 32);
    return v;
}

#define JSI_TAG_IS_FLOAT64(tag) ((unsigned)((tag) - JSI_TAG_FIRST) >= (JSI_TAG_FLOAT64 - JSI_TAG_FIRST))

/* same as JSI_VALUE_GET_TAG, but return JSI_TAG_FLOAT64 with NaN boxing */
static inline int JSI_VALUE_GET_NORM_TAG(JSValue v)
{
    uint32_t tag;
    tag = JSI_VALUE_GET_TAG(v);
    if (JSI_TAG_IS_FLOAT64(tag))
        return JSI_TAG_FLOAT64;
    else
        return tag;
}

static inline JSI_BOOL JSI_VALUE_IS_NAN(JSValue v)
{
    uint32_t tag;
    tag = JSI_VALUE_GET_TAG(v);
    return tag == (JSI_NAN >> 32);
}

static inline JSValue __JSI_NewShortBigInt(JSContext *ctx, int32_t d)
{
    return JSI_MKVAL(JSI_TAG_SHORT_BIG_INT, d);
}

#else /* !JSI_NAN_BOXING */

typedef union JSValueUnion {
    int64_t int64;
    uint64_t uint64;
    // double float64;
    void *ptr;
#if JSI_SHORT_BIG_INT_BITS == 32
    int32_t short_big_int;
#else
    int64_t short_big_int;
#endif
} JSValueUnion;

typedef struct JSValue {
    JSValueUnion u;
    int32_t tag;
    // int is_nan;
} JSValue;

#define JSValueConst JSValue

#define JSI_VALUE_GET_TAG(v) ((int32_t)(v).tag)
/* same as JSI_VALUE_GET_TAG, but return JSI_TAG_FLOAT64 with NaN boxing */
#define JSI_VALUE_GET_NORM_TAG(v) JSI_VALUE_GET_TAG(v)
#define JSI_VALUE_GET_INT(v) ((v).u.int64)
#define JSI_VALUE_GET_BOOL(v) ((v).u.int64)
#define JSI_VALUE_GET_FLOAT64(v) ((v).u.float64)
#define JSI_VALUE_GET_SHORT_BIG_INT(v) ((v).u.short_big_int)
#define JSI_VALUE_GET_PTR(v) ((v).u.ptr)

#define JSI_MKVAL(tag, val) (JSValue){ (JSValueUnion){ .int64 = val }, tag }
#define JSI_MKVALU(tag, val) (JSValue){ (JSValueUnion){ .uint64 = val }, tag}
#define JSI_MKPTR(tag, p) (JSValue){ (JSValueUnion){ .ptr = p }, tag}

// #define JSI_TAG_IS_FLOAT64(tag) ((unsigned)(tag) == JSI_TAG_FLOAT64)

// #define JSI_NAN (JSValue){ .u.float64 = JSI_FLOAT64_NAN, .tag=JSI_TAG_FLOAT64, .is_nan=1 }


static inline JSI_BOOL JSI_VALUE_IS_NAN(JSValue v)
{
    return v.tag == JSI_TAG_UNDEFINED;
    // union {
    //     double d;
    //     uint64_t u64;
    // } u;
    // if (v.tag != JSI_TAG_FLOAT64)
    //     return 0;
    // u.d = v.u.float64;
    // return (u.u64 & 0x7fffffffffffffff) > 0x7ff0000000000000;
}


#endif /* !JSI_NAN_BOXING */

#define JSI_VALUE_IS_BOTH_INT(v1, v2) ((JSI_VALUE_GET_TAG(v1) | JSI_VALUE_GET_TAG(v2)) == 0)
#define JSI_VALUE_IS_BOTH_FLOAT(v1, v2) (JSI_TAG_IS_FLOAT64(JSI_VALUE_GET_TAG(v1)) && JSI_TAG_IS_FLOAT64(JSI_VALUE_GET_TAG(v2)))

#define JSI_VALUE_HAS_REF_COUNT(v) ((unsigned)JSI_VALUE_GET_TAG(v) >= (unsigned)JSI_TAG_FIRST)

/* special values */
#define JSI_NULL      JSI_MKVAL(JSI_TAG_NULL, 0)
#define JSI_UNDEFINED JSI_MKVAL(JSI_TAG_UNDEFINED, 0)
#define JSI_FALSE     JSI_MKVAL(JSI_TAG_BOOL, 0)
#define JSI_TRUE      JSI_MKVAL(JSI_TAG_BOOL, 1)
#define JSI_EXCEPTION JSI_MKVAL(JSI_TAG_EXCEPTION, 0)
#define JSI_UNINITIALIZED JSI_MKVAL(JSI_TAG_UNINITIALIZED, 0)

/* flags for object properties */
#define JSI_PROP_CONFIGURABLE  (1 << 0)
#define JSI_PROP_WRITABLE      (1 << 1)
#define JSI_PROP_ENUMERABLE    (1 << 2)
#define JSI_PROP_C_W_E         (JSI_PROP_CONFIGURABLE | JSI_PROP_WRITABLE | JSI_PROP_ENUMERABLE)
#define JSI_PROP_LENGTH        (1 << 3) /* used internally in Arrays */
#define JSI_PROP_TMASK         (3 << 4) /* mask for NORMAL, GETSET, VARREF, AUTOINIT */
#define JSI_PROP_NORMAL         (0 << 4)
#define JSI_PROP_GETSET         (1 << 4)
#define JSI_PROP_VARREF         (2 << 4) /* used internally */
#define JSI_PROP_AUTOINIT       (3 << 4) /* used internally */

/* flags for JSI_DefineProperty */
#define JSI_PROP_HAS_SHIFT        8
#define JSI_PROP_HAS_CONFIGURABLE (1 << 8)
#define JSI_PROP_HAS_WRITABLE     (1 << 9)
#define JSI_PROP_HAS_ENUMERABLE   (1 << 10)
#define JSI_PROP_HAS_GET          (1 << 11)
#define JSI_PROP_HAS_SET          (1 << 12)
#define JSI_PROP_HAS_VALUE        (1 << 13)

/* throw an exception if false would be returned
   (JSI_DefineProperty/JSI_SetProperty) */
#define JSI_PROP_THROW            (1 << 14)
/* throw an exception if false would be returned in strict mode
   (JSI_SetProperty) */
#define JSI_PROP_THROW_STRICT     (1 << 15)

#define JSI_PROP_NO_ADD           (1 << 16) /* internal use */
#define JSI_PROP_NO_EXOTIC        (1 << 17) /* internal use */

#ifndef JSI_DEFAULT_STACK_SIZE
#define JSI_DEFAULT_STACK_SIZE (1024 * 1024)
#endif

/* JSI_Eval() flags */
#define JSI_EVAL_TYPE_GLOBAL   (0 << 0) /* global code (default) */
#define JSI_EVAL_TYPE_MODULE   (1 << 0) /* module code */
#define JSI_EVAL_TYPE_DIRECT   (2 << 0) /* direct call (internal use) */
#define JSI_EVAL_TYPE_INDIRECT (3 << 0) /* indirect call (internal use) */
#define JSI_EVAL_TYPE_MASK     (3 << 0)

#define JSI_EVAL_FLAG_STRICT   (1 << 3) /* force 'strict' mode */
/* compile but do not run. The result is an object with a
   JSI_TAG_FUNCTION_BYTECODE or JSI_TAG_MODULE tag. It can be executed
   with JSI_EvalFunction(). */
#define JSI_EVAL_FLAG_COMPILE_ONLY (1 << 5)
/* don't include the stack frames before this eval in the Error() backtraces */
#define JSI_EVAL_FLAG_BACKTRACE_BARRIER (1 << 6)
/* allow top-level await in normal script. JSI_Eval() returns a
   promise. Only allowed with JSI_EVAL_TYPE_GLOBAL */
#define JSI_EVAL_FLAG_ASYNC (1 << 7)

typedef JSValue JSCFunction(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
typedef JSValue JSCFunctionMagic(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic);
typedef JSValue JSCFunctionData(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic, JSValue *func_data);

typedef struct JSMallocState {
    size_t malloc_count;
    size_t malloc_size;
    size_t malloc_limit;
    void *opaque; /* user opaque */
} JSMallocState;

typedef struct JSMallocFunctions {
    void *(*jsi_malloc)(JSMallocState *s, size_t size);
    void (*jsi_free)(JSMallocState *s, void *ptr);
    void *(*jsi_realloc)(JSMallocState *s, void *ptr, size_t size);
    size_t (*jsi_malloc_usable_size)(const void *ptr);
} JSMallocFunctions;

typedef struct JSGCObjectHeader JSGCObjectHeader;

JSRuntime *JSI_NewRuntime(void);
/* info lifetime must exceed that of rt */
void JSI_SetRuntimeInfo(JSRuntime *rt, const char *info);
void JSI_SetMemoryLimit(JSRuntime *rt, size_t limit);
void JSI_SetGCThreshold(JSRuntime *rt, size_t gc_threshold);
/* use 0 to disable maximum stack size check */
void JSI_SetMaxStackSize(JSRuntime *rt, size_t stack_size);
/* should be called when changing thread to update the stack top value
   used to check stack overflow. */
void JSI_UpdateStackTop(JSRuntime *rt);
JSRuntime *JSI_NewRuntime2(const JSMallocFunctions *mf, void *opaque);
void JSI_FreeRuntime(JSRuntime *rt);
void *JSI_GetRuntimeOpaque(JSRuntime *rt);
void JSI_SetRuntimeOpaque(JSRuntime *rt, void *opaque);
typedef void JSI_MarkFunc(JSRuntime *rt, JSGCObjectHeader *gp);
void JSI_MarkValue(JSRuntime *rt, JSValueConst val, JSI_MarkFunc *mark_func);
void JSI_RunGC(JSRuntime *rt);
JSI_BOOL JSI_IsLiveObject(JSRuntime *rt, JSValueConst obj);

JSContext *JSI_NewContext(JSRuntime *rt);
void JSI_FreeContext(JSContext *s);
JSContext *JSI_DupContext(JSContext *ctx);
void *JSI_GetContextOpaque(JSContext *ctx);
void JSI_SetContextOpaque(JSContext *ctx, void *opaque);
JSRuntime *JSI_GetRuntime(JSContext *ctx);
void JSI_SetClassProto(JSContext *ctx, JSClassID class_id, JSValue obj);
JSValue JSI_GetClassProto(JSContext *ctx, JSClassID class_id);

/* the following functions are used to select the intrinsic object to
   save memory */
JSContext *JSI_NewContextRaw(JSRuntime *rt);
void JSI_AddIntrinsicBaseObjects(JSContext *ctx);
void JSI_AddIntrinsicEval(JSContext *ctx);
void JSI_AddIntrinsicStringNormalize(JSContext *ctx);
void JSI_AddIntrinsicPromise(JSContext *ctx);

JSValue jsi_string_codePointRange(JSContext *ctx, JSValueConst this_val,
                                 int argc, JSValueConst *argv);

void *jsi_malloc_rt(JSRuntime *rt, size_t size);
void jsi_free_rt(JSRuntime *rt, void *ptr);
void *jsi_realloc_rt(JSRuntime *rt, void *ptr, size_t size);
size_t jsi_malloc_usable_size_rt(JSRuntime *rt, const void *ptr);
void *jsi_mallocz_rt(JSRuntime *rt, size_t size);

void *jsi_malloc(JSContext *ctx, size_t size);
void jsi_free(JSContext *ctx, void *ptr);
void *jsi_realloc(JSContext *ctx, void *ptr, size_t size);
size_t jsi_malloc_usable_size(JSContext *ctx, const void *ptr);
void *jsi_realloc2(JSContext *ctx, void *ptr, size_t size, size_t *pslack);
void *jsi_mallocz(JSContext *ctx, size_t size);
char *jsi_strdup(JSContext *ctx, const char *str);
char *jsi_strndup(JSContext *ctx, const char *s, size_t n);

typedef struct JSMemoryUsage {
    int64_t malloc_size, malloc_limit, memory_used_size;
    int64_t malloc_count;
    int64_t memory_used_count;
    int64_t atom_count, atom_size;
    int64_t str_count, str_size;
    int64_t obj_count, obj_size;
    int64_t prop_count, prop_size;
    int64_t shape_count, shape_size;
    int64_t jsi_func_count, jsi_func_size, jsi_func_code_size;
    int64_t jsi_func_pc2line_count, jsi_func_pc2line_size;
    int64_t c_func_count, array_count;
    int64_t fast_array_count, fast_array_elements;
    int64_t binary_object_count, binary_object_size;
} JSMemoryUsage;

void JSI_ComputeMemoryUsage(JSRuntime *rt, JSMemoryUsage *s);
void JSI_DumpMemoryUsage(FILE *fp, const JSMemoryUsage *s, JSRuntime *rt);

/* atom support */
#define JSI_ATOM_NULL 0

JSAtom JSI_NewAtomLen(JSContext *ctx, const char *str, size_t len);
JSAtom JSI_NewAtom(JSContext *ctx, const char *str);
JSAtom JSI_NewAtomUInt32(JSContext *ctx, uint32_t n);
JSAtom JSI_DupAtom(JSContext *ctx, JSAtom v);
void JSI_FreeAtom(JSContext *ctx, JSAtom v);
void JSI_FreeAtomRT(JSRuntime *rt, JSAtom v);
JSValue JSI_AtomToValue(JSContext *ctx, JSAtom atom);
JSValue JSI_AtomToString(JSContext *ctx, JSAtom atom);
const char *JSI_AtomToCStringLen(JSContext *ctx, size_t *plen, JSAtom atom);
static inline const char *JSI_AtomToCString(JSContext *ctx, JSAtom atom)
{
    return JSI_AtomToCStringLen(ctx, NULL, atom);
}
JSAtom JSI_ValueToAtom(JSContext *ctx, JSValueConst val);

/* object class support */

typedef struct JSPropertyEnum {
    JSI_BOOL is_enumerable;
    JSAtom atom;
} JSPropertyEnum;

typedef struct JSPropertyDescriptor {
    int flags;
    JSValue value;
    JSValue getter;
    JSValue setter;
} JSPropertyDescriptor;

typedef struct JSClassExoticMethods {
    /* Return -1 if exception (can only happen in case of Proxy object),
       FALSE if the property does not exists, TRUE if it exists. If 1 is
       returned, the property descriptor 'desc' is filled if != NULL. */
    int (*get_own_property)(JSContext *ctx, JSPropertyDescriptor *desc,
                             JSValueConst obj, JSAtom prop);
    /* '*ptab' should hold the '*plen' property keys. Return 0 if OK,
       -1 if exception. The 'is_enumerable' field is ignored.
    */
    int (*get_own_property_names)(JSContext *ctx, JSPropertyEnum **ptab,
                                  uint32_t *plen,
                                  JSValueConst obj);
    /* return < 0 if exception, or TRUE/FALSE */
    int (*delete_property)(JSContext *ctx, JSValueConst obj, JSAtom prop);
    /* return < 0 if exception or TRUE/FALSE */
    int (*define_own_property)(JSContext *ctx, JSValueConst this_obj,
                               JSAtom prop, JSValueConst val,
                               JSValueConst getter, JSValueConst setter,
                               int flags);
    /* The following methods can be emulated with the previous ones,
       so they are usually not needed */
    /* return < 0 if exception or TRUE/FALSE */
    int (*has_property)(JSContext *ctx, JSValueConst obj, JSAtom atom);
    JSValue (*get_property)(JSContext *ctx, JSValueConst obj, JSAtom atom,
                            JSValueConst receiver);
    /* return < 0 if exception or TRUE/FALSE */
    int (*set_property)(JSContext *ctx, JSValueConst obj, JSAtom atom,
                        JSValueConst value, JSValueConst receiver, int flags);

    /* To get a consistent object behavior when get_prototype != NULL,
       get_property, set_property and set_prototype must be != NULL
       and the object must be created with a JSI_NULL prototype. */
    JSValue (*get_prototype)(JSContext *ctx, JSValueConst obj);
    /* return < 0 if exception or TRUE/FALSE */
    int (*set_prototype)(JSContext *ctx, JSValueConst obj, JSValueConst proto_val);
    /* return < 0 if exception or TRUE/FALSE */
    int (*is_extensible)(JSContext *ctx, JSValueConst obj);
    /* return < 0 if exception or TRUE/FALSE */
    int (*prevent_extensions)(JSContext *ctx, JSValueConst obj);
} JSClassExoticMethods;

typedef void JSClassFinalizer(JSRuntime *rt, JSValue val);
typedef void JSClassGCMark(JSRuntime *rt, JSValueConst val,
                           JSI_MarkFunc *mark_func);
#define JSI_CALL_FLAG_CONSTRUCTOR (1 << 0)
typedef JSValue JSClassCall(JSContext *ctx, JSValueConst func_obj,
                            JSValueConst this_val, int argc, JSValueConst *argv,
                            int flags);

typedef struct JSClassDef {
    const char *class_name;
    JSClassFinalizer *finalizer;
    JSClassGCMark *gc_mark;
    /* if call != NULL, the object is a function. If (flags &
       JSI_CALL_FLAG_CONSTRUCTOR) != 0, the function is called as a
       constructor. In this case, 'this_val' is new.target. A
       constructor call only happens if the object constructor bit is
       set (see JSI_SetConstructorBit()). */
    JSClassCall *call;
    /* XXX: suppress this indirection ? It is here only to save memory
       because only a few classes need these methods */
    JSClassExoticMethods *exotic;
} JSClassDef;

#define JSI_INVALID_CLASS_ID 0
JSClassID JSI_NewClassID(JSClassID *pclass_id);
/* Returns the class ID if `v` is an object, otherwise returns JSI_INVALID_CLASS_ID. */
JSClassID JSI_GetClassID(JSValueConst v);
int JSI_NewClass(JSRuntime *rt, JSClassID class_id, const JSClassDef *class_def);
int JSI_IsRegisteredClass(JSRuntime *rt, JSClassID class_id);

/* value handling */

static jsi_force_inline JSValue JSI_NewBool(JSContext *ctx, JSI_BOOL val)
{
    return JSI_MKVAL(JSI_TAG_BOOL, (val != 0));
}

static jsi_force_inline JSValue JSI_NewInt32(JSContext *ctx, int64_t val)
{
    return JSI_MKVAL(JSI_TAG_INT, val);
}

static jsi_force_inline JSValue JSI_NewCatchOffset(JSContext *ctx, int64_t val)
{
    return JSI_MKVAL(JSI_TAG_CATCH_OFFSET, val);
}

static jsi_force_inline JSValue JSI_NewInt64(JSContext *ctx, int64_t val)
{
    return JSI_MKVAL(JSI_TAG_INT, val);

}

static jsi_force_inline JSValue JSI_NewUint32(JSContext *ctx, uint64_t val)
{
    return JSI_MKVALU(JSI_TAG_INT, val);
}



static inline JSI_BOOL JSI_IsNumber(JSValueConst v)
{
    int tag = JSI_VALUE_GET_TAG(v);
    return tag == JSI_TAG_INT ;
}


static inline JSI_BOOL JSI_IsBool(JSValueConst v)
{
    return JSI_VALUE_GET_TAG(v) == JSI_TAG_BOOL;
}

static inline JSI_BOOL JSI_IsNull(JSValueConst v)
{
    return JSI_VALUE_GET_TAG(v) == JSI_TAG_NULL;
}

static inline JSI_BOOL JSI_IsUndefined(JSValueConst v)
{
    return JSI_VALUE_GET_TAG(v) == JSI_TAG_UNDEFINED;
}

static inline JSI_BOOL JSI_IsException(JSValueConst v)
{
    return jsi_unlikely(JSI_VALUE_GET_TAG(v) == JSI_TAG_EXCEPTION);
}

static inline JSI_BOOL JSI_IsUninitialized(JSValueConst v)
{
    return jsi_unlikely(JSI_VALUE_GET_TAG(v) == JSI_TAG_UNINITIALIZED);
}

static inline JSI_BOOL JSI_IsString(JSValueConst v)
{
    return JSI_VALUE_GET_TAG(v) == JSI_TAG_STRING ||
        JSI_VALUE_GET_TAG(v) == JSI_TAG_STRING_ROPE;
}

static inline JSI_BOOL JSI_IsSymbol(JSValueConst v)
{
    return JSI_VALUE_GET_TAG(v) == JSI_TAG_SYMBOL;
}

static inline JSI_BOOL JSI_IsObject(JSValueConst v)
{
    return JSI_VALUE_GET_TAG(v) == JSI_TAG_OBJECT;
}

JSValue JSI_Throw(JSContext *ctx, JSValue obj);
void JSI_SetUncatchableException(JSContext *ctx, JSI_BOOL flag);
JSValue JSI_GetException(JSContext *ctx);
JSI_BOOL JSI_HasException(JSContext *ctx);
JSI_BOOL JSI_IsError(JSContext *ctx, JSValueConst val);
JSValue JSI_NewError(JSContext *ctx);
JSValue __jsi_printf_like(2, 3) JSI_ThrowSyntaxError(JSContext *ctx, const char *fmt, ...);
JSValue __jsi_printf_like(2, 3) JSI_ThrowTypeError(JSContext *ctx, const char *fmt, ...);
JSValue __jsi_printf_like(2, 3) JSI_ThrowReferenceError(JSContext *ctx, const char *fmt, ...);
JSValue __jsi_printf_like(2, 3) JSI_ThrowRangeError(JSContext *ctx, const char *fmt, ...);
JSValue __jsi_printf_like(2, 3) JSI_ThrowInternalError(JSContext *ctx, const char *fmt, ...);
JSValue JSI_ThrowOutOfMemory(JSContext *ctx);

void __JSI_FreeValue(JSContext *ctx, JSValue v);
static inline void JSI_FreeValue(JSContext *ctx, JSValue v)
{
    if (JSI_VALUE_HAS_REF_COUNT(v)) {
        JSRefCountHeader *p = (JSRefCountHeader *)JSI_VALUE_GET_PTR(v);
        if (--p->ref_count <= 0) {
            __JSI_FreeValue(ctx, v);
        }
    }
}
void __JSI_FreeValueRT(JSRuntime *rt, JSValue v);
static inline void JSI_FreeValueRT(JSRuntime *rt, JSValue v)
{
    if (JSI_VALUE_HAS_REF_COUNT(v)) {
        JSRefCountHeader *p = (JSRefCountHeader *)JSI_VALUE_GET_PTR(v);
        if (--p->ref_count <= 0) {
            __JSI_FreeValueRT(rt, v);
        }
    }
}

static inline JSValue JSI_DupValue(JSContext *ctx, JSValueConst v)
{
    if (JSI_VALUE_HAS_REF_COUNT(v)) {
        JSRefCountHeader *p = (JSRefCountHeader *)JSI_VALUE_GET_PTR(v);
        p->ref_count++;
    }
    return (JSValue)v;
}

static inline JSValue JSI_DupValueRT(JSRuntime *rt, JSValueConst v)
{
    if (JSI_VALUE_HAS_REF_COUNT(v)) {
        JSRefCountHeader *p = (JSRefCountHeader *)JSI_VALUE_GET_PTR(v);
        p->ref_count++;
    }
    return (JSValue)v;
}

JSI_BOOL JSI_StrictEq(JSContext *ctx, JSValueConst op1, JSValueConst op2);
JSI_BOOL JSI_SameValue(JSContext *ctx, JSValueConst op1, JSValueConst op2);
JSI_BOOL JSI_SameValueZero(JSContext *ctx, JSValueConst op1, JSValueConst op2);

int JSI_ToBool(JSContext *ctx, JSValueConst val); /* return -1 for JSI_EXCEPTION */
int JSI_ToInt32(JSContext *ctx, int32_t *pres, JSValueConst val);
static inline int JSI_ToUint32(JSContext *ctx, uint32_t *pres, JSValueConst val)
{
    return JSI_ToInt32(ctx, (int32_t*)pres, val);
}
int JSI_ToInt64(JSContext *ctx, int64_t *pres, JSValueConst val);
int JSI_ToIndex(JSContext *ctx, uint64_t *plen, JSValueConst val);
/* return an exception if 'val' is a Number */
/* same as JSI_ToInt64() but allow BigInt */
int JSI_ToInt64Ext(JSContext *ctx, int64_t *pres, JSValueConst val);

JSValue JSI_NewStringLen(JSContext *ctx, const char *str1, size_t len1);
static inline JSValue JSI_NewString(JSContext *ctx, const char *str)
{
    return JSI_NewStringLen(ctx, str, strlen(str));
}
JSValue JSI_NewAtomString(JSContext *ctx, const char *str);
JSValue JSI_ToString(JSContext *ctx, JSValueConst val);
JSValue JSI_ToPropertyKey(JSContext *ctx, JSValueConst val);
const char *JSI_ToCStringLen2(JSContext *ctx, size_t *plen, JSValueConst val1, JSI_BOOL cesu8);
static inline const char *JSI_ToCStringLen(JSContext *ctx, size_t *plen, JSValueConst val1)
{
    return JSI_ToCStringLen2(ctx, plen, val1, 0);
}
static inline const char *JSI_ToCString(JSContext *ctx, JSValueConst val1)
{
    return JSI_ToCStringLen2(ctx, NULL, val1, 0);
}
void JSI_FreeCString(JSContext *ctx, const char *ptr);

JSValue JSI_NewObjectProtoClass(JSContext *ctx, JSValueConst proto, JSClassID class_id);
JSValue JSI_NewObjectClass(JSContext *ctx, int class_id);
JSValue JSI_NewObjectProto(JSContext *ctx, JSValueConst proto);
JSValue JSI_NewObject(JSContext *ctx);

JSI_BOOL JSI_IsFunction(JSContext* ctx, JSValueConst val);
JSI_BOOL JSI_IsConstructor(JSContext* ctx, JSValueConst val);
JSI_BOOL JSI_SetConstructorBit(JSContext *ctx, JSValueConst func_obj, JSI_BOOL val);

JSValue JSI_NewArray(JSContext *ctx);
int JSI_IsArray(JSContext *ctx, JSValueConst val);
JSValue JSI_GetPropertyInternal(JSContext *ctx, JSValueConst obj,
                               JSAtom prop, JSValueConst this_obj,
                               bool throw_ref_error);

static jsi_force_inline JSValue JSI_GetProperty(JSContext *ctx, JSValueConst this_obj,
                                              JSAtom prop)
{
    return JSI_GetPropertyInternal(ctx, this_obj, prop, this_obj, 0);
}
JSValue JSI_GetPropertyStr(JSContext *ctx, JSValueConst this_obj,
                          const char *prop);
JSValue JSI_GetPropertyUint32(JSContext *ctx, JSValueConst this_obj,
                             uint32_t idx);

int JSI_SetPropertyInternal(JSContext *ctx, JSValueConst obj,
                           JSAtom prop, JSValue val, JSValueConst this_obj,
                           int flags);
static inline int JSI_SetProperty(JSContext *ctx, JSValueConst this_obj,
                                 JSAtom prop, JSValue val)
{
    return JSI_SetPropertyInternal(ctx, this_obj, prop, val, this_obj, JSI_PROP_THROW);
}
int JSI_SetPropertyUint32(JSContext *ctx, JSValueConst this_obj,
                         uint32_t idx, JSValue val);
int JSI_SetPropertyInt64(JSContext *ctx, JSValueConst this_obj,
                        int64_t idx, JSValue val);
int JSI_SetPropertyStr(JSContext *ctx, JSValueConst this_obj,
                      const char *prop, JSValue val);
int JSI_HasProperty(JSContext *ctx, JSValueConst this_obj, JSAtom prop);
int JSI_IsExtensible(JSContext *ctx, JSValueConst obj);
int JSI_PreventExtensions(JSContext *ctx, JSValueConst obj);
int JSI_DeleteProperty(JSContext *ctx, JSValueConst obj, JSAtom prop, int flags);
int JSI_SetPrototype(JSContext *ctx, JSValueConst obj, JSValueConst proto_val);
JSValue JSI_GetPrototype(JSContext *ctx, JSValueConst val);

#define JSI_GPN_STRING_MASK  (1 << 0)
#define JSI_GPN_SYMBOL_MASK  (1 << 1)
#define JSI_GPN_PRIVATE_MASK (1 << 2)
/* only include the enumerable properties */
#define JSI_GPN_ENUM_ONLY    (1 << 4)
/* set theJSPropertyEnum.is_enumerable field */
#define JSI_GPN_SET_ENUM     (1 << 5)

int JSI_GetOwnPropertyNames(JSContext *ctx, JSPropertyEnum **ptab,
                           uint32_t *plen, JSValueConst obj, int flags);
void JSI_FreePropertyEnum(JSContext *ctx, JSPropertyEnum *tab,
                         uint32_t len);
int JSI_GetOwnProperty(JSContext *ctx, JSPropertyDescriptor *desc,
                      JSValueConst obj, JSAtom prop);

JSValue JSI_Call(JSContext *ctx, JSValueConst func_obj, JSValueConst this_obj,
                int argc, JSValueConst *argv);
JSValue JSI_Invoke(JSContext *ctx, JSValueConst this_val, JSAtom atom,
                  int argc, JSValueConst *argv);
JSValue JSI_CallConstructor(JSContext *ctx, JSValueConst func_obj,
                           int argc, JSValueConst *argv);
JSValue JSI_CallConstructor2(JSContext *ctx, JSValueConst func_obj,
                            JSValueConst new_target,
                            int argc, JSValueConst *argv);
JSI_BOOL JSI_DetectModule(const char *input, size_t input_len);
/* 'input' must be zero terminated i.e. input[input_len] = '\0'. */
JSValue JSI_Eval(JSContext *ctx, const char *input, size_t input_len,
                const char *filename, int eval_flags);
/* same as JSI_Eval() but with an explicit 'this_obj' parameter */
JSValue JSI_EvalThis(JSContext *ctx, JSValueConst this_obj,
                    const char *input, size_t input_len,
                    const char *filename, int eval_flags);
JSValue JSI_GetGlobalObject(JSContext *ctx);
int JSI_IsInstanceOf(JSContext *ctx, JSValueConst val, JSValueConst obj);
int JSI_DefineProperty(JSContext *ctx, JSValueConst this_obj,
                      JSAtom prop, JSValueConst val,
                      JSValueConst getter, JSValueConst setter, int flags);
int JSI_DefinePropertyValue(JSContext *ctx, JSValueConst this_obj,
                           JSAtom prop, JSValue val, int flags);
int JSI_DefinePropertyValueUint32(JSContext *ctx, JSValueConst this_obj,
                                 uint32_t idx, JSValue val, int flags);
int JSI_DefinePropertyValueStr(JSContext *ctx, JSValueConst this_obj,
                              const char *prop, JSValue val, int flags);
int JSI_DefinePropertyGetSet(JSContext *ctx, JSValueConst this_obj,
                            JSAtom prop, JSValue getter, JSValue setter,
                            int flags);
void JSI_SetOpaque(JSValue obj, void *opaque);
void *JSI_GetOpaque(JSValueConst obj, JSClassID class_id);
void *JSI_GetOpaque2(JSContext *ctx, JSValueConst obj, JSClassID class_id);
void *JSI_GetAnyOpaque(JSValueConst obj, JSClassID *class_id);

/* 'buf' must be zero terminated i.e. buf[buf_len] = '\0'. */
JSValue JSI_ParseJSON(JSContext *ctx, const char *buf, size_t buf_len,
                     const char *filename);
#define JSI_PARSE_JSON_EXT (1 << 0) /* allow extended JSON */
JSValue JSI_ParseJSON2(JSContext *ctx, const char *buf, size_t buf_len,
                      const char *filename, int flags);
JSValue JSI_JSONStringify(JSContext *ctx, JSValueConst obj,
                         JSValueConst replacer, JSValueConst space0);

typedef void JSFreeArrayBufferDataFunc(JSRuntime *rt, void *opaque, void *ptr);
JSValue JSI_NewArrayBuffer(JSContext *ctx, uint8_t *buf, size_t len,
                          JSFreeArrayBufferDataFunc *free_func, void *opaque,
                          JSI_BOOL is_shared);
JSValue JSI_NewArrayBufferCopy(JSContext *ctx, const uint8_t *buf, size_t len);
void JSI_DetachArrayBuffer(JSContext *ctx, JSValueConst obj);
uint8_t *JSI_GetArrayBuffer(JSContext *ctx, size_t *psize, JSValueConst obj);

typedef enum JSTypedArrayEnum {
    JSI_TYPED_ARRAY_UINT8C = 0,
    JSI_TYPED_ARRAY_INT8,
    JSI_TYPED_ARRAY_UINT8,
    JSI_TYPED_ARRAY_INT16,
    JSI_TYPED_ARRAY_UINT16,
    JSI_TYPED_ARRAY_INT32,
    JSI_TYPED_ARRAY_UINT32,
    JSI_TYPED_ARRAY_BIG_INT64,
    JSI_TYPED_ARRAY_BIG_UINT64,
    JSI_TYPED_ARRAY_FLOAT16,
    JSI_TYPED_ARRAY_FLOAT32,
    JSI_TYPED_ARRAY_FLOAT64,
} JSTypedArrayEnum;

JSValue JSI_NewTypedArray(JSContext *ctx, int argc, JSValueConst *argv,
                         JSTypedArrayEnum array_type);
JSValue JSI_GetTypedArrayBuffer(JSContext *ctx, JSValueConst obj,
                               size_t *pbyte_offset,
                               size_t *pbyte_length,
                               size_t *pbytes_per_element);
typedef struct {
    void *(*sab_alloc)(void *opaque, size_t size);
    void (*sab_free)(void *opaque, void *ptr);
    void (*sab_dup)(void *opaque, void *ptr);
    void *sab_opaque;
} JSSharedArrayBufferFunctions;
void JSI_SetSharedArrayBufferFunctions(JSRuntime *rt,
                                      const JSSharedArrayBufferFunctions *sf);

typedef enum JSPromiseStateEnum {
    JSI_PROMISE_PENDING,
    JSI_PROMISE_FULFILLED,
    JSI_PROMISE_REJECTED,
} JSPromiseStateEnum;


/* is_handled = TRUE means that the rejection is handled */
typedef void JSHostPromiseRejectionTracker(JSContext *ctx, JSValueConst promise,
                                           JSValueConst reason,
                                           JSI_BOOL is_handled, void *opaque);

/* return != 0 if the JS code needs to be interrupted */
typedef int JSInterruptHandler(JSRuntime *rt, void *opaque);
void JSI_SetInterruptHandler(JSRuntime *rt, JSInterruptHandler *cb, void *opaque);
/* if can_block is TRUE, Atomics.wait() can be used */
void JSI_SetCanBlock(JSRuntime *rt, JSI_BOOL can_block);
/* select which debug info is stripped from the compiled code */
#define JSI_STRIP_SOURCE (1 << 0) /* strip source code */
#define JSI_STRIP_DEBUG  (1 << 1) /* strip all debug info including source code */
void JSI_SetStripInfo(JSRuntime *rt, int flags);
int JSI_GetStripInfo(JSRuntime *rt);

/* set the [IsHTMLDDA] internal slot */
void JSI_SetIsHTMLDDA(JSContext *ctx, JSValueConst obj);

typedef struct JSModuleDef JSModuleDef;

/* return the module specifier (allocated with jsi_malloc()) or NULL if
   exception */
typedef char *JSModuleNormalizeFunc(JSContext *ctx,
                                    const char *module_base_name,
                                    const char *module_name, void *opaque);
typedef JSModuleDef *JSModuleLoaderFunc(JSContext *ctx,
                                        const char *module_name, void *opaque);
typedef JSModuleDef *JSModuleLoaderFunc2(JSContext *ctx,
                                         const char *module_name, void *opaque,
                                         JSValueConst attributes);
/* return -1 if exception, 0 if OK */
typedef int JSModuleCheckSupportedImportAttributes(JSContext *ctx, void *opaque,
                                                   JSValueConst attributes);
                                                   
/* module_normalize = NULL is allowed and invokes the default module
   filename normalizer */
void JSI_SetModuleLoaderFunc(JSRuntime *rt,
                            JSModuleNormalizeFunc *module_normalize,
                            JSModuleLoaderFunc *module_loader, void *opaque);
/* same as JSI_SetModuleLoaderFunc but with attributes. if
   module_check_attrs = NULL, no attribute checking is done. */
void JSI_SetModuleLoaderFunc2(JSRuntime *rt,
                             JSModuleNormalizeFunc *module_normalize,
                             JSModuleLoaderFunc2 *module_loader,
                             JSModuleCheckSupportedImportAttributes *module_check_attrs,
                             void *opaque);
/* return the import.meta object of a module */
JSValue JSI_GetImportMeta(JSContext *ctx, JSModuleDef *m);
JSAtom JSI_GetModuleName(JSContext *ctx, JSModuleDef *m);
JSValue JSI_GetModuleNamespace(JSContext *ctx, JSModuleDef *m);

/* JS Job support */

typedef JSValue JSJobFunc(JSContext *ctx, int argc, JSValueConst *argv);
int JSI_EnqueueJob(JSContext *ctx, JSJobFunc *job_func, int argc, JSValueConst *argv);

JSI_BOOL JSI_IsJobPending(JSRuntime *rt);
int JSI_ExecutePendingJob(JSRuntime *rt, JSContext **pctx);

/* Object Writer/Reader (currently only used to handle precompiled code) */
#define JSI_WRITE_OBJ_BYTECODE  (1 << 0) /* allow function/module */
#define JSI_WRITE_OBJ_BSWAP     (1 << 1) /* byte swapped output */
#define JSI_WRITE_OBJ_SAB       (1 << 2) /* allow SharedArrayBuffer */
#define JSI_WRITE_OBJ_REFERENCE (1 << 3) /* allow object references to
                                           encode arbitrary object
                                           graph */
uint8_t *JSI_WriteObject(JSContext *ctx, size_t *psize, JSValueConst obj,
                        int flags);
uint8_t *JSI_WriteObject2(JSContext *ctx, size_t *psize, JSValueConst obj,
                         int flags, uint8_t ***psab_tab, size_t *psab_tab_len);

#define JSI_READ_OBJ_BYTECODE  (1 << 0) /* allow function/module */
#define JSI_READ_OBJ_ROM_DATA  (1 << 1) /* avoid duplicating 'buf' data */
#define JSI_READ_OBJ_SAB       (1 << 2) /* allow SharedArrayBuffer */
#define JSI_READ_OBJ_REFERENCE (1 << 3) /* allow object references */
JSValue JSI_ReadObject(JSContext *ctx, const uint8_t *buf, size_t buf_len,
                      int flags);
/* instantiate and evaluate a bytecode function. Only used when
   reading a script or module with JSI_ReadObject() */
JSValue JSI_EvalFunction(JSContext *ctx, JSValue fun_obj);
/* load the dependencies of the module 'obj'. Useful when JSI_ReadObject()
   returns a module. */
int JSI_ResolveModule(JSContext *ctx, JSValueConst obj);

/* only exported for os.Worker() */
JSAtom JSI_GetScriptOrModuleName(JSContext *ctx, int n_stack_levels);
/* only exported for os.Worker() */
JSValue JSI_LoadModule(JSContext *ctx, const char *basename,
                      const char *filename);

/* C function definition */
typedef enum JSCFunctionEnum {  /* XXX: should rename for namespace isolation */
    JSI_CFUNC_generic,
    JSI_CFUNC_generic_magic,
    JSI_CFUNC_constructor,
    JSI_CFUNC_constructor_magic,
    JSI_CFUNC_constructor_or_func,
    JSI_CFUNC_constructor_or_func_magic,
    JSI_CFUNC_f_f,
    JSI_CFUNC_f_f_f,
    JSI_CFUNC_getter,
    JSI_CFUNC_setter,
    JSI_CFUNC_getter_magic,
    JSI_CFUNC_setter_magic,
    JSI_CFUNC_iterator_next,
} JSCFunctionEnum;

typedef union JSCFunctionType {
    JSCFunction *generic;
    JSValue (*generic_magic)(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic);
    JSCFunction *constructor;
    JSValue (*constructor_magic)(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv, int magic);
    JSCFunction *constructor_or_func;
    int64_t (*f_f)(int64_t);
    int64_t (*f_f_f)(int64_t, int64_t);
    JSValue (*getter)(JSContext *ctx, JSValueConst this_val);
    JSValue (*setter)(JSContext *ctx, JSValueConst this_val, JSValueConst val);
    JSValue (*getter_magic)(JSContext *ctx, JSValueConst this_val, int magic);
    JSValue (*setter_magic)(JSContext *ctx, JSValueConst this_val, JSValueConst val, int magic);
    JSValue (*iterator_next)(JSContext *ctx, JSValueConst this_val,
                             int argc, JSValueConst *argv, int *pdone, int magic);
} JSCFunctionType;

JSValue JSI_NewCFunction2(JSContext *ctx, JSCFunction *func,
                         const char *name,
                         int length, JSCFunctionEnum cproto, int magic);
JSValue JSI_NewCFunctionData(JSContext *ctx, JSCFunctionData *func,
                            int length, int magic, int data_len,
                            JSValueConst *data);

static inline JSValue JSI_NewCFunction(JSContext *ctx, JSCFunction *func, const char *name,
                                      int length)
{
    return JSI_NewCFunction2(ctx, func, name, length, JSI_CFUNC_generic, 0);
}

static inline JSValue JSI_NewCFunctionMagic(JSContext *ctx, JSCFunctionMagic *func,
                                           const char *name,
                                           int length, JSCFunctionEnum cproto, int magic)
{
    return JSI_NewCFunction2(ctx, (JSCFunction *)func, name, length, cproto, magic);
}
void JSI_SetConstructor(JSContext *ctx, JSValueConst func_obj,
                       JSValueConst proto);

/* C property definition */

typedef struct JSCFunctionListEntry {
    const char *name;
    uint8_t prop_flags;
    uint8_t def_type;
    int16_t magic;
    union {
        struct {
            uint8_t length; /* XXX: should move outside union */
            uint8_t cproto; /* XXX: should move outside union */
            JSCFunctionType cfunc;
        } func;
        struct {
            JSCFunctionType get;
            JSCFunctionType set;
        } getset;
        struct {
            const char *name;
            int base;
        } alias;
        struct {
            const struct JSCFunctionListEntry *tab;
            int len;
        } prop_list;
        const char *str;
        int32_t i32;
        int64_t i64;
        double f64;
    } u;
} JSCFunctionListEntry;

#define JSI_DEF_CFUNC          0
#define JSI_DEF_CGETSET        1
#define JSI_DEF_CGETSET_MAGIC  2
#define JSI_DEF_PROP_STRING    3
#define JSI_DEF_PROP_INT32     4
#define JSI_DEF_PROP_INT64     5
// #define JSI_DEF_PROP_DOUBLE    6
#define JSI_DEF_PROP_UNDEFINED 7
#define JSI_DEF_OBJECT         8
#define JSI_DEF_ALIAS          9

/* Note: c++ does not like nested designators */
#define JSI_CFUNC_DEF(name, length, func1) { name, JSI_PROP_WRITABLE | JSI_PROP_CONFIGURABLE, JSI_DEF_CFUNC, 0, .u = { .func = { length, JSI_CFUNC_generic, { .generic = func1 } } } }
#define JSI_CFUNC_MAGIC_DEF(name, length, func1, magic) { name, JSI_PROP_WRITABLE | JSI_PROP_CONFIGURABLE, JSI_DEF_CFUNC, magic, .u = { .func = { length, JSI_CFUNC_generic_magic, { .generic_magic = func1 } } } }
#define JSI_CFUNC_SPECIAL_DEF(name, length, cproto, func1) { name, JSI_PROP_WRITABLE | JSI_PROP_CONFIGURABLE, JSI_DEF_CFUNC, 0, .u = { .func = { length, JSI_CFUNC_ ## cproto, { .cproto = func1 } } } }
#define JSI_ITERATOR_NEXT_DEF(name, length, func1, magic) { name, JSI_PROP_WRITABLE | JSI_PROP_CONFIGURABLE, JSI_DEF_CFUNC, magic, .u = { .func = { length, JSI_CFUNC_iterator_next, { .iterator_next = func1 } } } }
#define JSI_CGETSET_DEF(name, fgetter, fsetter) { name, JSI_PROP_CONFIGURABLE, JSI_DEF_CGETSET, 0, .u = { .getset = { .get = { .getter = fgetter }, .set = { .setter = fsetter } } } }
#define JSI_CGETSET_MAGIC_DEF(name, fgetter, fsetter, magic) { name, JSI_PROP_CONFIGURABLE, JSI_DEF_CGETSET_MAGIC, magic, .u = { .getset = { .get = { .getter_magic = fgetter }, .set = { .setter_magic = fsetter } } } }
#define JSI_PROP_STRING_DEF(name, cstr, prop_flags) { name, prop_flags, JSI_DEF_PROP_STRING, 0, .u = { .str = cstr } }
#define JSI_PROP_INT32_DEF(name, val, prop_flags) { name, prop_flags, JSI_DEF_PROP_INT32, 0, .u = { .i32 = val } }
#define JSI_PROP_INT64_DEF(name, val, prop_flags) { name, prop_flags, JSI_DEF_PROP_INT64, 0, .u = { .i64 = val } }
// #define JSI_PROP_DOUBLE_DEF(name, val, prop_flags) { name, prop_flags, JSI_DEF_PROP_DOUBLE, 0, .u = { .f64 = val } }
#define JSI_PROP_UNDEFINED_DEF(name, prop_flags) { name, prop_flags, JSI_DEF_PROP_UNDEFINED, 0, .u = { .i32 = 0 } }
#define JSI_OBJECT_DEF(name, tab, len, prop_flags) { name, prop_flags, JSI_DEF_OBJECT, 0, .u = { .prop_list = { tab, len } } }
#define JSI_ALIAS_DEF(name, from) { name, JSI_PROP_WRITABLE | JSI_PROP_CONFIGURABLE, JSI_DEF_ALIAS, 0, .u = { .alias = { from, -1 } } }
#define JSI_ALIAS_BASE_DEF(name, from, base) { name, JSI_PROP_WRITABLE | JSI_PROP_CONFIGURABLE, JSI_DEF_ALIAS, 0, .u = { .alias = { from, base } } }

int JSI_SetPropertyFunctionList(JSContext *ctx, JSValueConst obj,
                               const JSCFunctionListEntry *tab,
                               int len);

/* C module definition */

typedef int JSModuleInitFunc(JSContext *ctx, JSModuleDef *m);

JSModuleDef *JSI_NewCModule(JSContext *ctx, const char *name_str,
                           JSModuleInitFunc *func);
/* can only be called before the module is instantiated */
int JSI_AddModuleExport(JSContext *ctx, JSModuleDef *m, const char *name_str);
int JSI_AddModuleExportList(JSContext *ctx, JSModuleDef *m,
                           const JSCFunctionListEntry *tab, int len);
/* can only be called after the module is instantiated */
int JSI_SetModuleExport(JSContext *ctx, JSModuleDef *m, const char *export_name,
                       JSValue val);
int JSI_SetModuleExportList(JSContext *ctx, JSModuleDef *m,
                           const JSCFunctionListEntry *tab, int len);
/* associate a JSValue to a C module */
int JSI_SetModulePrivateValue(JSContext *ctx, JSModuleDef *m, JSValue val);
JSValue JSI_GetModulePrivateValue(JSContext *ctx, JSModuleDef *m);
                        
/* debug value output */

typedef struct {
    JSI_BOOL show_hidden : 8; /* only show enumerable properties */
    JSI_BOOL raw_dump : 8; /* avoid doing autoinit and avoid any malloc() call (for internal use) */
    uint32_t max_depth; /* recurse up to this depth, 0 = no limit */
    uint32_t max_string_length; /* print no more than this length for
                                   strings, 0 = no limit */
    uint32_t max_item_count; /*  print no more than this count for
                                 arrays or objects, 0 = no limit */
} JSPrintValueOptions;

typedef void JSPrintValueWrite(void *opaque, const char *buf, size_t len);

void JSI_PrintValueSetDefaultOptions(JSPrintValueOptions *options);
void JSI_PrintValueRT(JSRuntime *rt, JSPrintValueWrite *write_func, void *write_opaque,
                     JSValueConst val, const JSPrintValueOptions *options);
void JSI_PrintValue(JSContext *ctx, JSPrintValueWrite *write_func, void *write_opaque,
                   JSValueConst val, const JSPrintValueOptions *options);

#undef jsi_unlikely
#undef jsi_force_inline

#ifdef __cplusplus
} /* extern "C" { */
#endif

#endif /* QUICKJSI_H */
