/*
 * Internal functions used to manipulate playlists
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

/** @file libavformat/playlist.h
 *  @author Geza Kovacs ( gkovacs mit edu )
 *
 *  @brief Internal functions used to manipulate playlists
 *
 *  @details These functions are used internally for manipulating playlists.
 *  The public playlist API can be found in avplaylist.h
 */

#ifndef AVFORMAT_PLAYLIST_H
#define AVFORMAT_PLAYLIST_H

#include "avplaylist.h"

/** @brief Opens the playlist element with the specified index from the AVPlaylistContext.
 *  @param ctx AVPlaylistContext containing the desired playlist element.
 *  @param pe_curidx Index of the playlist element to be opened.
 *  @return Returns 0 upon success, or negative upon failure.
 */
int ff_playlist_populate_context(AVPlaylistContext *ctx, int pe_curidx);

/** @brief Sets the master concat-type demuxer's streams to those of its currently opened playlist element.
 *  @param s AVFormatContext of the concat-type demuxer, which contains the AVPlaylistContext and substreams.
 *  @return Returns 0 upon success, or negative upon failure.
 */
int ff_playlist_set_streams(AVFormatContext *s);

#endif /* AVFORMAT_PLAYLIST_H */

