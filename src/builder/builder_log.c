#include "builder.h"

void CBBuilder_log(CBBuilder *builder, FILE *stream) {
    if (!builder || !stream) {
        return;
    }
    Offset *offset_iprd_it = builder->offset_iprd.data;
    Offset *cnt_iprd_it = builder->cnt_iprd.data;
    for (
        size_t i = 0;
        i < builder->offset_iprd.size;
        ++i, ++offset_iprd_it, ++cnt_iprd_it) {
        fprintf(
            stream,
            "Album %zu:\n"
            "  offset_iprd: %lu\n"
            "  iprd: %s\n"
            "  cnt: %lu\n",
            i,
            *offset_iprd_it,
            builder->buf_iprd.data + *offset_iprd_it,
            *cnt_iprd_it
        );
    }
    if (builder->cnt_na) {
        fprintf(
            stream,
            "Album NA:\n"
            "  cnt: %lu\n",
            builder->cnt_na
        );
    }
    CBBuilderTrack *track = builder->track.data;
    for (size_t i = 0; i < builder->track.size; ++i, ++track) {
        fprintf(
            stream,
            "Track %zu:\n"
            "  offset_fn: %lu\n"
            "  offset_iprd: %lu\n"
            "  offset_fmt: %lu\n"
            "  offset_inam: %lu\n"
            "  offset_itrk: %lu\n",
            i,
            track->offset_fn,
            track->offset_iprd,
            track->offset_fmt,
            track->offset_inam,
            track->offset_itrk
        );
        fprintf(
            stream,
            "  fn: %s\n"
            "  iprd: %s\n"
            "  inam: %s\n"
            "  itrk: %s\n"
            "  fmt: ",
            builder->buf_fn.data + track->offset_fn,
            track->offset_iprd == -1llu ?
            "N/A" :
            builder->buf_iprd.data + track->offset_iprd,
            track->offset_inam == -1llu ?
            "N/A" :
            builder->buf_inam.data + track->offset_inam,
            track->offset_itrk == -1llu ?
            "N/A" :
            builder->buf_itrk.data + track->offset_itrk
        );
        uint8_t *fmt_data = (uint8_t*)builder->buf_fmt.data + track->offset_fmt;
        for (size_t j = 0; j < 16; ++j, ++fmt_data) {
            fprintf(
                stream,
                "%02X ",
                *fmt_data
            );
        }
        fprintf(stream, "\n");
    }
}
