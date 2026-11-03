#include "builder.h"

#include <stdio.h>

#include <unistd.h>
#include <fcntl.h>

#include <riffutil/riffutil.h>
#include <riffutil/reader/errcode.h>

CBBuilderError CBBuilder_write(CBBuilder *builder, int fd) {
    if (!builder) {
        return CB_BUILDER_ERROR_NULLPTR;
    }
    if (fd < 0) {
        return CB_BUILDER_ERROR_IO;
    }
    static const char zero_pad[512] = {0};
    if (builder->verbose) {
        fprintf(
            stderr,
            "Writing header to fd %d\n"
            "Album: %lu\n"
            "Track: %lu\n",
            fd,
            builder->cnt_iprd.size + (builder->cnt_na > 0),
            builder->track.size
        );
    }
    ssize_t ret = write(fd, "CYANBOOK", 8);
    if (ret != 8) {
        return CB_BUILDER_ERROR_IO;
    }
    ret = write(fd, &builder->cnt_iprd.size + (builder->cnt_na > 0), 8);
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
    if (builder->cnt_na) {
        if (builder->verbose) {
            fprintf(
                stderr,
                "Writing album NA:\n"
                "  cnt: %lu\n",
                builder->cnt_na
            );
        }
        ret = write(fd, &builder->cnt_na, 8);
        if (ret != 8) {
            return CB_BUILDER_ERROR_IO;
        }
        ret = write(fd, zero_pad, 8);
        if (ret != 8) {
            return CB_BUILDER_ERROR_IO;
        }
        ret = write(fd, "NA", 3);
        if (ret != 3) {
            return CB_BUILDER_ERROR_IO;
        }
        ret = write(fd, zero_pad, 493);
        if (ret != 493) {
            return CB_BUILDER_ERROR_IO;
        }
    }
    Offset *cnt_iprd_it = builder->cnt_iprd.data;
    Offset *offset_iprd_it = builder->offset_iprd.data;
    Offset *ptr_iprd_it = builder->ptr_iprd.data;
    for (
        size_t i = 0;
        i < builder->cnt_iprd.size;
        ++i, ++cnt_iprd_it, ++offset_iprd_it, ++ptr_iprd_it) {
        if (builder->verbose) {
            fprintf(
                stderr,
                "Writing album %zu:\n"
                "  offset_iprd: %lu\n"
                "  iprd: %s\n"
                "  cnt: %lu\n"
                "  ptr: %lu\n",
                i,
                *offset_iprd_it,
                builder->buf_iprd.data + *offset_iprd_it,
                *cnt_iprd_it,
                *ptr_iprd_it
            );
        }
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
    if (builder->verbose) {
        fprintf(
            stderr,
            "Writing %zu tracks\n"
            "First pass: writing track metadata\n",
            builder->track.size
        );
    }
    long track_start = lseek(fd, 0, SEEK_CUR);
    if (builder->verbose) {
        fprintf(
            stderr,
            "Current file offset: %ld\n",
            track_start
        );
    }
    CBBuilderTrack *track_it = builder->track.data;
    for (size_t i = 0; i < builder->track.size; ++i, ++track_it) {
        if (builder->verbose) {
            fprintf(
                stderr,
                "Writing track %zu:\n"
                "  offset_fmt: %lu\n"
                "  offset_inam: %lu\n"
                "  offset_alb: %lu\n",
                i,
                track_it->offset_fmt,
                track_it->offset_inam,
                track_it->offset_alb
            );
        }
        ret = write(fd, zero_pad, 16);
        if (ret != 16) {
            return CB_BUILDER_ERROR_IO;
        }
        ret = write(fd, builder->buf_fmt.data + track_it->offset_fmt, 16);
        if (ret != 16) {
            return CB_BUILDER_ERROR_IO;
        }
        ret = write(fd, &track_it->offset_alb, 8);
        if (ret != 8) {
            return CB_BUILDER_ERROR_IO;
        }
        size_t inam_len =
            strlen(builder->buf_inam.data + track_it->offset_inam) + 1;
        if (inam_len <= 472) {
            ret =
                write(
                    fd, builder->buf_inam.data + track_it->offset_inam, inam_len);
            if (ret != (ssize_t)inam_len) {
                return CB_BUILDER_ERROR_IO;
            }
            ret = write(fd, zero_pad, 472 - inam_len);
            if (ret != (ssize_t)(472 - inam_len)) {
                return CB_BUILDER_ERROR_IO;
            }
        } else {
            ret =
                write(
                    fd, builder->buf_inam.data + track_it->offset_inam, 471);
            if (ret != (ssize_t)inam_len) {
                return CB_BUILDER_ERROR_IO;
            }
            ret = write(fd, zero_pad, 1);
            if (ret != 1) {
                return CB_BUILDER_ERROR_IO;
            }
        }
    }
    if (builder->verbose) {
        fprintf(
            stderr,
            "%s\n",
            "Second pass: writing samples"
        );
    }
    long data_start = lseek(fd, 0, SEEK_CUR);
    if (builder->verbose) {
        fprintf(
            stderr,
            "Current file offset: %ld\n",
            data_start
        );
    }
    lseek(fd, track_start, SEEK_SET);
    track_it = builder->track.data;
    for (size_t i = 0; i < builder->track.size; ++i, ++track_it) {
        if (builder->verbose) {
            fprintf(
                stderr,
                "Writing track %zu:\n"
                "  offset_fn: %lu\n"
                "  fn: %s\n",
                i,
                track_it->offset_fn,
                builder->buf_fn.data + track_it->offset_fn
            );
        }
        int fd_track =
            open(
                builder->buf_fn.data + track_it->offset_fn,
                O_RDONLY
            );
        if (fd_track < 0) {
            return CB_BUILDER_ERROR_IO;
        }
        RIFFReader reader;
        char chunk_id_data[] = "+RIFF.-WAVE.+data";
        RIFFReaderError riff_err =
            riff_reader_init(
                &reader,
                chunk_id_data,
                fd_track,
                false
            );
        if (riff_err) {
            fprintf(
                stderr,
                "Error initializing RIFF reader for track %zu\n"
                "RIFF Reader error code: %d (%s)\n",
                i,
                riff_err,
                RIFF_READER_ERROR_LOOKUP[riff_err]
            );
            close(fd_track);
            return CB_BUILDER_ERROR_IO;
        }
        uint8_t *data = 0;
        size_t data_size = 0;
        riff_err = riff_reader_read(&reader, &data, &data_size);
        if (riff_err) {
            fprintf(
                stderr,
                "Error reading RIFF data for track %zu\n"
                "RIFF Reader error code: %d (%s)\n",
                i,
                riff_err,
                RIFF_READER_ERROR_LOOKUP[riff_err]
            );
            close(fd_track);
            return CB_BUILDER_ERROR_IO;
        }
        close(fd_track);
        lseek(fd, 8, SEEK_CUR);
        ret = write(fd, &data_size, 8);
        if (ret != 8) {
            free(data);
            close(fd_track);
            return CB_BUILDER_ERROR_IO;
        }
        free(data);
        lseek(fd, 496, SEEK_CUR);
    }
    return CB_BUILDER_ERROR_NONE;
}
