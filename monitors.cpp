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
#include <unistd.h>
#include <sys/inotify.h>

extern bool bStopMonitoring;
int fdInotify=(-1);
int get_fdinotify() { return fdInotify; }

bool open_inotify()
{
	if (fdInotify==(-1)) fdInotify=inotify_init1(IN_NONBLOCK);
	if (fdInotify<0)
	{
		std::string e=spf("open_inotify error[", errno, "] ");
		switch(errno)
		{
			case EINVAL: e+="(inotify_init1()) An invalid value was specified in flags."; break;
			case EMFILE: e+="The user limit on the total number of inotify instances has been reached."; break;
			case ENFILE: e+="The system limit on the total number of file descriptors has been reached."; break;
			case ENOMEM: e+="Insufficient kernel memory is available."; break;
			default: e+="unknown error";
		}
		report_error(e);
	}
	return (fdInotify>(-1));
}

bool close_inotify() { if (fdInotify>(-1)) close(fdInotify); fdInotify=(-1); return true; }

void Monitors::exit_clear() { StopAll(); clear(); }

bool Monitors::Load()
{
	clear();
	return DBM.Load(*this);
}

bool Monitors::AddSource(const std::string &sSrcDir, const std::string &direx, const std::string &fileex)
{
	if (!dir_exist(sSrcDir)) return report_error(spf("cannot find source dir: '", sSrcDir, "'"));
	
	//if (!check_syntax(direx)) return report_error(spf("invalid dir-excludes: '", direx, "'"));
	//if (!check_syntax(fileex)) return report_error(spf("invalid file-excludes: '", fileex, "'"));
	///todo...tough one...e.g.: wildcard-templates are sep'd by comma => template cannot contain commas, so..when is it a template & when not?
	///case of garbage-in-garbage-out? --- alternative is specify excl's separately one-by-one...(then comma's won't matter)

	Monitor *pM=GetMonitor(sSrcDir);
	if (pM)
	{
		pM->direxcludes=direx.empty()?"":direx;
		pM->fileexcludes=fileex.empty()?"":fileex;
		return pM->Save();
	}
	else
	{
		Monitor B;
		B.sourcedir=sSrcDir;
		B.direxcludes=direx.empty()?"":direx;
		B.fileexcludes=fileex.empty()?"":fileex;
		if (B.Save()) { push_back(B); return true; }
		return report_error(spf("failed to create monitor for '", sSrcDir, "'"));
	}
}

bool Monitors::validate_bupdir(const std::string &sSrcDir, const std::string &sBupDir)
{

	///todo...think through...
	//...error - bup is sub of src => tail-chasing...
	//...error? - if src is parent of a current monitored src... (??? e.g.: bup subs to workbubs, & bup all to somewhere else
	//...not sub of a bup => error --- why? should be ok
	//... same dev/part => warn -- ok, intent is for safe backup in case drive fails..
	//...if parent of a bup then replaces bup OR error - must first remove bup then add parent ... prevent duplicating monitors...
	//...?
	
	if (issubdir(sBupDir, sSrcDir)) return report_error("cannot backup to a sub-dir of the source-directory");
	auto it=begin();
	while (it!=end())
	{
		if (!it->bupdirs.empty()) //...
		{
			//auto itb=it->bupdirs.begin();
			//while (itb!=it->bupdirs.end())
			//{
				///...these would be OK... why not?...
				//if (issubdir(sBupDir, itb->first)) return report_error("backup is a sub-dir of an existing backup-directory");
				//if (issubdir(itb->first, sBupDir)) return report_error("an existing backup-directory is a sub-dir of backup");
				//itb++;
			//}
		}
		it++;
	}
	return true;
}

bool Monitors::AddBackup(const std::string &sSrcDir, const std::string &sBupDir)
{
	Monitor* pM=GetMonitor(sSrcDir);
	if (!pM) return report_error(spf("monitor for source-dir '", sSrcDir, "' does not exist"));
	std::string sbdir=path_append(sBupDir, path_name(pM->sourcedir));
	if (!pM->HasBupdir(sbdir))
	{
		if (!validate_bupdir(pM->sourcedir, sbdir)) return report_error(spf("backup-directory '", sbdir, "' is invalid"));
		if (!path_realize(sbdir)) return report_error(spf("cannot create backup-dir '", sbdir, "'"));
		return pM->AddBupdir(sbdir);
	}
	else return pM->sync_bupdir(sbdir);
}

bool Monitors::Remove(const std::string &sSrcDir, const std::string &sBupDir)
{
	bool b=false;
	Monitor *pm=GetMonitor(sSrcDir);
	if (pm)
	{
		if (sBupDir.empty()) { b=DBM.Delete(*pm); }
		else { b=pm->RemoveBupdir(sBupDir); }
	}
	if (b) { if ((b=StopAll())) b=StartAll(); }
	return b;
}

Monitor* Monitors::GetMonitor(const std::string &sSrcDir)
{
	Monitor *pB=nullptr;
	auto it=begin();
	while (!pB&&(it!=end())) { if (seqs(sSrcDir, it->sourcedir)) pB=&(*it); it++; }
	return pB;
}

bool Monitors::StartAll()
{
	if (IsMonitoring()) StopAll();
	clear();
	if (!open_inotify()) return report_error("cannot initialize inotify");
	bStopMonitoring=false;
	init_bsc_watches();
	if (DBM.Load(*this)) //(re)load fresh from db
	{
		if (size()>0)
		{
			for (auto& M:(*this))
			{
				M.fdInotify=fdInotify;
				if (M.bupdirs.size()>0) { M.Start(); }
				else LogLog("FAIL: no backup-dir for '", M.sourcedir, "'");
			}
		}
		else LogVerbose("(nothing to monitor)");
		return true;
	}
	report_error(spf("Database: ", DBM.sDB, " - ", DBM.GetLastError()));
	return false;
}

bool Monitors::StopAll()
{
	if (size()>0) { bStopMonitoring=true; for (auto& M:(*this)) { M.Stop(); }}
	close_inotify();
	return !IsMonitoring();
}
