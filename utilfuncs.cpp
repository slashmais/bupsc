/*
    This file is part of utilfuncs comprising utilfuncs.h and utilfuncs.cpp

    utilfuncs - Copyright 2018 Marius Albertyn

    utilfuncs is free software: you can redistribute
    it and/or modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation, either
    version 3 of the License, or (at your option) any later version.

    utilfuncs is distributed in the hope that it will
    be useful, but WITHOUT ANY WARRANTY; without even the implied
    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with utilfuncs in a file named COPYING.
    If not, see <http://www.gnu.org/licenses/>.
*/

#include "utilfuncs.h"
#include <map>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <iomanip>
#include <system_error>
#include <chrono>
#include <ctime>
#include <sys/stat.h>


//#include <filesystem> //c++17!
//namespace FSYS=std::filesystem;
#include <experimental/filesystem> //NB: need to link with libstdc++fs
namespace FSYS=std::experimental::filesystem;


//--------------------------------------------------------------------------------------------------
void PRINTSTRING(const std::string &s) { std::cout << s << std::flush; }

//--------------------------------------------------------------------------------------------------
void LTRIM(std::string& s, const char *sch)
{
	std::string ss=sch;
	int i=0, n=s.length();
	while ((i<n) && (ss.find(s.at(i),0)!=std::string::npos)) i++;
	s = (i>0)?s.substr(i,n-i):s;
}

void RTRIM(std::string& s, const char *sch)
{
	std::string ss=sch;
	int n = s.length()-1;
	int i=n;
	while ((i>0) && (ss.find( s.at(i),0)!=std::string::npos)) i--;
	s = (i<n)?s.substr(0,i+1):s;
}

void TRIM(std::string& s, const char *sch) { LTRIM(s, sch); RTRIM(s, sch); }

void ReplacePhrase(std::string& S, const std::string& sPhrase, const std::string& sNew)
{ //replaces EACH occurrence of sPhrase with sNew
	size_t pos = 0, fpos;
	std::string s=S;
	while ((fpos = s.find(sPhrase, pos)) != std::string::npos)
	{
		s.replace(fpos, sPhrase.size(), sNew);
		pos = fpos + sNew.length();
	}
	S=s;
}

void ReplaceChars(std::string &s, const std::string &sCharList, const std::string &sReplacement)
{ //replaces EACH occurrence of EACH single char in sCharList with sReplacement
    std::string::size_type pos = 0;
    while ((pos=s.find_first_of(sCharList, pos))!= std::string::npos)
    {
        s.replace(pos, 1, sReplacement);
        pos += sReplacement.length();
    }
};

const std::string SanitizeName(const std::string &sp)
{
	std::string s=sp;
	ReplaceChars(s, " \\+-=~`!@#$%^&*()[]{}:;\"|'<>,.?/", "_");
	TRIM(s, "_");
	std::string t;
	do { t=s; ReplacePhrase(s, "__", "_"); } while (t.compare(s)!=0);
	return s;
}

const std::string ucase(const char *sz)
{
	std::string w(sz);
	for (auto &c:w) c=std::toupper(static_cast<unsigned char>(c));
	return w;
}

const std::string ucase(const std::string &s) { return ucase(s.c_str()); };

void toucase(std::string& s) { s=ucase(s.c_str()); }

const std::string lcase(const char *sz)
{
	std::string w(sz);
	for (auto &c:w) c=std::tolower(static_cast<unsigned char>(c));
	return w;
}

const std::string lcase(const std::string &s) { return lcase(s.c_str()); };

void tolcase(std::string &s) { s=lcase(s.c_str()); }


int scmp(const std::string &s1, const std::string &s2) { return s1.compare(s2); }
int sicmp(const std::string &s1, const std::string &s2)
{
	std::string t1=s1, t2=s2;
	return ucase(t1.c_str()).compare(ucase(t2.c_str()));
}
bool seqs(const std::string &s1, const std::string &s2) { return (scmp(s1,s2)==0); }
bool sieqs(const std::string &s1, const std::string &s2) { return (sicmp(s1,s2)==0); }

