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

#include "monitor.h"
#include "dbmaster.h"
#include "utilfuncs.h"
#include "compression.h"
#include <unistd.h>
#include <cerrno>
#include <functional>
#include <sys/inotify.h>
#include <mutex>

extern bool bVERBOSE_LOG;

bool bStopMonitoring;
unsigned int monitor_count=0;
std::mutex MUX_monitor_count;
bool IsMonitoring() { return (monitor_count>0); }

Monitor::Monitor()						{ clear(); }

//Monitor::Monitor(const Monitor &B)		{ *this=B; }
Monitor::Monitor(const Monitor &B)
{
	fdInotify=B.fdInotify;
	id=B.id;
	sourcedir=B.sourcedir;
	direxcludes=B.direxcludes;
	fileexcludes=B.fileexcludes;
	bupdirs=B.bupdirs;
	bIsExtracting=B.bIsExtracting;
}

Monitor::~Monitor()						{ clear(); }

bool Monitor::is_excluded_dir(const std::string &sdir) { return wildcard_match(direxcludes, sdir); }

bool Monitor::is_excluded_file(const std::string &sfile) { return wildcard_match(fileexcludes, sfile); }

bool Monitor::sync_compress(DTStamp dts, const std::string &F, const std::string &T, const std::map<std::string, int> &md)
{
	bool b=true;
	if (!md.empty())
	{
		std::string sf, st, sz;
		auto it=md.begin();
		while (b&&(it!=md.end()))
		{
			sf=path_append(F, it->first);
			st=path_append(T, it->first);
			if (isdirtype(it->second)&&!is_excluded_dir(sf))
			{
				std::map<std::string, int> me;
				if (dir_read(sf, me)) { if ((b=dir_create(st))) { b=sync_compress(dts, sf, st, me); }}
				else report_error(spf("cannot sync '", sf, "' -", filesys_error())); //and carry-on
			}
			if (isfiletype(it->second)&&!is_excluded_file(sf))
			{
				if (file_exist(st)&&!file_compare(sf, st))
				{
					sz=path_append(file_path(st), spf("DTS", dts, "_", file_name(st), ".z"));
					compress_file(st, sz);
				}
				b=file_copy(sf, st);
			}
			//ignore if not directory or regular file (=> device/pipe/link/socket) ..
			// or put handlers here if needed..
			it++;
		}
	}
	return b;
}

bool Monitor::sync_compress(const std::string &F, const std::string &T)
{
	if (!path_realize(T)) return false;
	DTStamp dts=dt_stamp();
	std::map<std::string, int> md;
	dir_read(F, md);
	return sync_compress(dts, F, T, md);
}

bool Monitor::sync_bupdir(const std::string &sbupdir)
{
	DTStamp dts=dt_stamp();
	std::map<std::string, int> md;
	dir_read(sourcedir, md);
	return sync_compress(dts, sourcedir, sbupdir, md);
}

/* is this needed?
Monitor& Monitor::operator=(const Monitor &B)
{
	fdInotify=B.fdInotify;
	id=B.id;
	sourcedir=B.sourcedir;
	direxcludes=B.direxcludes;
	fileexcludes=B.fileexcludes;
	bupdirs=B.bupdirs;
	bIsExtracting=B.bIsExtracting;
	return *this;
}
*/

bool Monitor::Save() { return DBM.Save(*this); }

bool Monitor::AddBupdir(const std::string &sdir)
{
	if (HasBupdir(sdir)) return sync_bupdir(sdir);
	if (dir_exist(sdir)&&sync_bupdir(sdir))
	{
		bupdirs[sdir]=dt_stamp();
		return Save();
	}
	return false;
}

bool Monitor::HasBupdir(const std::string &sdir)
{
	if (bupdirs.size()>0)
	{
		auto it=bupdirs.begin();
		while (it!=bupdirs.end()) { if (seqs(it->first, sdir)) return true; else it++; }
	}
	return false;
}

bool Monitor::RemoveBupdir(const std::string &sdir)
{
	if (HasBupdir(sdir)) { bupdirs.erase(sdir); return Save(); }
	return true;
}

