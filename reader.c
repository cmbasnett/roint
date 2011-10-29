/*
    ------------------------------------------------------------------------------------
    LICENSE:
    ------------------------------------------------------------------------------------
    This file is part of The Open Ragnarok Project
    Copyright 2007 - 2011 The Open Ragnarok Team
    For the latest information visit http://www.open-ragnarok.org
    ------------------------------------------------------------------------------------
    This program is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License as published by the Free Software
    Foundation; either version 2 of the License, or (at your option) any later
    version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License along with
    this program; if not, write to the Free Software Foundation, Inc., 59 Temple
    Place - Suite 330, Boston, MA 02111-1307, USA, or go to
    http://www.gnu.org/copyleft/lesser.txt.
    ------------------------------------------------------------------------------------
*/
#ifdef ROINT_INTERNAL
#	include "internal.h"
#else
#	define _xlog printf
#	define _xalloc malloc
#	define _xfree free
#endif

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>


#define CAST_UP(type, member, ptr) (type*)( (char*)ptr - offsetof(type,member) )
#define CAST_DOWN(ptr, member) ( &ptr->member )


struct _deflatereader {
	struct _reader base;
	struct _reader *parent;
	z_stream stream;
	unsigned long out_offset;
	unsigned long in_start;
	unsigned long in_size; // relative to in_start
	unsigned long in_offset; // relative to in_start
	unsigned char in_buf[256];
};


struct _memreader {
	struct _reader base;
	const unsigned char *data;
	unsigned long size;

	unsigned long offset;
	const unsigned char *ptr;
};


struct _filereader {
	struct _reader base;
	FILE *fp;
};


const int DEFLATEREADER_WINDOW_BITS = MAX_WBITS;


//-------------------------------------------------------------------


void _deflatereader_zerror(const char *funcname, int err) {
	switch(err) {
		case Z_MEM_ERROR:
			_xlog("deflatereader.%s : Z_MEM_ERROR\n", funcname);
			break;
		case Z_BUF_ERROR:
			_xlog("deflatereader.%s : Z_BUF_ERROR\n", funcname);
			break;
		case Z_STREAM_ERROR:
			_xlog("deflatereader.%s : Z_STREAM_ERROR\n", funcname);
			break;
		case Z_DATA_ERROR:
			_xlog("deflatereader.%s : Z_DATA_ERROR\n", funcname);
			break;
		default:
			_xlog("deflatereader.%s : zlib error number %d\n", funcname, err);
			break;
	}
}


voidpf _deflatereader_zalloc_func(struct _deflatereader *deflatereader, uInt items, uInt size) {
	if (_mul_over_limit(items, size, 0xFFFFFFFF))
		return(Z_NULL);
	return((voidpf)_xalloc(items * size));
}


void _deflatereader_zfree_func(struct _deflatereader *deflatereader, voidpf address) {
	_xfree(address);
}


void _deflatereader_input(struct _deflatereader *deflatereader) {
	struct _reader *parent = deflatereader->parent;
	unsigned long bytes;

	
	if (deflatereader->stream.avail_in > 0)
		return; // already has data

	bytes = deflatereader->in_size - deflatereader->in_offset;
	if (bytes > sizeof(deflatereader->in_buf))
		bytes = sizeof(deflatereader->in_buf);
	parent->read(deflatereader->in_buf, bytes, 1, parent);
	if (parent->error) {
		_xlog("deflatereader.input : parent.read error\n");
		return;
	}
	deflatereader->stream.next_in = deflatereader->in_buf;
	deflatereader->stream.avail_in = (uInt)bytes;
	deflatereader->in_offset += bytes;
}


void deflatereader_free(struct _reader *reader) {
	struct _deflatereader *deflatereader = CAST_UP(struct _deflatereader,base,reader);
	int err;

	if (deflatereader->stream.avail_in > 0) {
		struct _reader *parent = deflatereader->parent;
		parent->seek(parent, -(long)deflatereader->stream.avail_in, SEEK_CUR);
	}
	err = inflateEnd(&deflatereader->stream);
	if (err != Z_OK)
		_deflatereader_zerror("destroy", err);
	_xfree(deflatereader);
}


