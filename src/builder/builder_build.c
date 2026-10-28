#include "builder.h"

#include <stdio.h>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <limits.h>
#include <linux/limits.h>
#include <fcntl.h>

#include <riffutil/riffutil.h>
#include <riffutil/reader/errcode.h>

static CBBuilderError process_iprd(
    CBBuilder *builder,
    char *meta_data,
    uint64_t meta_data_size,
    Offset *offset_iprd,
    Offset *offset_alb) {
    if (!builder || !meta_data) {
        return CB_BUILDER_ERROR_NULLPTR;
    }
    *offset_iprd = -1llu;
    *offset_alb = -1llu;
    Offset *ii = builder->offset_iprd.data;
    Offset *iii = builder->cnt_iprd.data;
    for (size_t i = 0; i < builder->offset_iprd.size; ++i, ++ii, ++iii) {
        if (
            !strcmp(
                builder->buf_iprd.data + *ii,
                meta_data
            )) {
            *offset_alb = i;
            *offset_iprd = *ii;
            ++*iii;
            return CB_BUILDER_ERROR_NONE;
        }
    }
    if (*offset_iprd == -1llu) {
        *offset_iprd = builder->buf_iprd.size;
        int ret =
            DArrayOffset_push_back(
                &builder->offset_iprd,
                offset_iprd
            );
        if (ret) {
            return CB_BUILDER_ERROR_ALLOC;
        }
        ret =
            DArrayChar_push_back_batch(
                &builder->buf_iprd,
                meta_data,
                meta_data_size
            );
        if (ret) {
            return CB_BUILDER_ERROR_ALLOC;
        }
        Offset os = 1;
        ret =
            DArrayOffset_push_back(
                &builder->cnt_iprd,
                &os
            );
        if (ret) {
            return CB_BUILDER_ERROR_ALLOC;
        }
        os = -1llu;
        ret =
            DArrayOffset_push_back(
                &builder->ptr_iprd,
                &os
            );
        if (ret) {
            return CB_BUILDER_ERROR_ALLOC;
        }
    }
    return CB_BUILDER_ERROR_NONE;
}

static int CBBuilderTrack_compare(const void *a, const void *b) {
    const CBBuilderTrack *track_a = (const CBBuilderTrack*)a;
    const CBBuilderTrack *track_b = (const CBBuilderTrack*)b;
    int cmp =
        strcmp(
            track_a->offset_iprd == -1llu ?
            "" :
            track_a->builder->buf_iprd.data + track_a->offset_iprd,
            track_b->offset_iprd == -1llu ?
            "" :
            track_b->builder->buf_iprd.data + track_b->offset_iprd
        );
    if (cmp) {
        return cmp;
    }
    cmp =
        strcmp(
            track_a->offset_itrk == -1llu ?
            "" :
            track_a->builder->buf_itrk.data + track_a->offset_itrk,
            track_b->offset_itrk == -1llu ?
            "" :
            track_b->builder->buf_itrk.data + track_b->offset_itrk
        );
    if (cmp) {
        return cmp;
    }
    cmp =
        strcmp(
            track_a->builder->buf_fn.data + track_a->offset_fn,
            track_b->builder->buf_fn.data + track_b->offset_fn
        );
    return cmp;
}

