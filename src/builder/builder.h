#ifndef BUILDER_BUILDER_H
#define BUILDER_BUILDER_H

#include "../util/container.h"

typedef struct CBBuilder {
    DArrayChar buf_fn, buf_an, buf_td, buf_smp;
    DArrayOffset offset_an;
} CBBuilder;

typedef enum CBBuilderError {
    CB_BUILDER_ERROR_NONE = 0,
    CB_BUILDER_ERROR_NULLPTR,
    CB_BUILDER_ERROR_ALLOC,
    CB_BUILDER_ERROR_IO,
} CBBuilderError;

extern const char *CBBuilderErrorLookup[];

CBBuilderError CBBuilder_initialize(CBBuilder *builder);
CBBuilderError CBBuilder_finalize(CBBuilder *builder);
CBBuilderError CBBuilder_build(CBBuilder *builder, const char *dir, int fd);

#endif // BUILDER_BUILDER_H
