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

#include "config.h"
#include "FLACInput.hxx"
#include "decoder_api.h"
#include "gcc.h"
#include "InputStream.hxx"

FLAC__StreamDecoderReadStatus
FLACInput::Read(FLAC__byte buffer[], size_t *bytes)
{
	size_t r = decoder_read(decoder, input_stream, (void *)buffer, *bytes);
	*bytes = r;

	if (r == 0) {
		if (input_stream_lock_eof(input_stream) ||
		    (decoder != nullptr &&
		     decoder_get_command(decoder) != DECODE_COMMAND_NONE))
			return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
		else
			return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
	}

	return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}

FLAC__StreamDecoderSeekStatus
FLACInput::Seek(FLAC__uint64 absolute_byte_offset)
{
	if (!input_stream->seekable)
		return FLAC__STREAM_DECODER_SEEK_STATUS_UNSUPPORTED;

	if (!input_stream_lock_seek(input_stream,
				    absolute_byte_offset, SEEK_SET,
				    nullptr))
		return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;

	return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

FLAC__StreamDecoderTellStatus
FLACInput::Tell(FLAC__uint64 *absolute_byte_offset)
{
	if (!input_stream->seekable)
		return FLAC__STREAM_DECODER_TELL_STATUS_UNSUPPORTED;

	*absolute_byte_offset = (FLAC__uint64)input_stream->offset;
	return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

FLAC__StreamDecoderLengthStatus
FLACInput::Length(FLAC__uint64 *stream_length)
{
	if (input_stream->size < 0)
		return FLAC__STREAM_DECODER_LENGTH_STATUS_UNSUPPORTED;

	*stream_length = (FLAC__uint64)input_stream->size;
	return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
}

FLAC__bool
FLACInput::Eof()
{
	return (decoder != nullptr &&
		decoder_get_command(decoder) != DECODE_COMMAND_NONE &&
		decoder_get_command(decoder) != DECODE_COMMAND_SEEK) ||
		input_stream_lock_eof(input_stream);
}

void
FLACInput::Error(FLAC__StreamDecoderErrorStatus status)
{
	if (decoder == nullptr ||
	    decoder_get_command(decoder) != DECODE_COMMAND_STOP)
		g_warning("%s", FLAC__StreamDecoderErrorStatusString[status]);
}

FLAC__StreamDecoderReadStatus
FLACInput::Read(gcc_unused const FLAC__StreamDecoder *flac_decoder,
		FLAC__byte buffer[], size_t *bytes,
		void *client_data)
{
	FLACInput *i = (FLACInput *)client_data;

	return i->Read(buffer, bytes);
}

FLAC__StreamDecoderSeekStatus
FLACInput::Seek(gcc_unused const FLAC__StreamDecoder *flac_decoder,
		FLAC__uint64 absolute_byte_offset, void *client_data)
{
	FLACInput *i = (FLACInput *)client_data;

	return i->Seek(absolute_byte_offset);
}

FLAC__StreamDecoderTellStatus
FLACInput::Tell(gcc_unused const FLAC__StreamDecoder *flac_decoder,
		FLAC__uint64 *absolute_byte_offset, void *client_data)
{
	FLACInput *i = (FLACInput *)client_data;

	return i->Tell(absolute_byte_offset);
}

FLAC__StreamDecoderLengthStatus
FLACInput::Length(gcc_unused const FLAC__StreamDecoder *flac_decoder,
		  FLAC__uint64 *stream_length, void *client_data)
{
	FLACInput *i = (FLACInput *)client_data;

	return i->Length(stream_length);
}

FLAC__bool
FLACInput::Eof(gcc_unused const FLAC__StreamDecoder *flac_decoder,
	       void *client_data)
{
	FLACInput *i = (FLACInput *)client_data;

	return i->Eof();
}

void
FLACInput::Error(gcc_unused const FLAC__StreamDecoder *decoder,
		 FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	FLACInput *i = (FLACInput *)client_data;

	i->Error(status);
}

