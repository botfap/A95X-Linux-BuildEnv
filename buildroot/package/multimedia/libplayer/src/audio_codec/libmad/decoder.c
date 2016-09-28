/*
* libmad - MPEG audio decoder library
* Copyright (C) 2000-2004 Underbit Technologies, Inc.
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
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
* $Id: decoder.c,v 1.22 2004/01/23 09:41:32 rob Exp $
*/

#include<stdlib.h>
# include <stdio.h>
#include <string.h>

# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

# include "global.h"

# ifdef HAVE_SYS_TYPES_H
#  include <sys/types.h>
# endif

# ifdef HAVE_SYS_WAIT_H
#  include <sys/wait.h>
# endif

# ifdef HAVE_UNISTD_H
#  include <unistd.h>
# endif

# ifdef HAVE_FCNTL_H
#  include <fcntl.h>
# endif

# include <stdlib.h>

# ifdef HAVE_ERRNO_H
#  include <errno.h>
# endif

# include "stream.h"
# include "frame.h"
# include "synth.h"
# include "decoder.h"

#ifndef _WIN32
#include "../../amadec/adec-armdec-mgt.h"
#define audio_codec_print printf
#ifdef ANDROID
#include <android/log.h>
#define  LOG_TAG    "MadDecoder"
#define audio_codec_print(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#endif 
#else
#define audio_codec_print printf
#endif

struct mad_decoder decoder;
char *pcm_out_data;
int *pcm_out_len;
enum mad_flow (*error_func)(void *, struct mad_stream *, struct mad_frame *);
void *error_data;
int bad_last_frame = 0;
struct mad_stream *stream;
struct mad_frame *frame;
struct mad_synth *synth;
int result = 0;

/*
* This is a private message structure. A generic pointer to this structure
* is passed to each of the callback functions. Put here any data you need
* to access from within the callbacks.
*/

struct buffer {
	unsigned char  *start;
	unsigned long length;
};


enum tagtype {
	TAGTYPE_NONE = 0,
	TAGTYPE_ID3V1,
	TAGTYPE_ID3V2,
	TAGTYPE_ID3V2_FOOTER
};

unsigned long id3_parse_uint(char **ptr, unsigned int bytes)
{
	unsigned long value = 0;

	switch (bytes) {
	case 4: value = (value << 8) | *(*ptr)++;
	case 3: value = (value << 8) | *(*ptr)++;
	case 2: value = (value << 8) | *(*ptr)++;
	case 1: value = (value << 8) | *(*ptr)++;
	}

	return value;
}

unsigned long id3_parse_syncsafe(char **ptr, unsigned int bytes)
{
	unsigned long value = 0;

	switch (bytes) {
	case 5: value = (value << 4) | (*(*ptr)++ & 0x0f);
	case 4: value = (value << 7) | (*(*ptr)++ & 0x7f);
		value = (value << 7) | (*(*ptr)++ & 0x7f);
		value = (value << 7) | (*(*ptr)++ & 0x7f);
		value = (value << 7) | (*(*ptr)++ & 0x7f);
	}

	return value;
}
static
	void parse_header(char **ptr,
	unsigned int *version, int *flags, int *size)
{
	*ptr += 3;

	*version = id3_parse_uint(ptr, 2);
	*flags   = id3_parse_uint(ptr, 1);
	*size    = id3_parse_syncsafe(ptr, 4);
}

enum tagtype tagtype(char  *data, int length)
{
	if (length >= 3 &&
	data[0] == 'T' && data[1] == 'A' && data[2] == 'G')
	return TAGTYPE_ID3V1;

	if (length >= 10 &&
		((data[0] == 'I' && data[1] == 'D' && data[2] == '3') ||
		(data[0] == '3' && data[1] == 'D' && data[2] == 'I')) &&
		data[3] < 0xff && data[4] < 0xff &&
		data[6] < 0x80 && data[7] < 0x80 && data[8] < 0x80 && data[9] < 0x80)
		return data[0] == 'I' ? TAGTYPE_ID3V2 : TAGTYPE_ID3V2_FOOTER;

	return TAGTYPE_NONE;
}

