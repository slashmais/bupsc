/*
    This file is part of the bupsc application.

    bupsc - Copyright 2018 Marius Albertyn

    bupsc is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    bupsc is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with bupsc in a file named COPYING.
    If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _bscglobals_h_
#define _bscglobals_h_

#include <string>
#include <cstdint>
#include <fstream>
#include <sstream>

extern std::string LOG_FILE;
extern bool bVERBOSE_LOG;
extern bool bDEBUG_LOG;

const std::string Version();

typedef uint64_t DTStamp;

const std::string DateTime(bool bmillisec=false);

DTStamp dt_stamp();
DTStamp ToDTStamp(const std::string &yyyymmddHHMMSS); //HHMMSS-optional
const std::string ToDateStr(DTStamp dts);

void write_log(const std::string &sData); //used by LogLog(..)
bool report_error(const std::string &serr);

bool wildcard_match(const std::string &wcs, const std::string &sval);

template<typename...P> void w_t_f(std::ostream &os) {}
template<typename H, typename...P> void w_t_f(std::ostream &os, H h, P...p) { os << h; w_t_f(os, p...); }
template<typename...P> void write_to_file(const std::string sfile, P...p) { std::ofstream ofs(sfile, std::ios_base::app); w_t_f(ofs, p...); }
template<typename...P> void LogLog(P...p) { std::ostringstream oss(""); w_t_f(oss, p...); write_log(oss.str()); }
template<typename...P> void LogDebug(P...p) { if (bDEBUG_LOG) write_to_file(LOG_FILE+".DEBUG", p..., "\n"); }
template<typename...P> void LogVerbose(P...p) { if (bVERBOSE_LOG) write_to_file(LOG_FILE+".VERBOSE", p..., "\n"); }


#endif