bool Monitor::DoBackup(const std::string &sfile)
{

  LogDebug("DoBackup sfile='", sfile, "'");

	if (!isfile(sfile)||is_excluded_file(sfile)) return false; //backup only regular files
	DTStamp dts=dt_stamp();
	std::string st, sz;
	bool b=true;

  LogDebug("DoBackup dts='", dts, "'");

	if (bupdirs.size()>0)
	{
		auto it=bupdirs.begin();
		while (b&&(it!=bupdirs.end()))
		{
			st=path_append(it->first, sfile.substr(sourcedir.size()+1));

  LogDebug("DoBackup st='", st, "'");

			if (file_exist(st)&&!file_compare(sfile, st))
			{
				sz=path_append(file_path(st), spf("DTS", dts, "_", file_name(st), ".z"));
				if (file_exist(sz)) file_backup(sz);
				compress_file(st, sz);
			}
			b=file_copy(sfile, st);
			it++;
		}
	}
	if (!b) report_error(spf("cannot backup '", sfile, "' to '", st, "' - ", filesys_error()));
	return b;
}

//needs attention...
bool Monitor::ExtractBackup(const std::string &sDest, DTStamp xdts)
{
	SetExtracting();
	
	auto getdts=[](const std::string sf)->DTStamp
		{
			DTStamp dts=0;
			if (seqs(sf.substr(0,3), "DTS")) { size_t p; if ((p=sf.find('_'))!=std::string::npos) dts=stot<DTStamp>(sf.substr(3, p-3)); }
			return dts;
		};

	auto getfn=[](const std::string &S)->const std::string
		{
			std::string r(S);
			if (!r.substr(0,3).compare("DTS")&&!r.substr(r.size()-2).compare(".z"))
			{
				size_t p;
				if ((p=r.find('_'))!=std::string::npos) { p++; r=r.substr(p, (r.size()-p-2)); }
			}
			return r;
		};

	std::string sbdir; //always use oldest bupdir...(most backups)
	if (bupdirs.size()>0)
	{
		std::map<DTStamp, std::string> dtbup; //sort by dtstamp..
		dtbup.clear();
		for (auto p:bupdirs) dtbup[p.second]=p.first;
		sbdir=dtbup.begin()->second; //smallest==oldest
	}
	else { SetExtracting(false); return report_error(spf("failed to extract: no defined backups for '", sourcedir, "'")); }
		
	std::map<std::string, int> tree, xtree;
	tree.clear(); xtree.clear();
	if (!getfulltree(sbdir, tree)) { SetExtracting(false); return report_error(spf("failed to get backup-tree for '", sbdir, "' - ", filesys_error())); }
	
	if (tree.size()==0) { SetExtracting(false); return report_error(spf("nothing to extract for '", sourcedir, "'")); }

	typedef struct { DTStamp d; std::string s; int t; } D_S_T;
	DTStamp dts, cdts, bdts; //c=create-, b=backup-date
	std::map<std::string, D_S_T > files;
	files.clear();
	for (auto p:tree)
	{
		std::string fn=path_name(p.first); //can be filename.ext or DTS<dtstamp_>filename.ext.z
		std::string subfp=p.first.substr(sbdir.size()+1); //get shared portion of path
		std::string sf=getfn(fn); //filename.ext
		cdts=ToDTStamp(path_time(p.first));
		bdts=getdts(fn);
		
		if ((isdirtype(p.second)&&((xdts==0)||(cdts<=xdts)))||((xdts==0)&&!(sieqs(fn.substr(0,3), "dts")&&seqs(fn.substr(fn.size()-2), ".z"))))
		{
			xtree[subfp]=p.second;
		}
		else if (!isdirtype(p.second)&&(xdts!=0)) //ignore otherwise
		{
			dts=(bdts&&(bdts<=xdts))?bdts:(cdts<=xdts)?cdts:0; //compressed=>check bdts<=>xdts, otherwise check create-date
			if (dts!=0) //ignore if younger(later than!) than xdts
			{
				auto pp=files.find(fn);
				if (pp!=files.end()) { if (pp->second.d<dts) { files[fn].d=dts; files[fn].s=subfp; files[fn].t=p.second; }}
				else { files[fn].d=dts; files[fn].s=subfp; files[fn].t=p.second; }
			}
		}
	}

	if (!files.empty()) { for (auto p:files) xtree[p.second.s]=p.second.t; }
	if (!xtree.empty())
	{
		if (!dir_exist(sDest)) if (!path_realize(sDest)) { SetExtracting(false); return report_error(spf("destination '", sDest, "' is inaccessible")); }
		bool b=true;
		std::string sf("");
		std::string st("");
		
		auto destpath=[](const std::string &S)->const std::string
			{
				std::string s=path_path(S);
				std::string r=path_name(S);
				if (seqs(r.substr(0,3), "DTS")&&seqs(r.substr(r.size()-2), ".z"))
				{
					size_t p;
					if ((p=r.find('_', 3))!=std::string::npos) { r=S.substr(p+1); r=S.substr(0, (r.size()-2)); }
				}
				s=path_append(s, r);
				return s;
			};
		
		for (auto p:xtree)
		{
			sf=path_append(sbdir, p.first);
			st=path_append(sDest, destpath(p.first));
			if (isdirtype(p.second)) path_realize(st);
			else
			{
				if (seqs(sf.substr(sf.size()-2), ".z")) { if (file_exist(st)) file_backup(st); if (!(b=decompress_file(sf, st))) break; }
				else if (!(b=file_copy(sf, st, true))) break;
			}
		}
		if (!b) { SetExtracting(false); return report_error(spf("failed to extract '", sf, "' to '", st, "'")); }
	}
	else report_error(spf("nothing to extract for '", sourcedir, "', '", sDest, "', '", xdts, "'"));
	
	SetExtracting(false);
	return true;
}