bool hextoch(const std::string &shex, char &ch) //shex <- { "00", "01", ... , "fe", "ff" }
{
	if (shex.length()==2)
	{
		std::string sh=shex;
        tolcase(sh);
		int n=0;
		if		((sh[0]>='0')&&(sh[0]<='9')) n=(16*(sh[0]-'0'));
		else if ((sh[0]>='a')&&(sh[0]<='f')) n=(16*(sh[0]-'a'+10));
		else return false;
		if		((sh[1]>='0')&&(sh[1]<='9')) n+=(sh[1]-'0');
		else if ((sh[1]>='a')&&(sh[1]<='f')) n+=(sh[1]-'a'+10);
		else return false;
		ch=char(n);
	}
	return true;
}

void fshex(const std::string &sraw, std::string &shex, int rawlen)
{
	//output: hex-values + tab + {char/.}'s, e.g.: "Hello" ->  "48 65 6C 6C 6F    hello"
	int i, l=((rawlen<=0)||(rawlen>(int)sraw.length()))?(int)sraw.length():rawlen;
	auto c2s=[](unsigned char u)->std::string
		{
			char b[3];
			char *p=b;
			if(u<0x80) *p++=(char)u;
			else if(u<=0xff) { *p++=(0xc0|(u>>6)); *p++=(0x80|(u&0x3f)); }
			return std::string(b, p-b);
		};
	shex.clear();
	for (i=0; i<l; i++) { shex+=tohex<unsigned char>(sraw[i], 2); shex+=" "; }
	//shex+="\t";
	shex+="    ";//simulate 4-space 'tab'
	unsigned char u;
	for (i=0; i<l; i++) { u=sraw[i]; if (((u>=32)&&(u<=126))||((u>159)&&(u<255))) shex+=c2s(u); else shex+='.'; }
}

const std::string esc_quotes(const std::string &s)
{
	std::stringstream ss;
	ss << std::quoted(s);
	return ss.str();
}

const std::string unesc_quotes(const std::string &s)
{
	std::stringstream ss(s);
	std::string r;
	ss >> std::quoted(r);
	return r;
}

//--------------------------------------------------------------------------------------------------
const std::string to_sKMGT(size_t n)
{
	std::string s("");
	int i=0;
	while (n>1024) { i++; n>>=10; }
	return spf(n, " ", "BKMGTPEZY"[i], (i>0)?"B":"");
}


//--------------------------------------------------------------------------------------------------
std::string FILESYS_ERROR;
inline void clear_fs_err() { FILESYS_ERROR.clear(); }
inline bool fs_err(const std::string &e) { FILESYS_ERROR=e; return false; }

const std::string filesys_error() { return FILESYS_ERROR; }

//------------------------------------------------
const std::string path_append(const std::string &sPath, const std::string &sApp)
{
	std::string s=(sPath.empty()?"/":sPath);
	s=spf(s, ((s[s.length()-1]=='/')?"":"/"), sApp.c_str());
	return s;
}

bool path_realize(const std::string &spath)
{
	if (dir_exist(spath)) return true;
	clear_fs_err();
	std::error_code ec;
	if (FSYS::create_directories(spath, ec)) return true;
	else return fs_err(spf("path_realize: '", spath, "' - ", ec.message()));

//retain below code until sure above works as it should !
//	if (dir_exist(spath)) return true;
//	clear_fs_err();
//	std::vector<std::string> v;
//	if (desv(spath, '/', v, false)>0)
//	{
//		std::string p("/");
//		bool b=true;
//		auto it=v.begin();
//		while (b&&(it!=v.end())) { p=path_append(p, *it++); if (!dir_exist(p)) b=dir_create(p); }
//		if (!b) return fs_err(spf("path_realize: '", spath, "' - ", filesys_error()));
//	}
//	else return fs_err(spf("path_realize: '", spath, "' - invalid path"));
//	return true;

}
	