int deflatereader_read(void *dest, unsigned long size, unsigned int count, struct _reader *reader) {
	struct _deflatereader *deflatereader = CAST_UP(struct _deflatereader,base,reader);
	unsigned int wanted;

	reader->error = 0;
	if (_mul_over_limit(size,count,0xFFFFFFFF)) {
		unsigned char *ptr = (unsigned char*)dest;
		unsigned int i;
		for (i = 0; i < count; i++, ptr += size)
			memset(ptr, 0, size);
		_xlog("deflatereader.read : overflow\n");
		reader->error = 1;
		return(reader->error);
	}
	wanted = (unsigned int)size * count;
	deflatereader->stream.next_out = (Bytef*)dest;
	deflatereader->stream.avail_out = (uInt)wanted;

	while (deflatereader->stream.avail_out > 0) {
		_deflatereader_input(deflatereader);
		if (deflatereader->stream.avail_in == 0) {
			_xlog("deflatereader.read : not enough data\n");
			reader->error = 1;
			break;
		}
		else {
			int err = inflate(&deflatereader->stream, Z_SYNC_FLUSH);
			if (err != Z_OK && err != Z_STREAM_END) {
				_deflatereader_zerror("read",err);
				reader->error = 1;
				break;
			}
		}
	}
	if (deflatereader->stream.avail_out > 0)
		memset(deflatereader->stream.next_out, 0, deflatereader->stream.avail_out);
	deflatereader->out_offset += wanted - deflatereader->stream.avail_out;

	return(reader->error);
}


int deflatereader_seek(struct _reader *reader, long pos, int origin) {
	struct _deflatereader *deflatereader = CAST_UP(struct _deflatereader,base,reader);
	struct _reader *parent = deflatereader->parent;
	unsigned long out_offset = deflatereader->out_offset;
	int err;
	unsigned char buf[0x8000]; // 32k

	reader->error = 0;
	switch(origin){
		case SEEK_SET:
			if (pos < 0) {
				_xlog("deflatereader.seek : invalid position\n");
				reader->error = 1;
				break;
			}
			out_offset = pos;
			break;
		case SEEK_CUR:
			if ((pos < 0 && (long)(pos + deflatereader->out_offset) < 0)) {
				_xlog("deflatereader.seek : invalid position\n");
				break;
			}
			out_offset += pos;
			break;
		case SEEK_END:
		default:
			_xlog("deflatereader.seek : not supported (origin=%d)\n", origin);
			reader->error = 1;
			break;
	}
	if (reader->error)
		return(reader->error);
	if (parent->seek(parent, deflatereader->in_start, SEEK_SET) != 0) {
		_xlog("deflatereader.seek : parent.seek error\n");
		reader->error = 1;
		return(reader->error);
	}
	deflatereader->out_offset = 0;
	deflatereader->stream.next_in = Z_NULL;
	deflatereader->stream.avail_in = 0;
	err = inflateReset(&deflatereader->stream);
	if (err != Z_OK) {
		_deflatereader_zerror("seek",err);
		reader->error = 1;
		return(reader->error);
	}
	while (deflatereader->out_offset < out_offset) {
		unsigned long wanted = out_offset - deflatereader->out_offset;
		if (wanted > sizeof(buf))
			wanted = sizeof(buf);
		if (reader->read(buf, wanted, 1, reader) != 0) {
			_xlog("deflatereader.seek : read error\n");
			reader->error = 1;
			break;
		}
	}
	return(reader->error);
}


unsigned long deflatereader_tell(struct _reader *reader) {
	struct _deflatereader *deflatereader = CAST_UP(struct _deflatereader,base,reader);
	return(deflatereader->out_offset);
}


struct _reader *deflatereader_init(struct _reader *parent, unsigned char type) {
	struct _deflatereader *ret = (struct _deflatereader*)_xalloc(sizeof(struct _deflatereader));
	int windowBits;
	int err;

	ret->base.destroy = &deflatereader_free;
	ret->base.read = &deflatereader_read;
	ret->base.seek = &deflatereader_seek;
	ret->base.tell = &deflatereader_tell;
	ret->base.error = 0;
	
	ret->stream.zalloc = (alloc_func)&_deflatereader_zalloc_func;
	ret->stream.zfree = (free_func)&_deflatereader_zfree_func;
	ret->stream.opaque = (voidpf)ret;
	ret->stream.next_in = Z_NULL;
	ret->stream.avail_in = 0;

	ret->parent = parent;
	ret->in_start = parent->tell(parent);
	parent->seek(parent, 0, SEEK_END);
	ret->in_size = parent->tell(parent) - ret->in_start;
	if (parent->error) {
		ret->in_size = 0;
		_xlog("deflatereader.init : parent.tell error\n");
		ret->base.error = 1;
	}
	parent->seek(parent, ret->in_start, SEEK_SET);
	ret->in_offset = 0;
	ret->out_offset = 0;

	if (type == 255)
		windowBits = -DEFLATEREADER_WINDOW_BITS;
	else
		windowBits = DEFLATEREADER_WINDOW_BITS + (int)type * 16;
	err = inflateInit2(&ret->stream, windowBits);
	if (err != Z_OK) {
		_deflatereader_zerror("init",err);
		ret->base.error = 1;
	}