CBBuilderError CBBuilder_build(CBBuilder *builder, char *dir) {
    if (!builder || !dir) {
        return CB_BUILDER_ERROR_NULLPTR;
    }
    int ret =
        DArrayOffset_push_back(
            &builder->stack_path,
            &builder->buf_fn.size
        );
    if (ret) {
        return CB_BUILDER_ERROR_ALLOC;
    }
    ret = DArrayChar_push_back_batch(&builder->buf_fn, dir, strlen(dir) + 1);
    if (ret) {
        return CB_BUILDER_ERROR_ALLOC;
    }
    Offset path_offset = 0;
    char path[PATH_MAX];
    for (; builder->stack_path.size;) {
        DArrayOffset_pop_back(&builder->stack_path);
        path_offset = builder->stack_path.data[builder->stack_path.size];
        strncpy(path, builder->buf_fn.data + path_offset, sizeof(path));
        size_t path_len = strlen(path);
        if (builder->verbose) {
            fprintf(
                stderr,
                "Processing path: %s\n",
                path
            );
        }
        struct stat st;
        if (stat(
                path,
                &st
            )) {
            if (builder->verbose) {
                fprintf(
                    stderr,
                    "Error stating path: %s\n",
                    path
                );
            }
            continue;
        }
        if (S_ISDIR(st.st_mode)) {
            if (builder->verbose) {
                fprintf(
                    stderr,
                    "Reading directory: %s\n",
                    path
                );
            }
            DIR *dirp = opendir(path);
            if (!dirp) {
                if (builder->verbose) {
                    fprintf(
                        stderr,
                        "Error opening directory: %s\n",
                        path
                    );
                }
                continue;
            }
            struct dirent *entry;
            for (; (entry = readdir(dirp));) {
                if (
                    !strcmp(entry->d_name, ".") ||
                    !strcmp(entry->d_name, "..")) {
                    continue;
                }
                if (builder->verbose) {
                    fprintf(
                        stderr,
                        "Found entry: %s\n",
                        entry->d_name
                    );
                }
                ret =
                    DArrayOffset_push_back(
                        &builder->stack_path,
                        &builder->buf_fn.size
                    );
                if (ret) {
                    closedir(dirp);
                    return CB_BUILDER_ERROR_ALLOC;
                }
                ret =
                    DArrayChar_push_back_batch(
                        &builder->buf_fn,
                        path,
                        path_len
                    );
                if (ret) {
                    closedir(dirp);
                    return CB_BUILDER_ERROR_ALLOC;
                }
                char chr = '/';
                ret = DArrayChar_push_back(&builder->buf_fn, &chr);
                if (ret) {
                    closedir(dirp);
                    return CB_BUILDER_ERROR_ALLOC;
                }
                ret = DArrayChar_push_back_batch(
                    &builder->buf_fn,
                    entry->d_name,
                    strlen(entry->d_name) + 1
                );
                if (ret) {
                    closedir(dirp);
                    return CB_BUILDER_ERROR_ALLOC;
                }
            }
            closedir(dirp);
        } else if (S_ISREG(st.st_mode)) {
            if (builder->verbose) {
                fprintf(
                    stderr,
                    "Checking file: %s\n",
                    path
                );
                fprintf(
                    stderr,
                    "Reported file size: %lu bytes\n",
                    st.st_size
                );
            }
            int fd = open(path, O_RDONLY);
            if (fd < 0) {
                if (builder->verbose) {
                    fprintf(
                        stderr,
                        "Error opening file: %s\n",
                        path
                    );
                }
                continue;
            }
            char buf[4];
            ssize_t bytes_read = read(fd, buf, sizeof(buf));
            if (bytes_read != sizeof(buf)) {
                if (builder->verbose) {
                    fprintf(
                        stderr,
                        "Invalid format: %s\n",
                        path
                    );
                }
                close(fd);
                continue;
            }
            if (memcmp(buf, "RIFF", 4) != 0) {
                if (builder->verbose) {
                    fprintf(
                        stderr,
                        "Invalid format: %s\n",
                        path
                    );
                }
                close(fd);
                continue;
            }
            bytes_read = read(fd, buf, sizeof(buf));
            if (bytes_read != sizeof(buf)) {
                if (builder->verbose) {
                    fprintf(
                        stderr,
                        "Invalid format: %s\n",
                        path
                    );
                }
                close(fd);
                continue;
            }
            if (builder->verbose) {
                fprintf(
                    stderr,
                    "Size of RIFF chunk: %u bytes\n",
                    *(uint32_t*)buf
                );
            }
            if (st.st_size - 8 != *(uint32_t*)buf) {
                fprintf(
                    stderr,
                    "Invalid format: %s\n",
                    path
                );
                close(fd);
                continue;
            }
            bytes_read = read(fd, buf, sizeof(buf));
            if (bytes_read != sizeof(buf)) {
                if (builder->verbose) {
                    fprintf(
                        stderr,
                        "Invalid format: %s\n",
                        path
                    );
                }
                close(fd);
                continue;
            }
            if (memcmp(buf, "WAVE", 4) != 0) {
                if (builder->verbose) {
                    fprintf(
                        stderr,
                        "Invalid format: %s\n",
                        path
                    );
                }
                close(fd);
                continue;
            }
            if (builder->verbose) {
                fprintf(
                    stderr,
                    "Valid WAVE file: %s\n",
                    path
                );
            }
            uint8_t *meta_data = 0;
            uint64_t meta_data_size = 0;
            lseek(fd, 0, SEEK_SET);
            RIFFReader reader;
            char chunk_id_iprd[] = "+RIFF.-WAVE.+LIST.-INFO.+IPRD";
            RIFFReaderError riff_err =
                riff_reader_init(
                    &reader,
                    chunk_id_iprd,
                    fd,
                    false
                );
            if (riff_err) {
                if (builder->verbose) {
                    fprintf(
                        stderr,
                        "Error initializing RIFF reader: %s\n"
                        "RIFF Reader error code: %d (%s)\n",
                        path,
                        riff_err,
                        RIFF_READER_ERROR_LOOKUP[riff_err]
                    );
                }
                close(fd);
                continue;
            }
            CBBuilderTrack track;
            track.builder = builder;
            track.offset_fn = path_offset;
            riff_err = riff_reader_read(&reader, &meta_data, &meta_data_size);
            if (riff_err) {
                if (builder->verbose) {
                    fprintf(
                        stderr,
                        "Error reading RIFF file: %s\n"
                        "RIFF Reader error code: %d (%s)\n",
                        path,
                        riff_err,
                        RIFF_READER_ERROR_LOOKUP[riff_err]
                    );
                }
                switch (riff_err) {
                case RIFF_READER_ERROR_NOT_FOUND:
                    if (builder->verbose) {
                        fprintf(
                            stderr,
                            "No IPRD chunk found in file: %s\n",
                            path
                        );
                    }
                    track.offset_iprd = -1llu;
                    ret =
                        DArrayChar_push_back_batch(
                            &builder->buf_inam,
                            (char*)meta_data,
                            meta_data_size
                        );
                    if (ret) {
                        close(fd);
                        return CB_BUILDER_ERROR_ALLOC;
                    }
                    ++builder->cnt_na;
                    break;
                default:
                    close(fd);
                    continue;
                }
            } else {
                if (builder->verbose) {
                    fprintf(
                        stderr,
                        "IPRD chunk size: %lu bytes\n"
                        "IPRD chunk data: %s\n",
                        meta_data_size,
                        meta_data
                    );
                }
                if (meta_data[meta_data_size - 1]) {
                    meta_data[meta_data_size - 1] = 0;
                }
                CBBuilderError iprd_err =
                    process_iprd(
                        builder,
                        (char*)meta_data,
                        meta_data_size,
                        &track.offset_iprd,
                        &track.offset_alb
                    );
                if (iprd_err) {
                    free(meta_data);
                    close(fd);
                    return iprd_err;
                }
            }

            free(meta_data);
            lseek(fd, 0, SEEK_SET);
            char chunk_id_inam[] = "+RIFF.-WAVE.+LIST.-INFO.+INAM";
            riff_err =
                riff_reader_init(
                    &reader,
                    chunk_id_inam,
                    fd,
                    false
                );
            if (riff_err) {
                if (builder->verbose) {
                    fprintf(
                        stderr,
                        "Error initializing RIFF reader: %s\n"
                        "RIFF Reader error code: %d (%s)\n",
                        path,
                        riff_err,
                        RIFF_READER_ERROR_LOOKUP[riff_err]
                    );
                }
                close(fd);
                continue;
            }
            riff_err = riff_reader_read(&reader, &meta_data, &meta_data_size);
            if (riff_err) {
                if (builder->verbose) {
                    fprintf(
                        stderr,
                        "Error reading RIFF file: %s\n"
                        "RIFF Reader error code: %d (%s)\n",
                        path,
                        riff_err,
                        RIFF_READER_ERROR_LOOKUP[riff_err]
                    );
                }
                switch (riff_err) {
                case RIFF_READER_ERROR_NOT_FOUND:
                    if (builder->verbose) {
                        fprintf(
                            stderr,
                            "No INAM chunk found in file: %s\n",
                            path
                        );
                    }
                    track.offset_inam = -1llu;
                    ret =
                        DArrayChar_push_back_batch(
                            &builder->buf_inam,
                            (char*)meta_data,
                            meta_data_size
                        );
                    if (ret) {
                        close(fd);
                        return CB_BUILDER_ERROR_ALLOC;
                    }
                    break;
                default:
                    close(fd);
                    continue;
                }
            } else {
                if (builder->verbose) {
                    fprintf(
                        stderr,
                        "INAM chunk size: %lu bytes\n"
                        "INAM chunk data: %s\n",
                        meta_data_size,
                        meta_data
                    );
                }
                track.offset_inam = builder->buf_inam.size;
                if (meta_data[meta_data_size - 1]) {
                    meta_data[meta_data_size - 1] = 0;
                }
                ret =
                    DArrayChar_push_back_batch(
                        &builder->buf_inam,
                        (char*)meta_data,
                        meta_data_size
                    );
                if (ret) {
                    free(meta_data);
                    close(fd);
                    return CB_BUILDER_ERROR_ALLOC;
                }
            }
            free(meta_data);
            lseek(fd, 0, SEEK_SET);
            char chunk_id_itrk[] = "+RIFF.-WAVE.+LIST.-INFO.+ITRK";
            riff_err =
                riff_reader_init(
                    &reader,
                    chunk_id_itrk,
                    fd,
                    false
                );
            if (riff_err) {
                if (builder->verbose) {
                    fprintf(
                        stderr,
                        "Error initializing RIFF reader: %s\n"
                        "RIFF Reader error code: %d (%s)\n",
                        path,
                        riff_err,
                        RIFF_READER_ERROR_LOOKUP[riff_err]
                    );
                }
                close(fd);
                continue;
            }
            riff_err = riff_reader_read(&reader, &meta_data, &meta_data_size);
            if (riff_err) {
                if (builder->verbose) {
                    fprintf(
                        stderr,
                        "Error reading RIFF file: %s\n"
                        "RIFF Reader error code: %d (%s)\n",
                        path,
                        riff_err,
                        RIFF_READER_ERROR_LOOKUP[riff_err]
                    );
                }
                switch (riff_err) {
                case RIFF_READER_ERROR_NOT_FOUND:
                    if (builder->verbose) {
                        fprintf(
                            stderr,
                            "No ITRK chunk found in file: %s\n",
                            path
                        );
                    }
                    track.offset_itrk = -1llu;
                    ret =
                        DArrayChar_push_back_batch(
                            &builder->buf_inam,
                            (char*)meta_data,
                            meta_data_size
                        );
                    if (ret) {
                        close(fd);
                        return CB_BUILDER_ERROR_ALLOC;
                    }
                    break;
                default:
                    close(fd);
                    continue;
                }
            } else {
                if (builder->verbose) {
                    fprintf(
                        stderr,
                        "ITRK chunk size: %lu bytes\n"
                        "ITRK chunk data: %s\n",
                        meta_data_size,
                        meta_data
                    );
                }
                track.offset_itrk = builder->buf_itrk.size;
                if (meta_data[meta_data_size - 1]) {
                    meta_data[meta_data_size - 1] = 0;
                }
                ret =
                    DArrayChar_push_back_batch(
                        &builder->buf_itrk,
                        (char*)meta_data,
                        meta_data_size
                    );
                if (ret) {
                    free(meta_data);
                    close(fd);
                    return CB_BUILDER_ERROR_ALLOC;
                }
            }
            free(meta_data);
            lseek(fd, 0, SEEK_SET);
            char chunk_id_fmt[] = "+RIFF.-WAVE.+fmt ";
            riff_err =
                riff_reader_init(
                    &reader,
                    chunk_id_fmt,
                    fd,
                    false
                );
            if (riff_err) {
                if (builder->verbose) {
                    fprintf(
                        stderr,
                        "Error initializing RIFF reader: %s\n"
                        "RIFF Reader error code: %d (%s)\n",
                        path,
                        riff_err,
                        RIFF_READER_ERROR_LOOKUP[riff_err]
                    );
                }
                close(fd);
                continue;
            }
            riff_err = riff_reader_read(&reader, &meta_data, &meta_data_size);
            if (riff_err) {
                if (builder->verbose) {
                    fprintf(
                        stderr,
                        "Error reading RIFF file: %s\n"
                        "RIFF Reader error code: %d (%s)\n",
                        path,
                        riff_err,
                        RIFF_READER_ERROR_LOOKUP[riff_err]
                    );
                }
                switch (riff_err) {
                case RIFF_READER_ERROR_NOT_FOUND:
                    if (builder->verbose) {
                        fprintf(
                            stderr,
                            "No fmt chunk found in file: %s\n",
                            path
                        );
                    }
                    /* fallthrough */
                    // fall through to default case to skip this file since fmt
                    // chunk is required
                default:
                    close(fd);
                    continue;
                }
            } else {
                if (meta_data_size != 16) {
                    fprintf(
                        stderr,
                        "Invalid fmt chunk size: %lu bytes\n"
                        "Expected fmt chunk size: 16 bytes\n",
                        meta_data_size
                    );
                    free(meta_data);
                    close(fd);
                    continue;
                }
                if (*(uint16_t*)meta_data != 0x0001) {
                    fprintf(
                        stderr,
                        "Invalid audio format: %04X\n"
                        "Expected audio format: 0x0001 (PCM)\n",
                        *(uint16_t*)meta_data
                    );
                    free(meta_data);
                    close(fd);
                    continue;
                }
                if (*(uint32_t*)(meta_data + 4) > builder->sampling_rate_cap) {
                    fprintf(
                        stderr,
                        "Invalid sampling rate: %u Hz\n"
                        "Expected sampling rate: %lu Hz\n",
                        *(uint32_t*)(meta_data + 4),
                        builder->sampling_rate_cap
                    );
                    free(meta_data);
                    close(fd);
                    continue;
                }
                if (builder->verbose) {
                    fprintf(
                        stderr,
                        "fmt chunk size: %lu bytes\n"
                        "fmt chunk data:\n",
                        meta_data_size
                    );
                    for (size_t i = 0; i < meta_data_size; i++) {
                        fprintf(
                            stderr,
                            "%02X ",
                            meta_data[i]
                        );
                        if (i && !(i & 0xf)) {
                            fprintf(stderr, "\n");
                        }
                    }
                    if (meta_data_size && !(meta_data_size & 0xf)) {
                        fprintf(stderr, "\n");
                    }
                }
                track.offset_fmt = builder->buf_fmt.size;
                ret =
                    DArrayChar_push_back_batch(
                        &builder->buf_fmt,
                        (char*)meta_data,
                        meta_data_size
                    );
                if (ret) {
                    free(meta_data);
                    close(fd);
                    return CB_BUILDER_ERROR_ALLOC;
                }
            }
            free(meta_data);
            close(fd);
            ret = DArrayCBBuilderTrack_push_back(&builder->track, &track);
            if (ret) {
                return CB_BUILDER_ERROR_ALLOC;
            }
        } else {
            if (builder->verbose) {
                fprintf(
                    stderr,
                    "Skipping non-regular file: %s\n",
                    path
                );
            }
        }
    }
    qsort(
        builder->track.data,
        builder->track.size,
        sizeof(CBBuilderTrack),
        CBBuilderTrack_compare
    );
    CBBuilderTrack *iter_track = builder->track.data;
    for (size_t i = 0; i < builder->track.size; ++i, ++iter_track) {
        if (
            iter_track->offset_alb == -1llu ||
            builder->ptr_iprd.data[iter_track->offset_alb] != -1llu) {
            continue;
        }
        builder->ptr_iprd.data[iter_track->offset_alb] = i + builder->cnt_na;
    }
    return CB_BUILDER_ERROR_NONE;
}
