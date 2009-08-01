/*
 * Minimal playlist/concatenation demuxer
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

/** @file libavformat/concat.c
 *  @author Geza Kovacs ( gkovacs mit edu )
 *
 *  @brief Minimal playlist/concatenation demuxer
 *
 *  @details This is a minimal concat-type demuxer that can be constructed
 *  by allocating a PlayElem for each playlist item and setting its filename,
 *  then allocating a PlaylistContext, creating a list of PlayElem, and setting
 *  the PlaylistContext in the AVFormatContext.
 */

#include "concat.h"

// The FFmpeg codecs we support, and the IDs they have in the file
static const AVCodecTag codec_concat_tags[] = {
    { 0, 0 },
};

// concat demuxer should only be manually constructed in ffmpeg
static int concat_probe(AVProbeData *p)
{
    return 0;
}

// PlaylistD should be constructed externally
static int concat_read_header(AVFormatContext *s,
                              AVFormatParameters *ap)
{
    return 0;
}

AVInputFormat* ff_concat_alloc_demuxer(void)
{
    AVInputFormat *cdm  = av_malloc(sizeof(*cdm));
    cdm->name           = "concat";
    cdm->long_name      = NULL_IF_CONFIG_SMALL("CONCAT format");
    cdm->priv_data_size = sizeof(PlaylistContext);
    cdm->read_probe     = concat_probe;
    cdm->read_header    = concat_read_header;
    cdm->read_packet    = ff_concatgen_read_packet;
    cdm->read_close     = ff_concatgen_read_close;
    cdm->read_seek      = ff_concatgen_read_seek;
    cdm->read_timestamp = ff_concatgen_read_timestamp;
    cdm->flags          = 0;
    cdm->extensions     = NULL;
    cdm->value          = 0;
    cdm->read_play      = ff_concatgen_read_play;
    cdm->read_pause     = ff_concatgen_read_pause;
    cdm->codec_tag      = codec_concat_tags;
    cdm->read_seek2     = NULL;
    cdm->metadata_conv  = NULL;
    cdm->next           = NULL;
    return cdm;
}

#if CONFIG_CONCAT_DEMUXER
AVInputFormat concat_demuxer = {
    "concat",
    NULL_IF_CONFIG_SMALL("CONCAT format"),
    0,
    concat_probe,
    concat_read_header,
    ff_concatgen_read_packet,
    ff_concatgen_read_close,
    ff_concatgen_read_seek,
    ff_concatgen_read_timestamp,
    0, //flags
    NULL, //extensions
    0, //value
    ff_concatgen_read_play,
    ff_concatgen_read_pause,
    (const AVCodecTag* const []){codec_concat_tags, 0},
    NULL, //m3u_read_seek2
    NULL, //metadata_conv
    NULL, //next
};
#endif
