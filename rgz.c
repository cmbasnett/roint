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
#include "internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


unsigned short rgz_inspect(const struct RORgz *rgz) {
	unsigned int i;

	if (rgz == NULL) {
		_xlog("rgz.inspect : invalid argument (rgz=%p)\n", rgz);
		return(0);
	}

	if (rgz->entrycount > 0 && rgz->entries == NULL) {
		_xlog("rgz.inspect : expected NULL entries\n");
		return(0);
	}
	if (rgz->entrycount == 0 && rgz->entries != NULL) {
		_xlog("rgz.inspect : expected non-NULL entries\n");
		return(0);
	}

	for (i = 0; i < rgz->entrycount; i++) {
		const struct RORgzEntry *entry = &rgz->entries[i];
		if (entry->type != 'f' && entry->type != 'd' && entry->type != 'e') {
			_xlog("rgz.inspect : [%u] unknown entry type '%c'\n", i, entry->type);
			return(0);
		}
		if (memchr(entry->path, 0, sizeof(entry->path)) == NULL) {
			_xlog("rgz.inspect : [%u] path is not NUL terminated\n", i);
			return(0);
		}
		if (strchr(entry->path, '/') != NULL) {
			_xlog("rgz.inspect : [%u] path must use \\ instead of / as directory separator\n", i);
			return(0);
		}
		if (entry->datalength > 0 && entry->type != 'f' ) {
			_xlog("rgz.inspect : [%u] expected no data\n", i);
			return(0);
		}
		if (entry->datalength > 0 && entry->data == NULL) {
			_xlog("rgz.inspect : [%u] expected non-NULL data\n", i);
			return(0);
		}
		if (entry->datalength == 0 && entry->data != NULL) {
			_xlog("rgz.inspect : [%u] expected NULL data\n", i);
			return(0);
		}
		if (entry->type == 'e')
			break;
	}
	if (i == rgz->entrycount) {
		_xlog("rgz.inspect : end entry is missing\n", i);
		return(0);
	}

	return(1); // valid
}


struct RORgz *rgz_load(struct _reader *reader) {
	struct RORgz *ret;
	unsigned int entrylimit;
	struct _reader *gzipreader;

	if (reader == NULL || reader->error) {
		_xlog("rgz.load : invalid argument (reader=%p reader.error=%d)\n", reader, reader->error);
		return(NULL);
	}

	gzipreader = deflatereader_init(reader, 1); // gzip only
	if (gzipreader->error) {
		_xlog("rgz.load : gzipreader init failed\n");
		gzipreader->destroy(gzipreader);
		return(NULL);
	}

	ret = (struct RORgz*)_xalloc(sizeof(struct RORgz));
	ret->entries = NULL;
	ret->entrycount = entrylimit = 0;
	for (;;) {
		struct RORgzEntry *entry;
		unsigned char pathlen;

		if (ret->entrycount == entrylimit) {
			struct RORgzEntry *tmp = ret->entries;
			entrylimit = entrylimit * 4 + 3;
			ret->entries = (struct RORgzEntry*)_xalloc(sizeof(struct RORgzEntry) * entrylimit);
			if (tmp != NULL) {
				memcpy(ret->entries, tmp, sizeof(struct RORgzEntry) * ret->entrycount);
				_xfree(tmp);
			}
		}
		entry = &ret->entries[ret->entrycount];
		ret->entrycount++;
		memset(entry, 0, sizeof(struct RORgzEntry));

		gzipreader->read(&entry->type, 1, 1, gzipreader);
		if (gzipreader->error)
			break;

		gzipreader->read(&pathlen, 1, 1, gzipreader);
		if (gzipreader->error)
			break;

		if (pathlen > 0) {
			gzipreader->read(&entry->path, 1, pathlen, gzipreader);
			if (gzipreader->error)
				break;
			entry->path[pathlen - 1] = 0;
		}

		if (entry->type == 'f') {
			gzipreader->read(&entry->datalength, 4, 1, gzipreader);
			if (gzipreader->error)
				break;

			if (entry->datalength > 0) {
				entry->data = (unsigned char*)_xalloc(sizeof(unsigned char) * entry->datalength);
				gzipreader->read(entry->data, 1, entry->datalength, gzipreader);
				if (gzipreader->error)
					break;
			}
		}
		else if (entry->type == 'd') {
		}
		else if (entry->type == 'e') {
			break; // ignore rest of data
		}
		else {
			_xlog("Unknown entry type '%c' (%s)\n", entry->type, entry->path);
			rgz_unload(ret);
			ret = NULL;
			break;
		}
	}
	if (ret != NULL && gzipreader->error) {
		_xlog("rgz.load : read error\n");
		rgz_unload(ret);
		ret = NULL;
	}
	if (ret != NULL && ret->entrycount != entrylimit) {
		struct RORgzEntry *tmp = ret->entries;
		ret->entries = (struct RORgzEntry*)_xalloc(sizeof(struct RORgzEntry) * ret->entrycount);
		memcpy(ret->entries, tmp, sizeof(struct RORgzEntry) * ret->entrycount);
		_xfree(tmp);
	}
	gzipreader->destroy(gzipreader);

