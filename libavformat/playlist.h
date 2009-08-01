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

/** @file libavformat/playlist.h
 *  @author Geza Kovacs ( gkovacs mit edu )
 *
 *  @brief General components used by playlist formats
 *
 *  @details These functions are used to initialize and manipulate playlists
 *  (PlaylistContext) and their individual playlist elements (PlayElem), each
 *  of which encapsulates its own AVFormatContext. This abstraction is used for
 *  implementing file concatenation and support for playlist formats.
 */

#ifndef AVFORMAT_PLAYLIST_H
#define AVFORMAT_PLAYLIST_H

#include <libgen.h>
#include "avformat.h"
#include "riff.h"
#include "libavutil/avstring.h"

/** @struct PlaylistContext
 *  @brief Represents the playlist and contains PlayElem for each playlist item.
 */
typedef struct PlaylistContext {
    char **flist; /**< List of file names for each playlist item */
    AVFormatContext **icl; /**< List of FormatContext for each playlist items */
    int pelist_size; /**< Number of playlist elements stored in icl */
    int pe_curidx; /**< Index of the AVFormatContext in icl that packets are being read from */
    int64_t *durations; /**< Durations, in AV_TIME_BASE units, for each playlist item */
} PlaylistContext;

/** @fn AVFormatContext *ff_playlist_alloc_formatcontext(char *filename)
 *  @brief Allocates and opens file, codecs, and streams associated with filename.
 *  @param filename Null-terminated string of file to open.
 *  @return Returns an allocated AVFormatContext.
 */
AVFormatContext *ff_playlist_alloc_formatcontext(char *filename);

/** @fn void ff_playlist_populate_context(PlaylistContext *ctx, int pe_curidx)
 *  @brief Opens the playlist element with the specified index from the PlaylistContext.
 *  @param ctx PlaylistContext containing the desired playlist element.
 *  @param pe_curidx Index of the playlist element to be opened.
 */
void ff_playlist_populate_context(PlaylistContext *ctx, int pe_curidx);

/** @fn void ff_playlist_set_streams(AVFormatContext *s)
 *  @brief Sets the master concat-type demuxer's streams to those of its currently opened playlist element.
 *  @param s AVFormatContext of the concat-type demuxer, which contains the PlaylistContext and substreams.
 */
void ff_playlist_set_streams(AVFormatContext *s);

/** @fn PlaylistContext* ff_playlist_get_context(AVFormatContext *ic)
 *  @brief Returns PlaylistContext continaed within a concat-type demuxer.
 *  @param ic AVFormatContext of the concat-type demuxer, which contains the PlaylistContext.
 *  @return Returnes NULL if failed (not concat-type demuxer or Playlist not yet allocated), or PlaylistContext if succeeded.
 */
PlaylistContext* ff_playlist_get_context(AVFormatContext *ic);

/** @fn void ff_playlist_set_context(AVFormatContext *ic, PlaylistContext *ctx)
 *  @brief Sets PlaylistContext for a concat-type demuxer.
 *  @param ic AVFormatContext of the concat-type demuxer.
 *  @param ctx PlaylistContext that will be set in the concat-type demuxer.
 */
void ff_playlist_set_context(AVFormatContext *ic, PlaylistContext *ctx);

/** @fn void ff_playlist_relative_paths(char **flist, int len, const char *workingdir)
 *  @brief Converts a list of mixed relative or absolute paths into all absolute paths.
 *  @param flist List of null-terminated strings of relative or absolute paths.
 *  @param len Number of paths in flist.
 *  @param workingdir Path that strings in flist are relative to.
 */
void ff_playlist_relative_paths(char **flist, const int len, const char *workingdir);

/** @fn void ff_playlist_split_encodedstring(char *s, char sep, char ***flist_ptr, int *len_ptr)
 *  @brief Splits a character-delimited string into a list of strings.
 *  @param s The input character-delimited string ("one,two,three").
 *  @param sep The delimiter character (',').
 *  @param flist_ptr Pointer to string list which will be allocated by function.
 *  @param len_ptr Number of segments the string was split into.
 */
void ff_playlist_split_encodedstring(const char *s, const char sep, char ***flist_ptr, int *len_ptr);

/** @fn PlaylistContext *ff_playlist_from_encodedstring(char *s, char sep)
 *  @brief Allocates and returns a PlaylistContext with playlist elements specified by a character-delimited string.
 *  @param s The input character-delimited string ("one,two,three").
 *  @param sep The delimiter character (',').
 *  @return Returns the allocated PlaylistContext.
 */
PlaylistContext *ff_playlist_from_encodedstring(const char *s, const char sep);

/** @fn void ff_playlist_add_path(PlaylistContext *ctx, char *itempath)
 *  @brief Adds PlayElem for item located at specified path to a PlaylistContext.
 *  @param ctx Pre-allocated PlaylistContext to add elements to.
 *  @param itempath Absolute path to item for which to add a playlist element.
 */
void ff_playlist_add_path(PlaylistContext *ctx, const char *itempath);

/** @fn int64_t ff_playlist_time_offset(int64_t *durations, int pe_curidx)
 *  @brief Calculates the total time offset of an element in a PlaylistContext in AV_TIME_BASE units.
 *  @param durations Durations of playlist items in AV_TIME_BASE units, array must be of size greater than or equal to pe_curidx.
 *  @param pe_curidx Index of the playlist element for which to calculate the time offset.
 *  @return Returns the time offset in AV_TIME_BASE units.
 */
int64_t ff_playlist_time_offset(int64_t *durations, const int pe_curidx);

int ff_playlist_stream_index_from_time(PlaylistContext *ctx, int64_t pts);

#endif /* AVFORMAT_PLAYLIST_H */