int id3_tag_query(char  *data, int length)
{
	unsigned int version;
	int flags;
	int size;

	switch (tagtype(data, length)) {
	case TAGTYPE_ID3V1:
		return 128;

	case TAGTYPE_ID3V2:
		parse_header(&data, &version, &flags, &size);

		if (flags)
			size += 10;

		return 10 + size;

	case TAGTYPE_ID3V2_FOOTER:
		parse_header(&data, &version, &flags, &size);
		return -size - 10;

	case TAGTYPE_NONE:
		break;
	}

	return 0;
}


/*
* NAME:	decoder->init()
* DESCRIPTION:	initialize a decoder object with callback routines
*/
void mad_decoder_init(struct mad_decoder *decoder, void *data,
	enum mad_flow (*input_func)(void *,
struct mad_stream *),
	enum mad_flow (*header_func)(void *,
struct mad_header const *),
	enum mad_flow (*filter_func)(void *,
struct mad_stream const *,
struct mad_frame *),
	enum mad_flow (*output_func)(void *,
struct mad_header const *,
struct mad_pcm *),
	enum mad_flow (*error_func)(void *,
struct mad_stream *,
struct mad_frame *),
	enum mad_flow (*message_func)(void *,
	void *, unsigned int *))
{
	decoder->mode         = -1;

	decoder->options      = 0;

	decoder->async.pid    = 0;
	decoder->async.in     = -1;
	decoder->async.out    = -1;

	decoder->sync         = 0;

	decoder->cb_data      = 0;//data;

	decoder->input_func   = input_func;
	decoder->header_func  = header_func;
	decoder->filter_func  = filter_func;
	decoder->output_func  = output_func;
	decoder->error_func   = error_func;
	decoder->message_func = message_func;
}

int mad_decoder_finish(struct mad_decoder *decoder)
{
# if defined(USE_ASYNC)
	if (decoder->mode == MAD_DECODER_MODE_ASYNC && decoder->async.pid) {
		pid_t pid;
		int status;

		close(decoder->async.in);

		do
		pid = waitpid(decoder->async.pid, &status, 0);
		while (pid == -1 && errno == EINTR);

		decoder->mode = -1;

		close(decoder->async.out);

		decoder->async.pid = 0;
		decoder->async.in  = -1;
		decoder->async.out = -1;

		if (pid == -1)
			return -1;

		return (!WIFEXITED(status) || WEXITSTATUS(status)) ? -1 : 0;
	}
# endif

	return 0;
}

# if defined(USE_ASYNC)
static
	enum mad_flow send_io(int fd, void const *data, size_t len)
{
	char const *ptr = data;
	ssize_t count;

	while (len) {
		do
		count = write(fd, ptr, len);
		while (count == -1 && errno == EINTR);

		if (count == -1)
			return MAD_FLOW_BREAK;

		len -= count;
		ptr += count;
	}

	return MAD_FLOW_CONTINUE;
}

static
	enum mad_flow receive_io(int fd, void *buffer, size_t len)
{
	char *ptr = buffer;
	ssize_t count;

	while (len) {
		do
		count = read(fd, ptr, len);
		while (count == -1 && errno == EINTR);

		if (count == -1)
			return (errno == EAGAIN) ? MAD_FLOW_IGNORE : MAD_FLOW_BREAK;
		else if (count == 0)
			return MAD_FLOW_STOP;

		len -= count;
		ptr += count;
	}

	return MAD_FLOW_CONTINUE;
}

static
	enum mad_flow receive_io_blocking(int fd, void *buffer, size_t len)
{
	int flags, blocking;
	enum mad_flow result;

	flags = fcntl(fd, F_GETFL);
	if (flags == -1)
		return MAD_FLOW_BREAK;

	blocking = flags & ~O_NONBLOCK;

	if (blocking != flags &&
		fcntl(fd, F_SETFL, blocking) == -1)
		return MAD_FLOW_BREAK;

	result = receive_io(fd, buffer, len);

	if (flags != blocking &&
		fcntl(fd, F_SETFL, flags) == -1)
		return MAD_FLOW_BREAK;

	return result;
}

static
	enum mad_flow send(int fd, void const *message, unsigned int size)
{
	enum mad_flow result;

	/* send size */

	result = send_io(fd, &size, sizeof(size));

	/* send message */

	if (result == MAD_FLOW_CONTINUE)
		result = send_io(fd, message, size);

	return result;
}

