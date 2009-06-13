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
    ic = av_malloc(sizeof(AVFormatContext));
    ap = av_malloc(sizeof(AVFormatParameters));
    memset(ap, 0, sizeof(AVFormatParameters));
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

PlayElem* ff_make_playelem(unsigned char *filename)
{
    int err;
    PlayElem *pe = av_malloc(sizeof(PlayElem));
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

PlaylistD* ff_make_playlistd(unsigned char **flist,
                             int flist_len)
{
    int i;
    PlaylistD *playld = av_malloc(sizeof(PlaylistD));
    playld->pe_curidx = 0;
    playld->pelist_size = flist_len;
    playld->pelist = av_malloc(playld->pelist_size * sizeof(PlayElem*));
    memset(playld->pelist, 0, playld->pelist_size * sizeof(PlayElem*));
    return playld;
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

int ff_playlist_populate_context(PlaylistD *playld,
                                 AVFormatContext *s)
{
    int i;
//    unsigned int stream_offset;
    AVFormatContext *ic;
    AVFormatParameters *nap;
    printf("playlist_populate_context called\n");
    playld->pelist[playld->pe_curidx] = ff_make_playelem(playld->flist[playld->pe_curidx]);
    ic = playld->pelist[playld->pe_curidx]->ic;
    nap = playld->pelist[playld->pe_curidx]->ap;
    ic->iformat->read_header(ic, 0);
//    stream_offset = get_stream_offset(s);
    s->nb_streams = ic->nb_streams;
//    s->nb_streams = ic->nb_streams + stream_offset;
    for (i = 0; i < ic->nb_streams; ++i) {
        s->streams[i] = ic->streams[i];
//        s->streams[i+stream_offset] = ic->streams[i];
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

unsigned int ff_get_stream_offset(AVFormatContext *s)
{
    PlaylistD *playld;
    int i;
    unsigned int snum = 0;
    playld = s->priv_data;
    for (i = 0; i < playld->pe_curidx; ++i)
        snum += playld->pelist[i]->ic->nb_streams;
    return snum;
}

