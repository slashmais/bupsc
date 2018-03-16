/*
    This file is part of utilfuncs-utility comprising utilfuncs.h and utilfuncs.cpp

    utilfuncs-utility - Copyright 2018 Marius Albertyn

    utilfuncs-utility is free software: you can redistribute
    it and/or modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation, either
    version 3 of the License, or (at your option) any later version.

    utilfuncs-utility is distributed in the hope that it will
    be useful, but WITHOUT ANY WARRANTY; without even the implied
    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with utilfuncs-utility in a file named COPYING.
    If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _utilfuncs_h_
#define _utilfuncs_h_

#include <string>
#include <sstream>
#include <vector>
#include <map>


//--------------------------------------------------------------------------------------------------
void PRINTSTRING(const std::string &s);
template<typename...P> void tu_p(std::stringstream &ss) { PRINTSTRING(ss.str()); }
template<typename H, typename...P> void tu_p(std::stringstream &ss, H h, P...p) { ss << h; tu_p(ss, p...); }
template<typename...P> void telluser(P...p) { std::stringstream ss(""); tu_p(ss, p...); }
#define TODO telluser("To Do: '", __func__, "', in ", __FILE__, " [", long(__LINE__), "]");
#define todo TODO
template<typename...P> bool tellerror(P...p) { std::stringstream ss(""); ss << "ERROR: "; tu_p(ss, p...); return false; }

//--------------------------------------------------------------------------------------------------
inline bool is_name_char(int c)	{ return (((c>='0')&&(c<='9')) || ((c>='A')&&(c<='Z')) || ((c>='a')&&(c<='z')) || (c=='_')); }

//-----------------------------------------------------------------------------
template<typename...P> void spf_p(std::stringstream &s) {}
template<typename H, typename...P> void spf_p(std::stringstream &ss, H h, P...p) { ss << h; spf_p(ss, p...); }
template<typename...P> std::string spf(P...p) { std::stringstream ss(""); spf_p(ss, p...); return ss.str(); }

//--------------------------------------------------------------------------------------------------
///todo: whitespace chars - beep, para.., backspace, etc
#define WHITESPACE_CHARS (const char*)" \t\n\r"
void LTRIM(std::string& s, const char *sch=WHITESPACE_CHARS);
void RTRIM(std::string& s, const char *sch=WHITESPACE_CHARS);
void TRIM(std::string& s, const char *sch=WHITESPACE_CHARS);
void ReplacePhrase(std::string &sTarget, const std::string &sPhrase, const std::string &sReplacement); //each occ of phrase with rep
void ReplaceChars(std::string &sTarget, const std::string &sChars, const std::string &sReplacement); //each occ of each char from chars with rep
const std::string SanitizeName(const std::string &sp); //replaces no-alphanums with underscores
//=====

const std::string ucase(const char *sz);
const std::string ucase(const std::string &s);
void toucase(std::string &s);
const std::string lcase(const char *sz);
const std::string lcase(const std::string &s);
void tolcase(std::string &s);

//int scmp(const std::string &s1, const std::string &s2);
//int sicmp(const std::string &s1, const std::string &s2);
bool seqs(const std::string &s1, const std::string &s2); //Strings case-sensitive EQuality-compare
bool sieqs(const std::string &s1, const std::string &s2); //Strings case-Insensitive EQuality-compare

bool hextoch(const std::string &shex, char &ch); //expect shex <- { "00", "01", ... , "fe", "ff" }

void fshex(const std::string &sraw, std::string &shex, int rawlen=0); //format rawlen(0=>all) chars to hex

const std::string esc_quotes(const std::string &s);
const std::string unesc_quotes(const std::string &s);

//--------------------------------------------------------------------------------------------------
const std::string to_sKMGT(size_t n); //1024, Kilo/Mega/Giga/Tera/Peta/Exa/Zetta/Yotta

//--------------------------------------------------------------------------------------------------filesystem
const std::string filesys_error(); //applies to bool path_..(), dir_..() and file_..() functions below

//path..
const std::string path_append(const std::string &spath, const std::string &sapp); // (/a/b/c, d) => /a/b/c/d
bool path_realize(const std::string &spath); //full path only, a file-name will create dir with that name!
const std::string path_path(const std::string &spath); //excludes last name (/a/b/c/d => /a/b/c)
const std::string path_name(const std::string &spath); //only last name (/a/b/c/d => d)
const std::string path_time(const std::string &spath); //modified date-time: return format=>"yyyymmddHHMMSS"
bool path_dev_space(const std::string spath, size_t &max, size_t &free); //space on device on which path exist

//utility..
bool isdirtype(int t);
bool isfiletype(int t); //regular file
bool fsexist(const std::string &sfs);
bool issubdir(const std::string &Sub, const std::string &Dir); //is S a subdir of D
bool isdirempty(const std::string &D); //also false if D does not exist, or cannot be read
bool getfulltree(const std::string &sdir, std::map<std::string, int> &tree); //full-path: directories & filenames, types

//dir..
bool dir_exist(const std::string &sdir);
inline bool isdir(const std::string &sdir) { return dir_exist(sdir); }
inline const std::string dir_path(const std::string &sdpath) { return path_path(sdpath); }
inline const std::string dir_name(const std::string &sdpath) { return path_name(sdpath); }
bool dir_create(const std::string &sdir);
bool dir_delete(const std::string &sdir);
bool dir_compare(const std::string &l, const std::string &r, bool bNamesOnly=false); //deep; bNamesOnly=>no file-sizes; true=>same
bool dir_read(const std::string &sdir, std::map<std::string, int> &md); //[name(not path)]=type
//bool dir_read_deep(const std::string &sdir, std::map<std::string, int> &md); --- todo ---needed? see getfulltree()
bool dir_rename(const std::string &sdir, const std::string &sname);
bool dir_copy(const std::string &sdir, const std::string &destdir, bool bBackup=false);
bool dir_move(const std::string &sdir, const std::string &destdir);
bool dir_backup(const std::string &sfile); //makes ".~nn~" copy of dir
size_t dir_size(const std::string sdir); //deep-size

//file..
bool file_exist(const std::string &sfile);
inline bool isfile(const std::string &sfile) { return file_exist(sfile); }
inline const std::string file_path(const std::string &sfpath) { return path_path(sfpath); }
inline const std::string file_name(const std::string &sfpath) { return path_name(sfpath); }
bool file_delete(const std::string &sfile);
bool file_read(const std::string &sfile, std::string &sdata);
bool file_compare(const std::string &l, const std::string &r); //true=>same
bool file_write(const std::string &sfile, const std::string &sdata);
bool file_append(const std::string &sfile, const std::string &sdata);
bool file_rename(const std::string &sfile, const std::string &sname);
bool file_copy(const std::string &sfile, const std::string &dest, bool bBackup=false);
bool file_move(const std::string &sfile, const std::string &dest);
bool file_backup(const std::string &sfile); //makes ".~nn~" copy of file
size_t file_size(const std::string sdir);


//--------------------------------------------------------------------------------------------------
template<typename T> bool is_numeric(T t)
{
	return (std::is_integral<decltype(t)>{}||std::is_floating_point<decltype(t)>{});
}

//NB: use absolute value of n...
template<typename N>const std::string tohex(N n, unsigned int leftpadtosize=1, char pad='0')
{
	if (!is_numeric(n)) return "\0";
	std::string H="", h="0123456789ABCDEF";
	uint64_t U=n;
	while (U) { H.insert(0,1,h[(U&0xf)]); U>>=4; }
	while (H.size()<leftpadtosize) H.insert(0, 1, pad);
	return H;
}

template<typename T> void ensv(const std::vector<T> &v, char delim, std::string &list, bool bIncEmpty=true) //EN-[delimiter-]Separated-Values
{
	list.clear();
	std::ostringstream oss("");
	for (auto t:v) { if (!oss.str().empty()) oss << delim; oss << t; }
	list=oss.str();
}

template<typename T> size_t desv(const std::string &list, char delim, std::vector<T> &v, bool bIncEmpty=true) //DE-[delimiter-]Separated-Values
{
	std::istringstream iss(list);
	std::string s;
	v.clear();
	auto stt=[](const std::string &str)->const T{ std::istringstream ss(str); T t; ss >> t; return t; };
	while (std::getline(iss, s, delim)) { TRIM(s); if (!s.empty()||bIncEmpty) v.push_back(stt(s)); }
	return v.size();
}

template<typename T> const std::string ttos(const T &v) //TypeTOString
{
    std::ostringstream ss;
    ss << v;
    return ss.str();
}

template<typename T> const T stot(const std::string &str) //StringTOType
{
    std::istringstream ss(str);
    T ret;
    ss >> ret;
    return ret;
}

#endif