const std::string path_path(const std::string &spath)
{
	std::string r(spath);
	TRIM(r, "/");
	size_t p=r.rfind('/');
	if (p!=std::string::npos) r=r.substr(0, p);
	return r;
}

const std::string path_name(const std::string &spath)
{
	std::string r(spath);
	TRIM(r, "/");
	size_t p=r.rfind('/');
	if ((p!=std::string::npos)&&(p<r.size())) r=r.substr(p+1);
	return r;
}

const std::string path_time(const std::string &spath)
{
	std::string sdt("");
	std::error_code ec;
	if (FSYS::exists(spath, ec))
	{
		auto ftime = FSYS::last_write_time(spath);
		std::time_t cftime=decltype(ftime)::clock::to_time_t(ftime);
		std::tm *pdt=localtime(&cftime);
		sdt=spf((pdt->tm_year+1900),
				(pdt->tm_mon<9)?"0":"", (pdt->tm_mon+1),
				(pdt->tm_mday<10)?"0":"", (pdt->tm_mday),
				(pdt->tm_hour<10)?"0":"", (pdt->tm_hour),
				(pdt->tm_min<10)?"0":"", (pdt->tm_min),
				(pdt->tm_sec<10)?"0":"", (pdt->tm_sec));
	}
	return sdt; //packaged as yyyymmddHHMMSS
}

bool path_dev_space(const std::string spath, size_t &max, size_t &free)
{
	max=free=0;
	std::error_code ec;
	if (FSYS::exists(spath, ec))
	{
		try { FSYS::space_info si=FSYS::space(spath); max=si.capacity; free=si.free; return true; }
		catch(FSYS::filesystem_error e) { return fs_err(spf("path_space: '", spath, "' - ", e.what())); }
	}
	return fs_err(spf("path_space: invalid path '", spath, "'"));
}


//------------------------------------------------
bool isdirtype(int t) { return ((FSYS::file_type)t==FSYS::file_type::directory); }

bool isfiletype(int t) { return ((FSYS::file_type)t==FSYS::file_type::regular); }

bool fsexist(const std::string &sfs)
{
	clear_fs_err();
	std::error_code ec;
	if (FSYS::exists(sfs, ec)) return true;
	return fs_err(spf("fsexist: '", sfs, "' - ", ec.message()));
}

bool issubdir(const std::string &Sub, const std::string &Dir)
{
	if (Dir.size()&&(Dir.size()<Sub.size())) return seqs(Sub.substr(0, Dir.size()), Dir);
	return false;
}

bool isdirempty(const std::string &D)
{
	if (!dir_exist(D)) return false; //??? non-exist should imply empty ???
	std::map<std::string, int> md;
	if (dir_read(D, md)) return md.empty();
	return false;
}

bool getfulltree(const std::string &sdir, std::map<std::string, int> &tree)
{
	//tree.clear()-DON'T!: NB used recursively - must be prep'ed/cleared by caller
	std::map<std::string, int> md;
	bool b=true;
	if ((b=dir_read(sdir, md)))
	{
		if (md.size()>0)
		{
			for (auto p:md)
			{
				std::string s=path_append(sdir, p.first);
				tree[s]=p.second;
				if (isdirtype(p.second)) b=getfulltree(s, tree);
			}
		}
	}
	return b;
}


//------------------------------------------------
bool dir_exist(const std::string &sdir)
{
	std::error_code ec;
	if (FSYS::exists(sdir, ec)) { if (FSYS::is_directory(sdir)) return true; return false; }
	return fs_err(spf("dir_exist: '", sdir, "' - ", ec.message()));
}

bool dir_create(const std::string &sdir)
{
	clear_fs_err();
	std::error_code ec;
	if (dir_exist(sdir)) return true;
	if (FSYS::create_directory(sdir, ec)) return true;
	return fs_err(spf("dir_create: '", sdir, "' - ", ec.message()));
}

