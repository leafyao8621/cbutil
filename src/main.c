#include "builder/builder.h"

#include <stdio.h>

#include <unistd.h>
#include <fcntl.h>

int main(void) {
    CBBuilder builder;
    CBBuilderError error = CBBuilder_initialize(&builder, true, 48000, 24);
    if (error) {
        fprintf(
            stderr,
            "Error initializing builder (%d): %s\n",
            error,
            CBBuilderErrorLookup[error]
        );
        return 1;
    }
    error = CBBuilder_build(&builder, "/mnt/vault/Audio/wave/");
    if (error) {
        fprintf(
            stderr,
            "Error building (%d): %s\n",
            error,
            CBBuilderErrorLookup[error]
        );
        return 1;
    }
    CBBuilder_log(&builder, stderr);
    int fd = open("dump.img", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        fprintf(
            stderr,
            "Error opening output file: %s\n",
            "dump.img"
        );
        return 1;
    }
    error = CBBuilder_write(&builder, fd);
    if (error) {
        close(fd);
        fprintf(
            stderr,
            "Error writing builder (%d): %s\n",
            error,
            CBBuilderErrorLookup[error]
        );
        return 1;
    }
    close(fd);
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
