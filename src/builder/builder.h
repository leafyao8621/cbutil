#ifndef BUILDER_BUILDER_H
#define BUILDER_BUILDER_H

#include <stdio.h>

#include "../util/container.h"

typedef struct CBBuilder CBBuilder;

typedef struct CBBuilderTrack {
    CBBuilder *builder;
    Offset offset_fn, offset_iprd, offset_fmt, offset_inam, offset_itrk;
} CBBuilderTrack;

DEF_DARRAY(CBBuilderTrack)

struct CBBuilder {
    bool verbose;
    uint64_t sampling_rate_cap, bit_depth_cap, cnt_na;
    DArrayChar buf_fn, buf_iprd, buf_fmt, buf_inam, buf_itrk;
    DArrayOffset offset_iprd, stack_path, cnt_iprd, ptr_iprd;
    DArrayCBBuilderTrack track;
};

typedef enum CBBuilderError {
    CB_BUILDER_ERROR_NONE = 0,
    CB_BUILDER_ERROR_NULLPTR,
    CB_BUILDER_ERROR_ALLOC,
    CB_BUILDER_ERROR_IO
} CBBuilderError;

extern const char *CBBuilderErrorLookup[];

CBBuilderError CBBuilder_initialize(
    CBBuilder *builder,
    bool verbose,
    uint64_t sampling_rate_cap,
    uint64_t bit_depth_cap);
CBBuilderError CBBuilder_finalize(CBBuilder *builder);
CBBuilderError CBBuilder_build(CBBuilder *builder, char *dir);
CBBuilderError CBBuilder_write(CBBuilder *builder, int fd);
void CBBuilder_log(CBBuilder *builder, FILE *stream);

#endif // BUILDER_BUILDER_H