//start & stop called from monitors...
bool Monitor::Start() { if (!this_monitor.joinable()) { this_monitor=std::thread(do_monitor, this); } return this_monitor.joinable(); }
bool Monitor::Stop() { if (this_monitor.joinable()) this_monitor.join(); return true; }


//--------------------------------------------------------------------------------------------------
#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

std::map<int, std::pair<Monitor*, std::string> > BSC_WatchList;

void init_bsc_watches() { BSC_WatchList.clear(); }

void scansubs(Monitor *pM, std::string &sdir, std::function<bool(Monitor*, std::string&)> f) //add sub-dirs to watchlist...
{
	auto get_subdirs=[&](std::string &sdir, std::vector<std::string> &v)->bool
		{
			std::map<std::string, int> md;
			v.clear();
			if (dir_read(sdir, md))
			{
				if (!md.empty()) for (auto p:md) { if (isdirtype(p.second)) v.push_back(path_append(sdir, p.first)); }
			}
			return !v.empty();
		};
	std::vector<std::string> v;
	if (get_subdirs(sdir, v)) { for (auto d:v) { if (f(pM, d)) scansubs(pM, d, f); }}
}

void Monitor::do_monitor(Monitor *pMon)
{
	uint32_t MONITOR_MASK=(IN_MODIFY|IN_CREATE|IN_MOVED_TO|IN_MOVE_SELF|IN_DONT_FOLLOW);
	int wd;
	int fdI=pMon->fdInotify;


  LogDebug("starting threads for '", pMon->sourcedir, "'");
							

	auto iswatch=[&](std::string &sdir)->bool
	{
		auto it=BSC_WatchList.begin();
		while (it!=BSC_WatchList.end()) { if (seqs(it->second.second, sdir)) return true; it++; }
		return false;
	};
	
	auto inawerr=[&](int en, const std::string sd)->bool
		{
			std::string serr;
			serr=spf("\n\t>\tinotify_add_watch '", sd, "'");
			switch(en)
			{
				case EACCES:	serr+=":\n\t>\tRead access is not permitted."; break;
				case ENOENT:	serr+=":\n\t>\tA directory component in pathname does not exist or is a dangling symbolic link."; break;
				case EBADF:		serr+=":\n\t>\tThe given file descriptor is not valid."; break;
				case EFAULT:	serr+=":\n\t>\tpathname points outside of the process's accessible address space."; break;
				case EINVAL:	serr+=":\n\t>\tThe given event mask contains no legal events; or fd is not an inotify file descriptor."; break;
				case ENOMEM:	serr+=":\n\t>\tInsufficient kernel memory was available."; break;
				case ENOSPC:	serr+=":\n\t>\tThe user limit on inotify watches was reached or the kernel failed to allocate a resource."; break;
				default:		serr+=":\n\t>\tUnknown error"; break;
			}
			if ((errno==ENOMEM)||(errno==ENOSPC)) serr+="\n\t>\t(In case of a limit- or memory-related error, inspect:"
														"\n\t>\t\tsudo sysctl fs.inotify.max_user_watches\n\t>\tand"
														"\n\t>\t\tsudo sysctl fs.inotify.max_user_instances"
														"\n\t>\tand adjust the values.)";
			return report_error(serr);
		};

	auto watchit=[&](Monitor *pM, std::string &sdir)->bool
		{
			if (pM->is_excluded_dir(sdir)) return false;
			if (iswatch(sdir)) return true;
			wd=inotify_add_watch(fdI, sdir.c_str(), MONITOR_MASK);
			if (wd>=0)
			{
				BSC_WatchList[wd]=std::pair<Monitor*, std::string>(pM, sdir);
				std::unique_lock<std::mutex> LOCK(MUX_monitor_count); ++monitor_count; LOCK.unlock();
				LogVerbose("[wd=", wd, "] Monitor set for '", sdir, "'");
				return true;
			}
			else return inawerr(errno, sdir);
		};
	
	//create watches-------------------------------------------------------
	watchit(pMon, pMon->sourcedir);
	scansubs(pMon, pMon->sourcedir, watchit);

	//watch-loop-----------------------------------------------------------
	char EventBuf[BUF_LEN];
	struct inotify_event *pEvt;
	int n, length;
	Monitor *pM;
	std::string swdir;
	while (!bStopMonitoring)
	{
		length=read(fdI, EventBuf, BUF_LEN );
		if (length>0)
		{
			n=0;
			while (n<length)
			{
				pEvt=(struct inotify_event*)&EventBuf[n];

  LogDebug("got event : wd=", pEvt->wd, ", for name='", pEvt->name, "'");

				if (BSC_WatchList.find(pEvt->wd)!=BSC_WatchList.end())
				{
					pM=BSC_WatchList[pEvt->wd].first;
					swdir=BSC_WatchList[pEvt->wd].second;

  LogDebug("pM-sourcedir='", pM->sourcedir, ", watchlist-dir='", swdir, "'");

					if (!pM->IsExtracting())
					{
						if (pEvt->len)
						{
							std::string fd=path_append(swdir, pEvt->name);
							if ((pEvt->mask&IN_ISDIR)==IN_ISDIR)
							{
								if (((pEvt->mask&IN_CREATE)==IN_CREATE)||((pEvt->mask&IN_MOVED_TO)==IN_MOVED_TO)) //new sub-dir - add to watchlist..
								{

  LogDebug("created or renamed dir: '", fd, "'");

									for (auto p:pM->bupdirs)
									{
										std::string td=path_append(p.first, fd.substr(pM->sourcedir.size()+1));
										pM->sync_compress(fd, td);
									}
									if (watchit(pM, fd)) scansubs(pM, pM->sourcedir, watchit);
								}
							}
							else
							{

  LogDebug("new, renamed or modified file: '", fd, "'");
							
								if (isfile(fd)) pM->DoBackup(fd);
							} //ignore if not a regular file
						}
					}
				}
				n+=(EVENT_SIZE+pEvt->len);
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	//clean-up-------------------------------------------------------------
	std::unique_lock<std::mutex> LOCK(MUX_monitor_count);
		while (!BSC_WatchList.empty())
		{
			auto pW=BSC_WatchList.begin();
			inotify_rm_watch(fdI, pW->first);
			--monitor_count;
			std::this_thread::sleep_for(std::chrono::milliseconds(10)); //give inotify time to cleanup..?
			BSC_WatchList.erase(pW);
		}
	LOCK.unlock();


  LogDebug("exit threads for '", pMon->sourcedir, "'");
							

}



