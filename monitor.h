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

#ifndef _monitor_h_
#define _monitor_h_

#include "dbsqlite3types.h"
#include "bscglobals.h"
#include <string>
#include <vector>
#include <map>
#include <thread>

struct Monitor
{
	int fdInotify;
	IDTYPE id;
	std::string sourcedir;
	std::string direxcludes;
	std::string fileexcludes;
	std::map<std::string, DTStamp> bupdirs;
	bool bIsExtracting; //CYA when extracting to this monitored sourcedir
	
	std::thread this_monitor;
	
	void clear() { id=0; sourcedir.clear(); direxcludes.clear(); fileexcludes.clear(); bupdirs.clear(); bIsExtracting=false; }
	bool is_excluded_dir(const std::string &sdir);
	bool is_excluded_file(const std::string &sfile);
	bool sync_compress(DTStamp dts, const std::string &F, const std::string &T, const std::map<std::string, int> &md);
	bool sync_compress(const std::string &F, const std::string &T);
	bool sync_bupdir(const std::string &sbupdir); //get bup & source in same state
	bool IsExtracting() { return bIsExtracting; }
	void SetExtracting(bool b=true) { bIsExtracting=b; }

	Monitor();
	Monitor(const Monitor &B);
	virtual ~Monitor();
	
	Monitor& operator=(const Monitor &B);
	
	bool Save();
	bool AddBupdir(const std::string &sdir);
	bool HasBupdir(const std::string &sdir);
	bool RemoveBupdir(const std::string &sdir);
	bool DoBackup(const std::string &sfile);
	bool ExtractBackup(const std::string &sDest, DTStamp xdts=0);
	static void do_monitor(Monitor *pMon);
	bool Start();
	bool Stop();

};

struct Monitors : std::vector<Monitor>
{
	void init_clear() { clear(); } //???there was some reason somewhere...
	void exit_clear();

	bool validate_bupdir(const std::string &sSrcDir, const std::string &sBupDir);

	Monitors()							{ init_clear(); }
	virtual ~Monitors()					{ exit_clear(); }

	bool Load();
	bool AddSource(const std::string &sSrcDir, const std::string &direx="", const std::string &fileex="");
	bool AddBackup(const std::string &sSrcDir, const std::string &sBupDir);
	bool Remove(const std::string &sSrcDir, const std::string &sBupDir);
	Monitor* GetMonitor(const std::string &sSrcDir);
	bool StartAll();
	bool StopAll();
	
};

bool open_inotify();
bool close_inotify();
int get_fdinotify();
void init_bsc_watches();
bool IsMonitoring();

#endif
