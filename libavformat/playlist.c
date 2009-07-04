/*
 * General components used by playlist formats
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

#include "avformat.h"
#include "playlist.h"
#include "internal.h"
#include <time.h>

// based on decode_thread() in ffplay.c
int ff_alloc_playelem(unsigned char *filename,
                      PlayElem *pe)
{
    AVFormatContext *ic;
    AVFormatParameters *ap;
    ic = av_malloc(sizeof(*ic));
    ap = av_malloc(sizeof(*ap));
    memset(ap, 0, sizeof(*ap));
    ap->width = 0;
    ap->height = 0;
    ap->time_base = (AVRational){1, 25};
    ap->pix_fmt = 0;
    pe->ic = ic;
    pe->filename = filename;
    pe->fmt = 0;
    pe->buf_size = 0;
    pe->ap = ap;
    return 0;
}

PlayElem* ff_playlist_make_playelem(char *filename)
{
    int err;
    PlayElem *pe = av_malloc(sizeof(*pe));
    err = ff_alloc_playelem(filename, pe);
    if (err < 0)
        print_error("during-av_alloc_playelem", err);
    err = av_open_input_file(&(pe->ic), pe->filename, pe->fmt, pe->buf_size, pe->ap);
    if (err < 0)
        print_error("during-open_input_playelem", err);
    pe->fmt = pe->ic->iformat;
    if (!pe->fmt) {
        fprintf(stderr, "pefmt not set\n");
    }
    err = av_find_stream_info(pe->ic);
    if (err < 0) {
        fprintf(stderr, "failed codec probe av_find_stream_info\n");
    }
    if(pe->ic->pb) {
        pe->ic->pb->eof_reached = 0;
    }
    else {
        fprintf(stderr, "failed pe ic pb not set");
    }
    if(!pe->fmt) {
        fprintf(stderr, "failed pe ic fmt not set");
    }
    pe->time_offset = 0;
    pe->indv_time = clock();
    return pe;
}

PlaylistContext* ff_playlist_make_context(char *filename)
{
    int i;
    PlaylistContext *ctx = av_malloc(sizeof(*ctx));
    ctx->time_offsets_size = 2; // TODO don't assume we have just 2 streams
    ctx->time_offsets = av_malloc(sizeof(*(ctx->time_offsets)) * ctx->time_offsets_size);
    for (i = 0; i < ctx->time_offsets_size; ++i)
        ctx->time_offsets[i] = 0;
    ctx->pe_curidxs_size = 2; // TODO don't assume we have just 2 streams
    ctx->pe_curidxs = av_malloc(sizeof(*(ctx->pe_curidxs)) * ctx->pe_curidxs_size);
    for (i = 0; i < ctx->pe_curidxs_size; ++i)
        ctx->pe_curidxs[i] = 0;
    ff_split_wd_fn(filename,
                   &ctx->workingdir,
                   &ctx->filename);
    return ctx;
}

char* ff_conc_strings(char *string1,
                      char *string2)
{
    char *str1;
    char *str2;
    char *str;
    str1 = string1;
    str2 = string2;
    while (*string1 != 0)
        ++string1;
    while (*string2 != 0)
        ++string2;
    str = av_malloc((string1-str1)+(string2-str2));
    string1 = str1;
    string2 = str2;
    while (*string1 != 0)
        str[string1-str1] = *(string1++);
    str += (string1-str1);
    while (*string2 != 0)
        str[string2-str2] = *(string2++);
    return (str-string1)+str1;
}

char* ff_buf_getline(ByteIOContext *s)
{
    char *q;
    char *oq;
    int bufsize = 64;
    q = av_malloc(bufsize);
    oq = q;
    while (1) {
        int c = url_fgetc(s);
        if (c == EOF)
            return NULL;
        if (c == '\n')
            break;
        *q = c;
        if ((++q)-oq == bufsize) {
            oq = av_realloc(oq, bufsize+64);
            q = oq + bufsize;
            bufsize += 64;
        }
    }
    *q = 0;
    q = oq;
    while (*q != 0 && *q != '#') {
        ++q;
    }
    *q = 0;
    oq = av_realloc(oq, (q-oq)+1);
    return oq;
}

void ff_split_wd_fn(char *filepath,
                    char **workingdir,
                    char **filename)
{
    char *ofp;
    char *cofp;
    char *lslash = filepath;
    ofp = filepath;
    cofp = filepath;
    while (*filepath != 0) {
        if (*filepath == '/' || *filepath == '\\')
            lslash = filepath+1;
        ++filepath;
    }
    *workingdir = av_malloc((lslash-ofp)+1);
    *filename = av_malloc((filepath-lslash)+1);
    while (cofp < lslash)
        (*workingdir)[cofp-ofp] = *(cofp++);
    (*workingdir)[cofp-ofp] = 0;
    while (cofp < filepath)
        (*filename)[cofp-lslash] = *(cofp++);
    (*filename)[cofp-lslash] = 0;
}

int ff_playlist_populate_context(PlaylistContext *ctx,
                                 AVFormatContext *s,
                                 int stream_index)
{
    int i;
    AVFormatContext *ic;
    AVFormatParameters *nap;
    printf("playlist_populate_context called\n");
    ctx->pelist[ctx->pe_curidxs[stream_index]] = ff_playlist_make_playelem(ctx->flist[ctx->pe_curidxs[stream_index]]);
    ic = ctx->pelist[ctx->pe_curidxs[stream_index]]->ic;
    nap = ctx->pelist[ctx->pe_curidxs[stream_index]]->ap;
    ic->iformat->read_header(ic, 0);
    s->nb_streams = ic->nb_streams;
    for (i = 0; i < ic->nb_streams; ++i) {
        s->streams[i] = ic->streams[i];
    }
    // TODO remove this ugly hack
    s->av_class = ic->av_class;
    s->oformat = ic->oformat;
    s->pb = ic->pb;
    s->timestamp = ic->timestamp;
    s->year = ic->year;
    s->track = ic->track;
    s->ctx_flags = ic->ctx_flags;
    s->packet_buffer = ic->packet_buffer;
    s->start_time = ic->start_time;
    s->duration = ic->duration;
    s->file_size = ic->file_size;
    s->bit_rate = ic->bit_rate;
    s->cur_st = ic->cur_st;
    s->cur_ptr_deprecated = ic->cur_ptr_deprecated;
    s->cur_len_deprecated = ic->cur_len_deprecated;
    s->cur_pkt_deprecated = ic->cur_pkt_deprecated;
    s->data_offset = ic->data_offset;
    s->index_built = ic->index_built;
    s->mux_rate = ic->mux_rate;
    s->packet_size = ic->packet_size;
    s->preload = ic->preload;
    s->max_delay = ic->max_delay;
    s->loop_output = ic->loop_output;
    s->flags = ic->flags;
    s->loop_input = ic->loop_input;
    s->probesize = ic->probesize;
    s->max_analyze_duration = ic->max_analyze_duration;
    s->key = ic->key;
    s->keylen = ic->keylen;
    s->nb_programs = ic->nb_programs;
    s->programs = ic->programs;
    s->video_codec_id = ic->video_codec_id;
    s->audio_codec_id = ic->audio_codec_id;
    s->subtitle_codec_id = ic->subtitle_codec_id;
    s->max_index_size = ic->max_index_size;
    s->max_picture_buffer = ic->max_picture_buffer;
    s->nb_chapters = ic->nb_chapters;
    s->chapters = ic->chapters;
    s->debug = ic->debug;
    s->raw_packet_buffer = ic->raw_packet_buffer;
    s->raw_packet_buffer_end = ic->raw_packet_buffer_end;
    s->packet_buffer_end = ic->packet_buffer_end;
    s->metadata = ic->metadata;
    return 0;
}

// returns duration in seconds * AV_TIME_BASE
int64_t ff_playlist_get_duration(AVFormatContext *ic, int stream_index)
{
// TODO storing previous packet pts/dts is ugly hack
// ic->stream[]->cur_dts correct
// ic->strea[]->duration correct
// pkt->pts incorrect (huge negative)
// pkt->dts correct, depended on by ffmpeg (need to change)
// ic->stream[]->pts incorrect (0)
// ic->start_time always 0
// changing ic->start_time has no effect
// ic->duration correct, divide by AV_TIME_BASE to get seconds
// h264 and mpeg1: pkt->dts values incorrect
    int64_t durn;

//    durn = ic->duration;
    AVRational avbasetime = {1, AV_TIME_BASE};
    durn = av_rescale_q(ic->streams[stream_index]->duration, ic->streams[stream_index]->time_base, avbasetime);

//    durn = ic->streams[stream_index]->duration; // ogg gives wrong value
    printf("duration is %ld\n", durn);
    return durn;
}