static
	enum mad_flow receive(int fd, void **message, unsigned int *size)
{
	enum mad_flow result;
	unsigned int actual;

	if (*message == 0)
		*size = 0;

	/* receive size */

	result = receive_io(fd, &actual, sizeof(actual));

	/* receive message */

	if (result == MAD_FLOW_CONTINUE) {
		if (actual > *size)
			actual -= *size;
		else {
			*size  = actual;
			actual = 0;
		}

		if (*size > 0) {
			if (*message == 0) {
				*message = malloc(*size);
				if (*message == 0)
					return MAD_FLOW_BREAK;
			}

			result = receive_io_blocking(fd, *message, *size);
		}

		/* throw away remainder of message */

		while (actual && result == MAD_FLOW_CONTINUE) {
			char sink[256];
			unsigned int len;

			len = actual > sizeof(sink) ? sizeof(sink) : actual;

			result = receive_io_blocking(fd, sink, len);

			actual -= len;
		}
	}

	return result;
}

static
	enum mad_flow check_message(struct mad_decoder *decoder)
{
	enum mad_flow result;
	void *message = 0;
	unsigned int size;

	result = receive(decoder->async.in, &message, &size);

	if (result == MAD_FLOW_CONTINUE) {
		if (decoder->message_func == 0)
			size = 0;
		else {
			result = decoder->message_func(decoder->cb_data, message, &size);

			if (result == MAD_FLOW_IGNORE ||
				result == MAD_FLOW_BREAK)
				size = 0;
		}

		if (send(decoder->async.out, message, size) != MAD_FLOW_CONTINUE)
			result = MAD_FLOW_BREAK;
	}

	if (message)
		free(message);

	return result;
}
# endif

static
	enum mad_flow error_default(void *data, struct mad_stream *stream,
struct mad_frame *frame)
{
	int *bad_last_frame = data;

	switch (stream->error) {
	case MAD_ERROR_BADCRC:
		if (*bad_last_frame)
			mad_frame_mute(frame);
		else
			*bad_last_frame = 1;

		return MAD_FLOW_IGNORE;

	default:
		return MAD_FLOW_CONTINUE;
	}
}

static
	int run_sync(struct mad_decoder *decoder)
{

	/*
	if (decoder->input_func == 0)
	return 0;

	if (decoder->error_func) {
	error_func = decoder->error_func;
	//error_data = decoder->cb_data;
	}
	else {
	error_func = error_default;
	error_data = &bad_last_frame;
	}

	stream = &decoder->sync->stream;
	frame  = &decoder->sync->frame;
	synth  = &decoder->sync->synth;

	mad_stream_init(stream);
	mad_frame_init(frame);
	mad_synth_init(synth);

	mad_stream_options(stream, decoder->options);
	*/
	do {
		switch (decoder->input_func(decoder->cb_data, stream)) {
		case MAD_FLOW_STOP:
			goto done;
		case MAD_FLOW_BREAK:
			goto fail;
		case MAD_FLOW_IGNORE:
			continue;
		case MAD_FLOW_CONTINUE:
			break;
		}

		while (1) {
# if defined(USE_ASYNC)
			if (decoder->mode == MAD_DECODER_MODE_ASYNC) {
				switch (check_message(decoder)) {
				case MAD_FLOW_IGNORE:
				case MAD_FLOW_CONTINUE:
					break;
				case MAD_FLOW_BREAK:
					goto fail;
				case MAD_FLOW_STOP:
					goto done;
				}
			}
# endif

			if (decoder->header_func) {
				if (mad_header_decode(&frame->header, stream) == -1) {
					if (!MAD_RECOVERABLE(stream->error))
						break;

					switch (error_func(decoder->cb_data, stream, frame)) {
					case MAD_FLOW_STOP:
						goto done;
					case MAD_FLOW_BREAK:
						goto fail;
					case MAD_FLOW_IGNORE:
					case MAD_FLOW_CONTINUE:
					default:
						continue;
					}
				}

				switch (decoder->header_func(decoder->cb_data, &frame->header)) {
				case MAD_FLOW_STOP:
					goto done;
				case MAD_FLOW_BREAK:
					goto fail;
				case MAD_FLOW_IGNORE:
					continue;
				case MAD_FLOW_CONTINUE:
					break;
				}
			}

			if (mad_frame_decode(frame, stream) == -1) {
				if (!MAD_RECOVERABLE(stream->error))
					break;

				switch (error_func(decoder->cb_data, stream, frame)) {
				case MAD_FLOW_STOP:
					goto done;
				case MAD_FLOW_BREAK:
					goto fail;
				case MAD_FLOW_IGNORE:
					break;
				case MAD_FLOW_CONTINUE:
				default:
					continue;
				}
			}
			else
				bad_last_frame = 0;

			if (decoder->filter_func) {
				switch (decoder->filter_func(decoder->cb_data, stream, frame)) {
				case MAD_FLOW_STOP:
					goto done;
				case MAD_FLOW_BREAK:
					goto fail;
				case MAD_FLOW_IGNORE:
					continue;
				case MAD_FLOW_CONTINUE:
					break;
				}
			}

			mad_synth_frame(synth, frame);

			if (decoder->output_func) {
				switch (decoder->output_func(decoder->cb_data,
					&frame->header, &synth->pcm)) {
				case MAD_FLOW_STOP:
					goto done;
				case MAD_FLOW_BREAK:
					goto fail;
				case MAD_FLOW_IGNORE:
				case MAD_FLOW_CONTINUE:
					break;
				}
			}
		}
	}
	while (stream->error == MAD_ERROR_BUFLEN);

fail:
	result = -1;

done:
	/*
	mad_synth_finish(synth);
	mad_frame_finish(frame);
	mad_stream_finish(stream);
	*/
	//return result;
	return stream->this_frame - stream->buffer;
}

