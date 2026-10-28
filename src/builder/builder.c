#include "builder.h"

const char *CBBuilderErrorLookup[] =
    {
        "No error",
        "Null pointer error",
        "Memory allocation error",
        "I/O error",
    };

CBBuilderError CBBuilder_initialize(
    CBBuilder *builder,
    bool verbose,
    uint64_t sampling_rate_cap,
    uint64_t bit_depth_cap) {
    if (!builder) {
        return CB_BUILDER_ERROR_NULLPTR;
    }
    builder->verbose = verbose;
    builder->sampling_rate_cap = sampling_rate_cap;
    builder->bit_depth_cap = bit_depth_cap;
    builder->cnt_na = 0;
    if (DArrayChar_initialize(&builder->buf_fn, 1024)) {
        return CB_BUILDER_ERROR_ALLOC;
    }
    if (DArrayChar_initialize(&builder->buf_iprd, 1024)) {
        return CB_BUILDER_ERROR_ALLOC;
    }
    if (DArrayChar_initialize(&builder->buf_fmt, 1024)) {
        return CB_BUILDER_ERROR_ALLOC;
    }
    if (DArrayChar_initialize(&builder->buf_inam, 1024)) {
        return CB_BUILDER_ERROR_ALLOC;
    }
    if (DArrayChar_initialize(&builder->buf_itrk, 1024)) {
        return CB_BUILDER_ERROR_ALLOC;
    }
    if (DArrayOffset_initialize(&builder->offset_iprd, 1024)) {
        return CB_BUILDER_ERROR_ALLOC;
    }
    if (DArrayOffset_initialize(&builder->stack_path, 1024)) {
        return CB_BUILDER_ERROR_ALLOC;
    }
    if (DArrayOffset_initialize(&builder->cnt_iprd, 1024)) {
        return CB_BUILDER_ERROR_ALLOC;
    }
    if (DArrayOffset_initialize(&builder->ptr_iprd, 1024)) {
        return CB_BUILDER_ERROR_ALLOC;
    }
    if (DArrayCBBuilderTrack_initialize(&builder->track, 1024)) {
        return CB_BUILDER_ERROR_ALLOC;
    }
    return CB_BUILDER_ERROR_NONE;
}

CBBuilderError CBBuilder_finalize(CBBuilder *builder) {
    if (!builder) {
        return CB_BUILDER_ERROR_NULLPTR;
    }
    DArrayChar_finalize(&builder->buf_fn);
    DArrayChar_finalize(&builder->buf_iprd);
    DArrayChar_finalize(&builder->buf_fmt);
    DArrayChar_finalize(&builder->buf_inam);
    DArrayChar_finalize(&builder->buf_itrk);
    DArrayOffset_finalize(&builder->offset_iprd);
    DArrayOffset_finalize(&builder->stack_path);
    DArrayOffset_finalize(&builder->cnt_iprd);
    DArrayOffset_finalize(&builder->ptr_iprd);
    DArrayCBBuilderTrack_finalize(&builder->track);
    return CB_BUILDER_ERROR_NONE;
}