bool dir_delete(const std::string &sdir)
{
	clear_fs_err();
	std::error_code ec;
	FSYS::remove_all(sdir, ec);
	if (ec) return fs_err(spf("dir_delete: '", sdir, "' - ", ec.message()));
	return !dir_exist(sdir);
	
}

bool dir_compare(const std::string &l, const std::string &r, bool bNamesOnly)
{
	clear_fs_err();
	if (!dir_exist(l)) return fs_err(spf("dir_compare: l-directory '", l, "' dtryoes not exist"));
	if (!dir_exist(r)) return fs_err(spf("dir_compare: r-directory '", r, "' does not exist"));
	bool b;
	std::map<std::string, int> mc;
	std::map<std::string, int> m;
	auto all2=[&mc]()->bool{ if (mc.size()>0) { for (auto p:mc) { if (p.second!=2) return false; }} return true; };
	mc.clear();
	m.clear();
	if ((b=dir_read(l, m)))
	{
		if (m.size()>0) for (auto p:m) mc[p.first]=1;
		m.clear();
		if ((b=dir_read(r, m)))
		{
			if (m.size()>0) for (auto p:m) mc[p.first]++;
			if ((b=all2()))
			{
				auto it=m.begin();
				while (b&&(it!=m.end()))
				{
					if ((FSYS::file_type)it->second==FSYS::file_type::directory)
						{ b=dir_compare(path_append(l, it->first), path_append(r, it->first)); }
					else if (!bNamesOnly&&((FSYS::file_type)it->second==FSYS::file_type::regular))
						{ b=file_compare(path_append(l, it->first), path_append(r, it->first)); }
						//if !b then stuff into some diff's list... means not bail-out when b==false...
					it++;
				}
			}
		}
	}
	return b;
}

bool dir_read(const std::string &sdir, std::map<std::string, int> &md)
{
	clear_fs_err();
	md.clear();
	if (!dir_exist(sdir)) return fs_err(spf("dir_read: directory '", sdir, "' does not exist"));
	try
	{
		auto DI=FSYS::directory_iterator(sdir);
		auto it=begin(DI);
		for (; it!=end(DI); ++it) { md[path_name(it->path())]=(int)(FSYS::status(it->path()).type()); }
	}
	catch(FSYS::filesystem_error e) { return fs_err(spf("dir_read: '", sdir, "' - ", e.what())); }
	return true;
}

//bool dir_read_deep(const std::string &sdir, std::map<std::string, int> &md) --- todo ---needed? see getfulltree()
//{
//	clear_fs_err();
//	md.clear();
//	if (!dir_exist(sdir)) return fs_err(spf("dir_read: directory '", sdir, "' does not exist"));
//	try
//	{
//		auto DID=FSYS::recursive_directory_iterator(sdir);
//		auto it=begin(DID);
//		for (; it!=end(DID); ++it) md[path_name(it->path())]=(int)(FSYS::status(it->path()).type());
//	}
//	catch(FSYS::filesystem_error e) { return fs_err(spf("dir_read: '", sdir, "' - ", e.what())); }
//	return true;
//}

bool dir_rename(const std::string &sdir, const std::string &sname)
{
	clear_fs_err();
	std::error_code ec;
	if (!dir_exist(sdir)) return fs_err(spf("dir_rename: directory '", sdir, "' does not exist"));
	std::string snd=path_append(path_path(sdir), sname);
	if (dir_exist(snd))  return fs_err(spf("dir_rename: name '", sname, "' already exist"));
	FSYS::rename(sdir, snd, ec);
	if (ec) return fs_err(spf("dir_rename: '", sdir, "' to '", snd, "' - ", ec.message()));
	return (!dir_exist(sdir)&&dir_exist(snd));
}