# if defined(USE_ASYNC)
static
	int run_async(struct mad_decoder *decoder)
{
	pid_t pid;
	int ptoc[2], ctop[2], flags;

	if (pipe(ptoc) == -1)
		return -1;

	if (pipe(ctop) == -1) {
		close(ptoc[0]);
		close(ptoc[1]);
		return -1;
	}

	flags = fcntl(ptoc[0], F_GETFL);
	if (flags == -1 ||
		fcntl(ptoc[0], F_SETFL, flags | O_NONBLOCK) == -1) {
			close(ctop[0]);
			close(ctop[1]);
			close(ptoc[0]);
			close(ptoc[1]);
			return -1;
	}

	pid = fork();
	if (pid == -1) {
		close(ctop[0]);
		close(ctop[1]);
		close(ptoc[0]);
		close(ptoc[1]);
		return -1;
	}

	decoder->async.pid = pid;

	if (pid) {
		/* parent */

		close(ptoc[0]);
		close(ctop[1]);

		decoder->async.in  = ctop[0];
		decoder->async.out = ptoc[1];

		return 0;
	}

	/* child */

	close(ptoc[1]);
	close(ctop[0]);

	decoder->async.in  = ptoc[0];
	decoder->async.out = ctop[1];

	_exit(run_sync(decoder));

	/* not reached */
	return -1;
}
# endif

/*
* NAME:	decoder->run()
* DESCRIPTION:	run the decoder thread either synchronously or asynchronously
*/
int mad_decoder_run(struct mad_decoder *decoder, enum mad_decoder_mode mode)
{
	int result;
	int (*run)(struct mad_decoder *) = 0;

	switch (decoder->mode = mode) {
	case MAD_DECODER_MODE_SYNC:
		run = run_sync;
		break;

	case MAD_DECODER_MODE_ASYNC:
# if defined(USE_ASYNC)
		run = run_async;
# endif
		break;
	}

	if (run == 0)
		return -1;

	//decoder->sync = malloc(sizeof(*decoder->sync));
	if (decoder->sync == 0)
		return -1;

	result = run(decoder);

	//free(decoder->sync);
	//decoder->sync = 0;

	return result;
}

/*
* NAME:	decoder->message()
* DESCRIPTION:	send a message to and receive a reply from the decoder process
*/
int mad_decoder_message(struct mad_decoder *decoder,
	void *message, unsigned int *len)
{
# if defined(USE_ASYNC)
	if (decoder->mode != MAD_DECODER_MODE_ASYNC ||
		send(decoder->async.out, message, *len) != MAD_FLOW_CONTINUE ||
		receive(decoder->async.in, &message, len) != MAD_FLOW_CONTINUE)
		return -1;

	return 0;
# else
	return -1;
# endif
}


