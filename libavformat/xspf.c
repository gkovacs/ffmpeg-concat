/*
 * XSPF playlist demuxer
 * Copyright (c) 2009 Geza Kovacs
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/** @file libavformat/pls.c
 *  @author Geza Kovacs ( gkovacs mit edu )
 *
 *  @brief XSPF playlist demuxer
 */

#include "concatgen.h"
#include "riff.h"
#include "libavutil/avstring.h"
#include "internal.h"
#include "playlist.h"

/* The ffmpeg codecs we support, and the IDs they have in the file */
static const AVCodecTag codec_xspf_tags[] = {
    { 0, 0 },
};

static int xspf_probe(AVProbeData *p)
{
    int i, len;
    char found_xml, found_tag;
    unsigned char c;
    const char match_xml[] = "<?xml";
    char buf_xml[5] = {0};
    const char match_tag[] = "<playlist";
    char buf_tag[9] = {0};
    found_xml = found_tag = 0;
    len = p->buf_size;
    for (i = 0; i < len; i++) {
        if (found_tag)
            break;
        c = p->buf[i];
        if (!found_xml) {
            memmove(buf_xml, buf_xml+1, 4);
            buf_xml[4] = c;
            if (!memcmp(buf_xml, match_xml, 5))
                found_xml = 1;
        }
        if (!found_tag) {
            memmove(buf_tag, buf_tag+1, 8);
            buf_tag[8] = c;
            if (!memcmp(buf_tag, match_tag, 9))
                found_tag = 1;
        }
    }
    if (found_xml && found_tag)
        return AVPROBE_SCORE_MAX;
    else if (found_xml || found_tag)
        return AVPROBE_SCORE_MAX / 2;
    else
        return 0;
}

static int xspf_list_files(ByteIOContext *b, char ***flist_ptr, int *len_ptr)
{
    int i, j, c;
    unsigned int buflen;
    char state;
    char **flist, **flist_tmp;
    char buf[1024];
    char buf_tag[10] = {0};
    const char match_tag[] = "<location>";
    flist = NULL;
    state = buflen = i = j = 0;
    while ((c = url_fgetc(b))) {
        if (c == EOF)
            break;
        if (state == 0) {
            memmove(buf_tag, buf_tag+1, 9);
            buf_tag[9] = c;
            if (!memcmp(buf_tag, match_tag, 10))
                state = 1;
        } else {
            if (c == '<') {
                termfn:
                buf[i++] = 0;
                flist_tmp = av_fast_realloc(flist, &buflen, sizeof(*flist) * (j+2));
                if (!flist_tmp) {
                    av_log(NULL, AV_LOG_ERROR, "av_realloc error in m3u_list_files\n");
                    av_free(flist);
                    return AVERROR_NOMEM;
                } else
                    flist = flist_tmp;
                flist[j] = av_malloc(i);
                av_strlcpy(flist[j++], buf, i);
                i = 0;
                state = 0;
                buf_tag[sizeof(buf_tag)-1] = c;
                continue;
            } else {
                buf[i++] = c;
                if (i >= sizeof(buf)-1)
                    goto termfn;
            }
        }
    }
    *flist_ptr = flist;
    *len_ptr = j;
    if (!flist) // no files have been found
        return AVERROR_EOF;
    flist[j] = 0;
    return 0;
}

static int xspf_read_header(AVFormatContext *s,
                            AVFormatParameters *ap)
{
    AVPlaylistContext *ctx;
    char **flist;
    int flist_len, i;
    xspf_list_files(s->pb, &flist, &flist_len);
    if (!flist || flist_len <= 0) {
        fprintf(stderr, "no playlist items found in %s\n", s->filename);
        return AVERROR_EOF;
    }
    av_playlist_relative_paths(flist, flist_len, dirname(s->filename));
    ctx = av_mallocz(sizeof(*ctx));
    if (!ctx) {
        av_log(NULL, AV_LOG_ERROR, "failed to allocate AVPlaylistContext in xspf_read_header\n");
        return AVERROR_NOMEM;
    }
    for (i = 0; i < flist_len; ++i)
        av_playlist_add_path(ctx, flist[i]);
    av_free(flist);
    s->priv_data = ctx;
    ctx->master_formatcontext = s;
    ff_playlist_populate_context(ctx, ctx->pe_curidx);
    ff_playlist_set_streams(ctx);
    return 0;
}

AVInputFormat xspf_demuxer = {
    "xspf",
    NULL_IF_CONFIG_SMALL("CONCAT XSPF format"),
    sizeof(AVPlaylistContext),
    xspf_probe,
    xspf_read_header,
    ff_concatgen_read_packet,
    ff_concatgen_read_close,
    ff_concatgen_read_seek,
    ff_concatgen_read_timestamp,
    0, //flags
    NULL, //extensions
    0, //value
    ff_concatgen_read_play,
    ff_concatgen_read_pause,
    (const AVCodecTag* const []){codec_xspf_tags, 0},
};