bool dir_copy(const std::string &sdir, const std::string &destdir, bool bBackup)
{
	clear_fs_err();
	std::error_code ec;
	if (!dir_exist(sdir)) return fs_err(spf("dir_copy: source directory '", sdir, "' does not exist"));
	if (!dir_exist(destdir)) return fs_err(spf("dir_copy: target directory '", destdir, "' does not exist"));
	std::string sd=path_append(destdir, path_name(sdir));
	if (bBackup&&dir_exist(sd)) { if (!dir_backup(sd)) return fs_err(spf("dir_copy: failed to backup '", sd, "'")); }
	FSYS::copy(sdir, sd, FSYS::copy_options::recursive|FSYS::copy_options::copy_symlinks, ec);
	if (ec) return fs_err(spf("dir_copy: '", sdir, "' to '", sd, "' - ", ec.message()));
	return dir_exist(sd);
}

bool dir_move(const std::string &sdir, const std::string &destdir)
{
	clear_fs_err();
	std::error_code ec;
	if (!dir_exist(sdir)) return fs_err(spf("dir_move: source directory '", sdir, "' does not exist"));
	if (!dir_exist(destdir)) return fs_err(spf("dir_move: target directory '", destdir, "' does not exist"));
	std::string sd=path_append(destdir, path_name(sdir));
	if (dir_exist(sd)) return fs_err(spf("dir_move: target '", sd, "' already exist"));
	FSYS::rename(sdir, sd, ec);
	if (ec) return fs_err(spf("dir_move: '", sdir, "' to '", sd, "' - ", ec.message()));
	return (!dir_exist(sdir)&&dir_exist(sd));
}

bool dir_backup(const std::string &sdir)
{
	if (dir_exist(sdir))
	{
		int i=1;
		std::string sd=sdir;
		while (dir_exist(sd)) { sd=spf(sdir, ".~", i, '~'); i++; }
		return dir_copy(sdir, sd, false);
	}
	return false;
}

size_t dir_size(const std::string sdir)
{
	size_t used=0;
	if (!dir_exist(sdir)) { fs_err(spf("dir_size: invalid directory '", sdir, "'")); }
	else
	{
		std::map<std::string, int> tree;
		if (getfulltree(sdir, tree))
		{
			if (tree.size()>0) { for (auto p:tree) { struct stat st; used+=(!stat(p.first.c_str(), &st))?st.st_size:0; }}
		}
	}
	return used;
}


//------------------------------------------------
bool file_exist(const std::string &sfile) //true iff existing reg-file
{
	clear_fs_err();
	std::error_code ec;
	if (FSYS::exists(sfile, ec)) { if (FSYS::is_regular_file(sfile, ec)) return true; return false; }
	if (ec) return fs_err(spf("file_exist: '", sfile, "' - ", ec.message()));
	return false;
}

bool priv_is_file(const std::string &sfile) //true if reg-file or does not exist
{
	clear_fs_err();
	std::error_code ec;
	if (FSYS::exists(sfile, ec)) { if (FSYS::is_regular_file(sfile, ec)) return true; return false; }
	if (ec) return fs_err(spf("file_exist: '", sfile, "' - ", ec.message()));
	return true;
}

bool file_delete(const std::string &sfile)
{
	clear_fs_err();
	std::error_code ec;
	if (!priv_is_file(sfile)) return fs_err(spf("file_delete: '", sfile, "' is not a file"));
	if (file_exist(sfile))
	{
		FSYS::remove(sfile, ec);
		if (ec) return fs_err(spf("file_delete: '", sfile, "' - ", ec.message()));
	}
	return true;
}

bool file_read(const std::string &sfile, std::string &sdata)
{
	clear_fs_err();
	sdata.clear();
	std::ifstream ifs(sfile.c_str());
	if (ifs.good()) { std::stringstream ss; ss << ifs.rdbuf(); sdata=ss.str(); return true; }
	return fs_err(spf("file_read: cannot read file '", sfile, "'"));
}

bool file_compare(const std::string &l, const std::string &r)
{
	if (file_exist(l)&&file_exist(r))
	{
		std::string sl, sr;
		if (file_read(l, sl)&&file_read(r, sr)) return seqs(sl, sr);
	}
	return false;
}