/*
* This is the input callback. The purpose of this callback is to (re)fill
* the stream buffer which is to be decoded. In this example, an entire file
* has been mapped into memory, so we just call mad_stream_buffer() with the
* address and length of the mapping. When this callback is called a second
* time, we are finished decoding.
*/

static
	enum mad_flow input(void *data,
struct mad_stream *stream)
{
	struct buffer *buffer = data;

	if (!buffer->length)
		return MAD_FLOW_STOP;

	mad_stream_buffer(stream, buffer->start, buffer->length);

	buffer->length = 0;

	return MAD_FLOW_CONTINUE;
}

/*
* The following utility routine performs simple rounding, clipping, and
* scaling of MAD's high-resolution samples down to 16 bits. It does not
* perform any dithering or noise shaping, which would be recommended to
* obtain any exceptional audio quality. It is therefore not recommended to
* use this routine if high-quality output is desired.
*/

static 
	signed int scale(mad_fixed_t sample)
{
	/* round */
	sample += (1L << (MAD_F_FRACBITS - 16));

	/* clip */
	if (sample >= MAD_F_ONE)
		sample = MAD_F_ONE - 1;
	else if (sample < -MAD_F_ONE)
		sample = -MAD_F_ONE;

	/* quantize */
	return sample >> (MAD_F_FRACBITS + 1 - 16);
}

/*
* This is the output callback function. It is called after each frame of
* MPEG audio data has been completely decoded. The purpose of this callback
* is to output (or play) the decoded PCM audio.
*/

static
	enum mad_flow output(void *data,
struct mad_header const *header,
struct mad_pcm *pcm)
{
	unsigned int nchannels, nsamples;
	mad_fixed_t const *left_ch, *right_ch;

	/* pcm->samplerate contains the sampling frequency */

	nchannels = pcm->channels;
	nsamples  = pcm->length;
	left_ch   = pcm->samples[0];
	right_ch  = pcm->samples[1];
	//*pcm_out_len += 4608;
	*pcm_out_len += pcm->length*2*(header->mode>0?2:1);;
	while (nsamples--) {
		signed int sample_l;
		signed int sample_r;

		/* output sample(s) in 16-bit signed little-endian PCM */

		sample_l = scale(*left_ch++);
		//putchar((sample >> 0) & 0xff);
		//putchar((sample >> 8) & 0xff);
		pcm_out_data[0] = sample_l >> 0;
		pcm_out_data[1] = sample_l >> 8;
              pcm_out_data += 2;
		if (nchannels == 2) {
			sample_r = scale(*right_ch++);
			//putchar((sample >> 0) & 0xff);
			//putchar((sample >> 8) & 0xff);
        		pcm_out_data[0] = sample_r >> 0;
        		pcm_out_data[1] = sample_r >> 8;
        		pcm_out_data += 2;
		}
		
	}

	return MAD_FLOW_CONTINUE;
}

/*
* This is the error callback function. It is called whenever a decoding
* error occurs. The error is indicated by stream->error; the list of
* possible MAD_ERROR_* errors can be found in the mad.h (or stream.h)
* header file.
*/

static
	enum mad_flow error(void *data,
struct mad_stream *stream,
struct mad_frame *frame)
{
	struct buffer *buffer = data;
	int tagsize;
	switch (stream->error) 
	{
	case MAD_ERROR_LOSTSYNC:
		tagsize = id3_tag_query(stream->this_frame,
			stream->bufend - stream->this_frame);
		if (tagsize > 0) 
		{
			stream->skiplen = tagsize;

			audio_codec_print("id3 info, size = %d, just skip it!\n", tagsize);

			return MAD_FLOW_CONTINUE;
		}

		/* fall through */

	default:
		break;
	}
	fprintf(stderr, "decoding error 0x%04x (%s) at byte offset %u\n",
		stream->error, mad_stream_errorstr(stream),
		stream->this_frame - stream->buffer);

	/* return MAD_FLOW_BREAK here to stop decoding (and propagate an error) */

	return MAD_FLOW_CONTINUE;
}

/*
* This is the function called by main() above to perform all the decoding.
* It instantiates a decoder object and configures it with the input,
* output, and error callback functions above. A single call to
* mad_decoder_run() continues until a callback function returns
* MAD_FLOW_STOP (to stop decoding) or MAD_FLOW_BREAK (to stop decoding and
* signal an error).
*/

