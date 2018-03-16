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

#ifndef _compression_h_
#define _compression_h_

#include <string>

bool compress(const std::string &R, std::string &C); //string to string
bool decompress(const std::string &C, std::string &R);

bool compress_file(const std::string &rfile, std::string &cfile); //file to file
bool decompress_file(const std::string &cfile, std::string &rfile);


#endif
