/*
    This file is part of compression-utility comprising compression.h and compression.cpp

    compression-utility - Copyright 2018 Marius Albertyn

    compression-utility is free software: you can redistribute
    it and/or modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation, either
    version 3 of the License, or (at your option) any later version.

    compression-utility is distributed in the hope that it will
    be useful, but WITHOUT ANY WARRANTY; without even the implied
    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with compression-utility in a file named COPYING.
    If not, see <http://www.gnu.org/licenses/>.
*/

#include "compression.h"
#include "utilfuncs.h"
#include <fstream>
#include <cstring>
#include <zlib.h> //NB: must add libz

//--------------------------------------------------------------------------------------------------
//credit for idea: https://panthema.net/2007/0328-ZLibString.html
//--------------------------------------------------------------------------------------------------

const int ZBUFSIZE=16384;

bool compress(const std::string &R, std::string &C)
{
	int r;
	z_stream Z;
	std::memset(&Z, 0, sizeof(Z));
	if ((r=deflateInit(&Z, Z_BEST_COMPRESSION))!=Z_OK) { return tellerror("zlib init failed : [", r, "] ", Z.msg ,"\n"); }
	Z.next_in=(Bytef*)R.data();
	Z.avail_in=R.size();
	char cbuf[ZBUFSIZE];
	C.clear();
	do
	{
		Z.next_out=(Bytef*)cbuf; Z.avail_out=ZBUFSIZE; r=deflate(&Z, Z_FINISH);
		if (C.size()<Z.total_out) { C.append(cbuf, Z.total_out-C.size()); }
	} while (r==Z_OK);
	deflateEnd(&Z);
	if (r!=Z_STREAM_END) { return tellerror("compress(): [", r, "] ", Z.msg ,"\n"); }
	return true;
}

bool decompress(const std::string &C, std::string &R)
{
	int r;
	z_stream Z;
	std::memset(&Z, 0, sizeof(Z));
    if ((r=inflateInit(&Z))!=Z_OK) { return tellerror("zlib init failed: [", r, "] ", Z.msg ,"\n"); }
	Z.next_in=(Bytef*)C.data();
	Z.avail_in=C.size();
	char rbuf[ZBUFSIZE];
	R.clear();
	do
	{
		Z.next_out=(Bytef*)rbuf; Z.avail_out=ZBUFSIZE; r=inflate(&Z, 0);
		if (R.size()<Z.total_out) { R.append(rbuf, Z.total_out-R.size()); }
	
	} while (r==Z_OK);
	inflateEnd(&Z);
	if (r!=Z_STREAM_END) { return tellerror("decompress(): [", r, "] ", Z.msg ,"\n"); }
	return true;
}

bool compress_file(const std::string &rfile, std::string &cfile)
{
	std::string r, c;
	if (!file_read(rfile, r)) return tellerror("compress_file: Cannot read '", rfile, "'\n");
	if (!compress(r, c)) return tellerror("compress_file: compression failed\n");
	if (!file_write(cfile, c)) return tellerror("compress_file: Cannot write '", cfile, "'\n");
	return true;
}

bool decompress_file(const std::string &cfile, std::string &rfile)
{
	std::string r, c;
	if (!file_read(cfile, c)) return tellerror("decompress_file: Cannot read '", cfile, "'\n");
	if (!decompress(c, r)) return tellerror("decompress_file: decompression failed\n");
	if (!file_write(rfile, r)) return tellerror("decompress_file: Cannot write '", rfile, "'\n");
	return true;
}
	
