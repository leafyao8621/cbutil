#include <stdio.h>

#include "builder/builder.h"

int main(void) {
    CBBuilder builder;
    CBBuilderError error = CBBuilder_initialize(&builder);
    if (error) {
        fprintf(
            stderr,
            "Error initializing builder (%d): %s\n",
            error,
            CBBuilderErrorLookup[error]
        );
        return 1;
    }
    error = CBBuilder_finalize(&builder);
    if (error) {
        fprintf(
            stderr,
            "Error finalizing builder (%d): %s\n",
            error,
            CBBuilderErrorLookup[error]
        );
        return 1;
    }
    return 0;
}
