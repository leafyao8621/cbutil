#include "builder.h"

#include <stdio.h>

#include <unistd.h>

CBBuilderError CBBuilder_write(CBBuilder *builder, int fd) {
    if (!builder) {
        return CB_BUILDER_ERROR_NULLPTR;
    }
    if (fd < 0) {
        return CB_BUILDER_ERROR_IO;
    }
    static const char zero_pad[512] = {0};
    ssize_t ret = write(fd, "CYANBOOK", 8);
    if (ret != 8) {
        return CB_BUILDER_ERROR_IO;
    }
    ret = write(fd, &builder->cnt_iprd.size, 8);
    if (ret != 8) {
        return CB_BUILDER_ERROR_IO;
    }
    ret = write(fd, &builder->track.size, 8);
    if (ret != 8) {
        return CB_BUILDER_ERROR_IO;
    }
    ret = write(fd, zero_pad, 488);
    if (ret != 488) {
        return CB_BUILDER_ERROR_IO;
    }
    Offset *cnt_iprd_it = builder->cnt_iprd.data;
    Offset *offset_iprd_it = builder->offset_iprd.data;
    Offset *ptr_iprd_it = builder->ptr_iprd.data;
    for (
        size_t i = 0;
        i < builder->cnt_iprd.size;
        ++i, ++cnt_iprd_it, ++offset_iprd_it, ++ptr_iprd_it) {
        ret = write(fd, cnt_iprd_it, 8);
        if (ret != 8) {
            return CB_BUILDER_ERROR_IO;
        }
        ret = write(fd, ptr_iprd_it, 8);
        if (ret != 8) {
            return CB_BUILDER_ERROR_IO;
        }
        size_t iprd_len =
            strlen(builder->buf_iprd.data + *offset_iprd_it) + 1;
        if (iprd_len <= 496) {
            ret = write(fd, builder->buf_iprd.data + *offset_iprd_it, iprd_len);
            if (ret != (ssize_t)iprd_len) {
                return CB_BUILDER_ERROR_IO;
            }
            ret = write(fd, zero_pad, 496 - iprd_len);
            if (ret != (ssize_t)(496 - iprd_len)) {
                return CB_BUILDER_ERROR_IO;
            }
        } else {
            ret = write(fd, builder->buf_iprd.data + *offset_iprd_it, 495);
            if (ret != (ssize_t)iprd_len) {
                return CB_BUILDER_ERROR_IO;
            }
            ret = write(fd, zero_pad, 1);
            if (ret != 1) {
                return CB_BUILDER_ERROR_IO;
            }
        }
    }
    return CB_BUILDER_ERROR_NONE;
}