	return(ret);
}


struct RORgz *rgz_loadFromData(const unsigned char *data, unsigned int length) {
	struct RORgz *ret;
	struct _reader *reader;

	reader = memreader_init(data, length);
	ret = rgz_load(reader);
	reader->destroy(reader);

	return(ret);
}


struct RORgz *rgz_loadFromFile(const char *fn) {
	struct RORgz *ret;
	struct _reader *reader;

	reader = filereader_init(fn);
	ret = rgz_load(reader);
	reader->destroy(reader);

	return(ret);
}


int rgz_save(const struct RORgz *rgz, struct _writer *writer) {
	struct _writer *gzipwriter;
	unsigned int i;
	unsigned char pathlen;

	if (rgz == NULL || writer == NULL || writer->error) {
		_xlog("rgz.save : invalid argument (rgz=%p writer=%p writer.error=%d)\n", rgz, writer, writer->error);
		return(1);
	}

	if (rgz_inspect(rgz) == 0) {
		_xlog("rgz.save : invalid\n");
		return(1);
	}

	gzipwriter = deflatewriter_init(writer, 1); // gzip
	if (gzipwriter->error) {
		_xlog("rgz.save : gzipwriter init failed\n");
		gzipwriter->destroy(gzipwriter);
		return(1);
	}

	for (i = 0; i < rgz->entrycount; i++) {
		const struct RORgzEntry *entry = &rgz->entries[i];
		gzipwriter->write(&entry->type, 1, 1, gzipwriter);
		pathlen = (unsigned char)strlen(entry->path) + 1;
		gzipwriter->write(&pathlen, 1, 1, gzipwriter);
		gzipwriter->write(entry->path, 1, pathlen, gzipwriter);
		if (entry->type == 'f') {
			gzipwriter->write(&entry->datalength, 4, 1, gzipwriter);
			if (entry->datalength > 0)
				gzipwriter->write(entry->data, 1, entry->datalength, gzipwriter);
		}
		else if (entry->type == 'e')
			break; // ignore the rest
	}
	gzipwriter->destroy(gzipwriter);

	return(writer->error);
}


int rgz_saveToData(const struct RORgz *rgz, unsigned char **data_out, unsigned long *size_out) {
	int ret;
	struct _writer *writer;

	writer = memwriter_init(data_out, size_out);
	ret = rgz_save(rgz, writer);
	writer->destroy(writer);

	return(ret);
}


int rgz_saveToFile(const struct RORgz *rgz, const char *fn) {
	int ret;
	struct _writer *writer;

	writer = filewriter_init(fn);
	ret = rgz_save(rgz, writer);
	writer->destroy(writer);

	return(ret);
}


void rgz_unload(struct RORgz* rgz) {
	unsigned int i;

	if (rgz == NULL)
		return;

	for (i = 0; i < rgz->entrycount; i++)
		if (rgz->entries[i].data != NULL)
			_xfree(rgz->entries[i].data);
	if (rgz->entries != NULL)
		_xfree(rgz->entries);
	_xfree(rgz);
}