bool file_write(const std::string &sfile, const std::string &sdata)
{
	clear_fs_err();
	std::ofstream ofs(sfile);
	if (ofs.good()) { ofs << sdata; return true; }
	return fs_err(spf("file_read: cannot write file '", sfile, "'"));
}

bool file_append(const std::string &sfile, const std::string &sdata)
{
	clear_fs_err();
	std::ofstream ofs(sfile, std::ios_base::app);
	if (ofs.good()) { ofs << sdata; return true; }
	return fs_err(spf("file_read: cannot write file '", sfile, "'"));
}

bool file_rename(const std::string &sfile, const std::string &sname)
{
	clear_fs_err();
	std::error_code ec;
	if (!file_exist(sfile)) return fs_err(spf("file_rename: old file '", sfile, "' does not exist"));
	std::string sd=path_append(path_path(sfile), sname);
	if (file_exist(sd)) return fs_err(spf("file_rename: name '", sname, "' already exist"));
	FSYS::rename(sfile, sd, ec);
	if (ec) return fs_err(spf("file_rename: '", sfile, "' to '", sd, "' - ", ec.message()));
	return (!file_exist(sfile)&&file_exist(sd));
}

bool file_copy(const std::string &sfile, const std::string &dest, bool bBackup)
{
	clear_fs_err();
	std::error_code ec;
	std::string sd=dest;
	if (!file_exist(sfile)) return fs_err(spf("file_copy: source file '", sfile, "' does not exist"));
	if (isdir(sd))
	{
		if (!dir_exist(sd)) return fs_err(spf("file_copy: target directory '", sd, "' does not exist"));
		sd=path_append(sd, path_name(sfile));
	}
	if (bBackup&&file_exist(sd)) { if (!file_backup(sd)) return fs_err(spf("file_copy: failed to backup '", sd, "'")); }
	FSYS::copy_file(sfile, sd, FSYS::copy_options::overwrite_existing, ec);
	if (ec) return fs_err(spf("file_copy: '", sfile, "' to '", sd, "' - ", ec.message()));
	return file_exist(sd);
}

bool file_move(const std::string &sfile, const std::string &dest)
{
	clear_fs_err();
	std::error_code ec;
	std::string sd=dest;
	if (!file_exist(sfile)) return fs_err(spf("file_move: source file '", sfile, "' does not exist"));
	if (isdir(sd))
	{
		if (!dir_exist(sd)) return fs_err(spf("file_move: target directory '", sd, "' does not exist"));
		std::string sd=path_append(sd, path_name(sfile));
	}
	if (file_exist(sd)) return fs_err(spf("file_move: file '", sd, "' already exist"));
	FSYS::rename(sfile, sd, ec);
	if (ec) return fs_err(spf("file_move: '", sfile, "' to '", sd, "' - ", ec.message()));
	return (!file_exist(sfile)&&file_exist(sd));
}

bool file_backup(const std::string &sfile)
{
	if (file_exist(sfile))
	{
		clear_fs_err();
		std::error_code ec;
		int i=1;
		std::string sf=sfile;
		while (file_exist(sf)) { sf=spf(sfile, ".~", i, '~'); i++; }
		FSYS::copy_file(sfile, sf, ec);
		if (ec) return fs_err(spf("file_backup: '", sfile, "' to '", sf, "' - ", ec.message()));
		return file_exist(sf);
	}
	return false;
}

size_t file_size(const std::string sfile)
{
	size_t n=0;
	if (fsexist(sfile))
	{
		clear_fs_err();
		try { n=(size_t)FSYS::file_size(sfile); }
		catch(FSYS::filesystem_error e) { n=0; fs_err(spf("file_size: '", sfile, "' - ", e.what())); }
	}
	else { fs_err(spf("file_size: '", sfile, "' - does not exist")); }
	return n;
}

