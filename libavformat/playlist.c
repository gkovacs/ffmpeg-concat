/*
 * M3U muxer and demuxer
 * Copyright (c) 2001 Geza Kovacs
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

/*
 * Based on AU muxer and demuxer in au.c
 */

#include "avformat.h"
#include "playlist.h"
#include <time.h>

int av_open_input_playelem(PlayElem *pe)
{
//    return av_open_input_file(&(pe->ic), pe->filename, 0, 0, 0);
    return av_open_input_file(&(pe->ic), pe->filename, pe->fmt, pe->buf_size, pe->ap);
}


// based on decode_thread() in ffplay.c
int av_alloc_playelem(unsigned char *filename, PlayElem *pe)
{
//    AVProbeData *pd;
    AVFormatContext *ic;
    AVFormatParameters *ap;
//    pd = av_malloc(sizeof(AVProbeData));
    ic = av_malloc(sizeof(AVFormatContext));
    ap = av_malloc(sizeof(AVFormatParameters));
    memset(ap, 0, sizeof(AVFormatParameters));
    ap->width = 0;
    ap->height = 0;
    ap->time_base = (AVRational){1, 25};
    ap->pix_fmt = 0;
//    pd->filename = filename;
//    pd->buf = NULL;
//    pd->buf_size = 0;
    pe->ic = ic;
    pe->filename = filename;
    pe->fmt = 0;
    pe->buf_size = 0;
    pe->ap = ap;
//    pe->fmt = pe->ic->iformat;
    return 0;
}

PlayElem* av_make_playelem(unsigned char *filename)
{
    int err;
    PlayElem *pe = av_malloc(sizeof(PlayElem));
    err = av_alloc_playelem(filename, pe);
    if (err < 0)
        print_error("during-av_alloc_playelem", err);
    err = av_open_input_playelem(pe);
    if (err < 0)
        print_error("during-open_input_playelem", err);
    pe->fmt = pe->ic->iformat;
    if (!pe->fmt)
    {
        fprintf(stderr, "pefmt not set\n");
        fflush(stderr);
    }
    err = av_find_stream_info(pe->ic);
    if (err < 0)
    {
        fprintf(stderr, "failed codec probe av_find_stream_info\n");
        fflush(stderr);
    }
    if(pe->ic->pb)
    {
        pe->ic->pb->eof_reached = 0;
    }
    else
    {
        fprintf(stderr, "failed pe ic pb not set");
        fflush(stderr);
    }
    if(!pe->fmt)
    {
        fprintf(stderr, "failed pe ic fmt not set");
        fflush(stderr);
    }
    pe->time_offset = 0;
    pe->indv_time = clock();
    return pe;
}

PlaylistD* av_make_playlistd(unsigned char **flist, int flist_len)
{
    int i;
    PlaylistD *playld = av_malloc(sizeof(PlaylistD));
    playld->pe_curidx = 0;
    playld->pelist_size = flist_len;
    playld->pelist = av_malloc(playld->pelist_size * sizeof(PlayElem*));
    memset(playld->pelist, 0, playld->pelist_size * sizeof(PlayElem*));
//    for (int i = 0; i < playld->pelist_size; ++i)
//    {
//        playld->pelist[i] = av_make_playelem(flist[i]);
//    }
    return playld;
}

int playlist_populate_context(PlaylistD *playld, AVFormatContext *s)
{
    int i;
    playld->pelist[playld->pe_curidx] = av_make_playelem(playld->flist[playld->pe_curidx]);
    AVFormatContext *ic = playld->pelist[playld->pe_curidx]->ic;
    AVFormatParameters *nap = playld->pelist[playld->pe_curidx]->ap;
//    ic->iformat->read_header(ic, nap);
//    ic->debug = 1;
    ic->iformat->read_header(ic, 0);
    s->nb_streams = ic->nb_streams;
    for (i = 0; i < ic->nb_streams; ++i)
    {
        s->streams[i] = ic->streams[i];
    }
    s->av_class = ic->av_class;
    s->oformat = ic->oformat;
    s->pb = ic->pb;
//    s->filename = &(ic->filename[0]);
    s->timestamp = ic->timestamp;
//    s->title = &(ic->title[0]);
//    s->author = &(ic->author[0]);
//    s->copyright = &(ic->copyright[0]);
//    s->comment = &(ic->comment[0]);
//    s->album = &(ic->album[0]);
    s->year = ic->year;
    s->track = ic->track;
//    s->genre = &(ic->genre[0]);
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

int check_file_extn(char *cch, char *extn)
{
    int pos;
    int extnl;
    pos = -1;
    extnl = 0;
    while (extn[extnl] != 0)
       ++extnl;
    pos = -1;
    if (!cch)
    {
        return 0;
    }
    if (*cch == 0)
    {
        return 0;
    }
    while (*cch != 0)
    {
        if (*cch == '.')
        {
            pos = 0;
        }
        else if (pos >= 0)
        {
            if (*cch == extn[pos])
                ++pos;
            else
                pos = -1;
            if (pos == extnl)
            {
                return 1;
            }
        }
        ++cch;
    }
    return 0;
}

