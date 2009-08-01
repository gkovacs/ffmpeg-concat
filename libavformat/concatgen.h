/*
 * Generic functions used by playlist/concatenation demuxers
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

#ifndef AVFORMAT_CONCATGEN_H
#define AVFORMAT_CONCATGEN_H

#include "playlist.h"

int ff_concatgen_read_packet(AVFormatContext *s, AVPacket *pkt);

int ff_concatgen_read_seek(AVFormatContext *s, int stream_index, int64_t pts, int flags);

int64_t ff_concatgen_read_timestamp(AVFormatContext *s,
                                    int stream_index,
                                    int64_t *pos,
                                    int64_t pos_limit);

int ff_concatgen_read_close(AVFormatContext *s);

int ff_concatgen_read_play(AVFormatContext *s);

int ff_concatgen_read_pause(AVFormatContext *s);

#endif /* AVFORMAT_CONCATGEN_H */

