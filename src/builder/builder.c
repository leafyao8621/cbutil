#include "builder.h"

const char *CBBuilderErrorLookup[] =
    {
        "No error",
        "Null pointer error",
        "Memory allocation error",
        "I/O error",
    };

CBBuilderError CBBuilder_initialize(CBBuilder *builder) {
    if (!builder) {
        return CB_BUILDER_ERROR_NULLPTR;
    }
    if (DArrayChar_initialize(&builder->buf_fn, 1024)) {
        return CB_BUILDER_ERROR_ALLOC;
    }
    if (DArrayChar_initialize(&builder->buf_an, 1024)) {
        return CB_BUILDER_ERROR_ALLOC;
    }
    if (DArrayChar_initialize(&builder->buf_td, 1024)) {
        return CB_BUILDER_ERROR_ALLOC;
    }
    if (DArrayChar_initialize(&builder->buf_smp, 1024)) {
        return CB_BUILDER_ERROR_ALLOC;
    }
    if (DArrayOffset_initialize(&builder->offset_an, 1024)) {
        return CB_BUILDER_ERROR_ALLOC;
    }
    return CB_BUILDER_ERROR_NONE;
}

CBBuilderError CBBuilder_finalize(CBBuilder *builder) {
    if (!builder) {
        return CB_BUILDER_ERROR_NULLPTR;
    }
    DArrayChar_finalize(&builder->buf_fn);
    DArrayChar_finalize(&builder->buf_an);
    DArrayChar_finalize(&builder->buf_td);
    DArrayChar_finalize(&builder->buf_smp);
    DArrayOffset_finalize(&builder->offset_an);
    return CB_BUILDER_ERROR_NONE;
}