int audio_dec_decode(
#ifndef _WIN32
	audio_decoder_operations_t *adec_ops,
#endif
	char *outbuf, int *outlen, char *inbuf, int inlen/*unsigned char const *start, unsigned long length*/)
{
	int result;
	struct buffer buffer;

	buffer.start  = inbuf;
	buffer.length = inlen;

	/* initialize our private message structure */

	pcm_out_data = outbuf;
	pcm_out_len = outlen;
	*pcm_out_len = 0;

	/* configure input, output, and error functions */
	decoder.cb_data = &buffer;

	/* start decoding */

	result = mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);

	/* release the decoder */

	return result;
}

int audio_dec_init(
#ifndef _WIN32
	audio_decoder_operations_t *adec_ops
#endif
)
{
    audio_codec_print("\n\n[%s]adec_ops=%x",__FUNCTION__,adec_ops);
    audio_codec_print("\n\n[%s]BuildDate--%s  BuildTime--%s adec_ops=%x",__FUNCTION__,__DATE__,__TIME__,adec_ops);
	memset(&decoder, 0, sizeof(struct mad_decoder));

	mad_decoder_init(&decoder, 0/*&buffer*/,
		input, 0 /* header */, 0 /* filter */, output,
		error, 0 /* message */);

	if (decoder.input_func == 0)
		return -1;

	if (decoder.error_func)
	{
		error_func = decoder.error_func;
		//error_data = decoder->cb_data;
	}
	else 
	{
		error_func = error_default;
		error_data = &bad_last_frame;
	}

	decoder.sync = malloc(sizeof(*decoder.sync));
	stream = &decoder.sync->stream;
	frame  = &decoder.sync->frame;
	synth  = &decoder.sync->synth;

	mad_stream_init(stream);
	mad_frame_init(frame);
	mad_synth_init(synth);

	mad_stream_options(stream, decoder.options);
    
#ifndef _WIN32
    adec_ops->nInBufSize = 5*1024;
    adec_ops->nOutBufSize = 500*1024;
#endif
	audio_codec_print("libmad init ok!\n");

	return 0;
}

#ifndef _WIN32
int audio_dec_getinfo(audio_decoder_operations_t *adec_ops, void *pAudioInfo)
{   
    return 0;
}
#endif

int audio_dec_release(
#ifndef _WIN32
	audio_decoder_operations_t *adec_ops
#endif
	)
{


	mad_synth_finish(synth);
	mad_frame_finish(frame);
	mad_stream_finish(stream);

	free(decoder.sync);

	mad_decoder_finish(&decoder);

	audio_codec_print("libmad release ok!\n");

	return 0;
}
// win test
#ifdef _WIN32
char *filename = "mnjr.mp3";
char *filename2= "mnjr.pcm";
FILE *fp;
FILE *fp2;
int main(int argc, char *argv[])
{
	char *outbuf;
	int *outlen;
	char *inbuf;
	int inlen;
	int size;
	int rlen;

	size = 0;
	inlen = 0;
	inbuf = (char *)malloc(50001);
	outbuf = (char *)malloc(1920000);
	outlen = (int *)malloc(5);

	//init
	fp =fopen(filename, "rb");
	if(fp == NULL)
	{
		printf("open input file failed!\n");
		return 0;
	}
	fp2 = fopen(filename2, "wb");

	audio_dec_init();

	do
	{
		//input
		if(size)
		{
			memmove(inbuf, inbuf+size, 20000-size);
			rlen = size;
		}
		else
			rlen = 20000;

		inlen = fread(inbuf+20000-rlen, 1, rlen, fp);

		if(inlen == rlen)
		{
			inlen = 20000;
		}

		//decode
		size = audio_dec_decode(
#ifndef _WIN32
			adec_ops,
#endif
			outbuf, outlen, inbuf, inlen);

		//output
		fwrite(outbuf, 1, *outlen, fp2);

	}while(inlen == 20000);

	//release
	audio_dec_release();
	*pcm_out_data = NULL;
	*pcm_out_len = NULL;
	free(inbuf);
	free(outbuf);
	free(outlen);
	fclose(fp);
	fclose(fp2);
	inbuf = NULL;
	outbuf = NULL;
	outlen = NULL;

	return size;
}
#endif
