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

/** @file libavformat/avplaylist.h
 *  @author Geza Kovacs ( gkovacs mit edu )
 *
 *  @brief General components used by playlist formats
 *
 *  @details These functions are used to initialize and manipulate playlists
 *  (AVPlaylistContext) and AVFormatContext for each playlist item.
 *  This abstraction is used to implement file concatenation and
 *  support for playlist formats.
 */

#ifndef AVFORMAT_AVPLAYLIST_H
#define AVFORMAT_AVPLAYLIST_H

#include <libgen.h>
#include "avformat.h"

/** @struct AVPlaylistContext
 *  @brief Represents the playlist and contains AVFormatContext for each playlist item.
 */
typedef struct AVPlaylistContext {
    char **flist;                         /**< List of file names for each playlist item */
    AVFormatContext **formatcontext_list; /**< List of AVFormatContext for each playlist item */
    int pelist_size;                      /**< Number of playlist elements stored in formatcontext_list */
    int pe_curidx;                        /**< Index of the AVFormatContext in formatcontext_list that packets are being read from */
    int64_t *durations;                   /**< Sum of previous durations, in AV_TIME_BASE units, for each playlist item */
    int *nb_streams_list;                 /**< List of the number of streams in each playlist item*/
} AVPlaylistContext;

/** @brief Allocates AVFormatContext, then opens file, and probes and opens streams.
 *  @param filename Null-terminated string of path to file to open.
 *  @return Returns an allocated AVFormatContext upon success, or NULL upon failure.
 */
AVFormatContext *av_playlist_alloc_formatcontext(char *filename);

/** @brief Opens the playlist element with the specified index from the AVPlaylistContext.
 *  @param ctx AVPlaylistContext containing the desired playlist element.
 *  @param pe_curidx Index of the playlist element to be opened.
 *  @return Returns 0 upon success, or negative upon failure.
 */
int av_playlist_populate_context(AVPlaylistContext *ctx, int pe_curidx);

/** @brief Sets the master concat-type demuxer's streams to those of its currently opened playlist element.
 *  @param s AVFormatContext of the concat-type demuxer, which contains the AVPlaylistContext and substreams.
 *  @return Returns 0 upon success, or negative upon failure.
 */
int av_playlist_set_streams(AVFormatContext *s);

/** @brief Allocates and populates a new AVFormatContext for a concat-type demuxer.
 *  @param flist List of filenames from which to construct the playlist.
 *  @param len Length of filename list.
 *  @return Returns NULL if failed, or AVFormatContext if succeeded.
 */
AVFormatContext *av_playlist_formatcontext_from_filelist(const char **flist, int len);

/** @brief Returns AVPlaylistContext continaed within a concat-type demuxer.
 *  @param ic AVFormatContext of the concat-type demuxer, which contains the AVPlaylistContext.
 *  @return Returns NULL if failed (not concat-type demuxer or Playlist not yet allocated), or AVPlaylistContext if succeeded.
 */
AVPlaylistContext* av_playlist_get_context(AVFormatContext *ic);

/** @brief Converts a list of mixed relative or absolute paths into all absolute paths.
 *  @param flist List of null-terminated strings of relative or absolute paths.
 *  @param len Number of paths in flist.
 *  @param workingdir Path that strings in flist are relative to.
 */
void av_playlist_relative_paths(char **flist,
                                int len,
                                const char *workingdir);

/** @brief Splits a character-delimited string into a list of strings.
 *  @param s The input character-delimited string ("one,two,three").
 *  @param sep The delimiter character (',').
 *  @param flist_ptr Pointer to string list which will be allocated by function.
 *  @param len_ptr Number of segments the string was split into.
 *  @return Returns 0 upon success, or negative upon failure.
 */
int av_playlist_split_encodedstring(const char *s,
                                     const char sep,
                                     char ***flist_ptr,
                                     int *len_ptr);

/** @brief Allocates and returns a AVPlaylistContext with playlist elements specified by a file list.
 *  @param flist List of filenames from which to construct the playlist.
 *  @param len Length of filename list.
 *  @return Returns the allocated AVPlaylistContext.
 */
AVPlaylistContext *av_playlist_from_filelist(const char **flist, int len);

/** @brief Creates and adds AVFormatContext for item located at specified path to a AVPlaylistContext.
 *  @param ctx Pre-allocated AVPlaylistContext to add elements to.
 *  @param itempath Absolute path to item for which to add a playlist element.
 *  @return Returns 0 upon success, or negative upon failure.
 */
int av_playlist_add_path(AVPlaylistContext *ctx, const char *itempath);

/** @brief Calculates the index of the playlist item which would contain the timestamp specified in AV_TIME_BASE units.
 *  @param ctx AVPlaylistContext within which the list of playlist elements and durations are stored.
 *  @param pts Timestamp in AV_TIME_BASE.
 *  @param localpts Time in the local demuxer's timeframe in AV_TIME_BASE units; if null, not calculated.
 *  @return Returns the index of the stream which covers the specified time range.
 */
int av_playlist_stream_index_from_time(AVPlaylistContext *ctx,
                                       int64_t pts,
                                       int64_t *localpts);

/** @brief Calculates the local stream index which corresponds to a global stream index.
 *  @param ctx AVPlaylistContext within which the list of playlist elements and durations are stored.
 *  @param stream_index Global stream index, the index of the stream within the playlist demuxer.
 *  @return Returns the local stream index, the index of the stream within the child demuxer.
 */
int av_playlist_localstidx_from_streamidx(AVPlaylistContext *ctx, int stream_index);

/** @brief Calculates the stream offset which corresponds to the given playlist item index.
 *  @param ctx AVPlaylistContext within which the list of playlist elements and durations are stored.
 *  @param playidx Playlist item index, the index of the child demuxer within ctx->formatcontext_list.
 *  @return Returns the stream offset, which is global stream index - local stream index.
 */
int av_playlist_streams_offset_from_playidx(AVPlaylistContext *ctx, int playidx);

#endif /* AVFORMAT_AVPLAYLIST_H */
