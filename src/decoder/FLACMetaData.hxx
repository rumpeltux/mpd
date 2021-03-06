/*
 * Copyright (C) 2003-2012 The Music Player Daemon Project
 * http://www.musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef MPD_FLAC_METADATA_H
#define MPD_FLAC_METADATA_H

#include "gcc.h"
#include "FLACIOHandle.hxx"

#include <FLAC/metadata.h>

#include <assert.h>

class FLACMetadataChain {
	FLAC__Metadata_Chain *chain;

public:
	FLACMetadataChain():chain(::FLAC__metadata_chain_new()) {}

	~FLACMetadataChain() {
		::FLAC__metadata_chain_delete(chain);
	}

	explicit operator FLAC__Metadata_Chain *() {
		return chain;
	}

	bool Read(const char *path) {
		return ::FLAC__metadata_chain_read(chain, path);
	}

	bool Read(FLAC__IOHandle handle, FLAC__IOCallbacks callbacks) {
		return ::FLAC__metadata_chain_read_with_callbacks(chain,
								  handle,
								  callbacks);
	}

	bool Read(input_stream *is) {
		return Read(::ToFLACIOHandle(is), ::GetFLACIOCallbacks(is));
	}

	bool ReadOgg(const char *path) {
		return ::FLAC__metadata_chain_read_ogg(chain, path);
	}

	bool ReadOgg(FLAC__IOHandle handle, FLAC__IOCallbacks callbacks) {
		return ::FLAC__metadata_chain_read_ogg_with_callbacks(chain,
								      handle,
								      callbacks);
	}

	bool ReadOgg(input_stream *is) {
		return ReadOgg(::ToFLACIOHandle(is), ::GetFLACIOCallbacks(is));
	}

	gcc_pure
	FLAC__Metadata_ChainStatus GetStatus() const {
		return ::FLAC__metadata_chain_status(chain);
	}

	gcc_pure
	const char *GetStatusString() const {
		return FLAC__Metadata_ChainStatusString[GetStatus()];
	}

	void Scan(const struct tag_handler *handler, void *handler_ctx);
};

class FLACMetadataIterator {
	FLAC__Metadata_Iterator *iterator;

public:
	FLACMetadataIterator():iterator(::FLAC__metadata_iterator_new()) {}

	FLACMetadataIterator(FLACMetadataChain &chain)
		:iterator(::FLAC__metadata_iterator_new()) {
		::FLAC__metadata_iterator_init(iterator,
					       (FLAC__Metadata_Chain *)chain);
	}

	~FLACMetadataIterator() {
		::FLAC__metadata_iterator_delete(iterator);
	}

	bool Next() {
		return ::FLAC__metadata_iterator_next(iterator);
	}

	gcc_pure
	FLAC__StreamMetadata *GetBlock() {
		return ::FLAC__metadata_iterator_get_block(iterator);
	}
};

struct tag_handler;
struct tag;
struct replay_gain_info;

static inline unsigned
flac_duration(const FLAC__StreamMetadata_StreamInfo *stream_info)
{
	assert(stream_info->sample_rate > 0);

	return (stream_info->total_samples + stream_info->sample_rate - 1) /
		stream_info->sample_rate;
}

bool
flac_parse_replay_gain(struct replay_gain_info *rgi,
		       const FLAC__StreamMetadata *block);

bool
flac_parse_mixramp(char **mixramp_start, char **mixramp_end,
		   const FLAC__StreamMetadata *block);

void
flac_vorbis_comments_to_tag(struct tag *tag,
			    const FLAC__StreamMetadata_VorbisComment *comment);

void
flac_scan_metadata(const FLAC__StreamMetadata *block,
		   const struct tag_handler *handler, void *handler_ctx);

#endif
