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
    Offset *iter_cnt_iprd = builder->cnt_iprd.data;
    Offset *iter_offset_iprd = builder->offset_iprd.data;
    for (
        size_t i = 0;
        i < builder->cnt_iprd.size;
        ++i, ++iter_cnt_iprd, ++iter_offset_iprd) {
        ret = write(fd, iter_cnt_iprd, 8);
        if (ret != 8) {
            return CB_BUILDER_ERROR_IO;
        }
        ret = write(fd, zero_pad, 8);
        if (ret != 8) {
            return CB_BUILDER_ERROR_IO;
        }
        size_t iprd_len =
            strlen(builder->buf_iprd.data + *iter_offset_iprd) + 1;
        if (iprd_len <= 496) {
            ret = write(fd, builder->buf_iprd.data + *iter_offset_iprd, iprd_len);
            if (ret != (ssize_t)iprd_len) {
                return CB_BUILDER_ERROR_IO;
            }
            ret = write(fd, zero_pad, 496 - iprd_len);
            if (ret != (ssize_t)(496 - iprd_len)) {
                return CB_BUILDER_ERROR_IO;
            }
        } else {
            ret = write(fd, builder->buf_iprd.data + *iter_offset_iprd, 495);
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