	return(CAST_DOWN(ret,base));
}


//-------------------------------------------------------------------


void memreader_free(struct _reader *reader) {
	struct _memreader *memreader = CAST_UP(struct _memreader,base,reader);
	_xfree(memreader);
}


int memreader_read(void *dest, unsigned long size, unsigned int count, struct _reader *reader) {
	struct _memreader *memreader = CAST_UP(struct _memreader,base,reader);
	unsigned long remaining = memreader->size - memreader->offset;
	unsigned long wanted = size * count;
	if (wanted <= remaining) {
		memcpy(dest, memreader->ptr, wanted);
		memreader->ptr += wanted;
		memreader->offset += wanted;
		reader->error = 0;
	}
	else {
		unsigned long complete = (remaining / size) * size;
		if (complete > 0)
			memcpy(dest, memreader->ptr, complete);
		memset((unsigned char*)dest + complete, 0, wanted - complete);
		memreader->ptr = memreader->data + memreader->size;
		memreader->offset = memreader->size;
		reader->error = 1;
	}
	return(reader->error);
}


int memreader_seek(struct _reader *reader, long pos, int origin) {
	struct _memreader *memreader = CAST_UP(struct _memreader,base,reader);
	reader->error = 0;
	switch(origin){
		case SEEK_SET:
			if (pos < 0 || (unsigned long)pos >= memreader->size) {
				reader->error = 1;
				break;
			}
			memreader->offset = pos;
			break;
		case SEEK_CUR:
			if ((pos >= 0 && pos + memreader->offset >= memreader->size) || (pos < 0 && (long)(pos + memreader->offset) < 0)) {
				reader->error = 1;
				break;
			}
			memreader->offset += pos;
			break;
		case SEEK_END:
			if (pos > 0 || (long)(pos + memreader->size) < 0) {
				reader->error = 1;
				break;
			}
			memreader->offset = pos + memreader->size;
			break;
		default:
			reader->error = 1;
			break;
	}

	memreader->ptr = memreader->data + memreader->offset;

	return(reader->error);
}


unsigned long memreader_tell(struct _reader *reader) {
	struct _memreader *memreader = CAST_UP(struct _memreader,base,reader);
	return(memreader->offset);
}


struct _reader *memreader_init(const unsigned char *ptr, unsigned long size) {
	struct _memreader *ret = (struct _memreader*)_xalloc(sizeof(struct _memreader));

	ret->base.destroy = &memreader_free;
	ret->base.read = &memreader_read;
	ret->base.seek = &memreader_seek;
	ret->base.tell = &memreader_tell;
	ret->base.error = 0;

	ret->data = ptr;
	ret->size = size;

	ret->offset = 0;
	ret->ptr = ptr;

	return(CAST_DOWN(ret,base));
}


//-------------------------------------------------------------------


void filereader_free(struct _reader *reader) {
	struct _filereader *filereader = CAST_UP(struct _filereader,base,reader);
	fclose(filereader->fp);
	_xfree(filereader);
}


int filereader_read(void *dest, unsigned long size, unsigned int count, struct _reader *reader) {
	struct _filereader *filereader = CAST_UP(struct _filereader,base,reader);
	size_t donecount;

	donecount = fread(dest, size, count, filereader->fp);
	if (donecount == count) {
		reader->error = 0;
	}
	else {
		unsigned long wanted = size * count;
		unsigned long complete = size * donecount;
		memset((unsigned char*)dest + complete, 0, wanted - complete);
		reader->error = 1;
	}
	return(reader->error);
}


int filereader_seek(struct _reader *reader, long pos, int origin) {
	struct _filereader *filereader = CAST_UP(struct _filereader,base,reader);
	reader->error = fseek(filereader->fp, pos, origin);
	return(reader->error);
}


unsigned long filereader_tell(struct _reader *reader) {
	struct _filereader *filereader = CAST_UP(struct _filereader,base,reader);
	return(ftell(filereader->fp));
}


struct _reader *filereader_init(const char *fn) {
	struct _filereader *ret = (struct _filereader*)_xalloc(sizeof(struct _filereader));

	ret->base.destroy = &filereader_free;
	ret->base.read = &filereader_read;
	ret->base.seek = &filereader_seek;
	ret->base.tell = &filereader_tell;
	ret->base.error = 0;

	ret->fp = fopen(fn,"rb");
	if (ret->fp == 0) {
		_xlog("Cannot open file %s\n", fn);
		ret->base.error = 1;
	}

	return(CAST_DOWN(ret,base));
}
