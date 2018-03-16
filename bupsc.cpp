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

#include "bupsc.h"
#include "dbmaster.h"
#include "utilfuncs.h"
#include "bscglobals.h"
#include <unistd.h>
#include <pwd.h>
#include <sys/time.h>
#include <sstream>
#include <fstream>


int RESTART_SIGNAL;// SIGUSR1
int STOP_SIGNAL; // SIGUSR2
int DEBUG_ON_SIGNAL; // SIGRTMIN+1
int DEBUG_OFF_SIGNAL; // SIGRTMIN+2
int VERBOSE_ON_SIGNAL; // SIGRTMIN+3
int VERBOSE_OFF_SIGNAL; // SIGRTMIN+4


//--------------------------------------------------------------------------------------------------
std::string FULL_APP_NAME;
std::string APP_NAME;
std::string CONFIG_PATH;
std::string LOCK_FILE=std::string("/var/lock/bupsc.lock"); //NB: FIXED LOCATION!
std::string MASTER_DB; //prj's meta
std::string LOG_FILE;


//--------------------------------------------------------------------------------------------------
void AppName(const std::string &sname) { FULL_APP_NAME=sname; APP_NAME=path_name(sname); }
const std::string AppName() { return APP_NAME; }

pid_t get_lock_pid()
{
	pid_t p=0;
	std::string s;
	std::ifstream ifs(LOCK_FILE.c_str()); if (ifs.good()) { std::getline(ifs, s); p=stot<pid_t>(s); }
	if (p!=0)
	{
		bool b=false;
		std::ifstream ifs(spf("/proc/", p, "/cmdline").c_str());
		while (ifs.good()&&!b) { std::getline(ifs, s);  b=(s.rfind(APP_NAME.c_str())!=std::string::npos); }
		if (!b) { p=0; file_delete(LOCK_FILE.c_str()); } //prev instance died unexpectedly
	}
	return p;
}

bool is_running() { return (get_lock_pid()!=0); } //stale lock removed by get_lock_pid()

//single instance running as daemon..
bool set_status(bool bSet=true)
{
	pid_t pid=getpid(), lpid=get_lock_pid();
	std::string s;

	if (lpid==pid) { if (bSet) return true; else { file_delete(LOCK_FILE); lpid=0; }}
	if (lpid==0) { if (!bSet) return true; }
	if (lpid!=0) return false;
	return (bSet)?file_write(LOCK_FILE, ttos<pid_t>(pid)):true;
}

bool InitBVC()
{
	//prep signals...
	RESTART_SIGNAL=SIGUSR1;
	STOP_SIGNAL=SIGUSR2;
	DEBUG_ON_SIGNAL=SIGRTMIN+1;
	DEBUG_OFF_SIGNAL=SIGRTMIN+2;
	VERBOSE_ON_SIGNAL=SIGRTMIN+3;
	VERBOSE_OFF_SIGNAL=SIGRTMIN+4;
	
	std::string st;
	struct passwd *pw=getpwuid(getuid());
	CONFIG_PATH=pw->pw_dir; //homedir
	CONFIG_PATH=path_append(CONFIG_PATH, spf(".config/", APP_NAME));
	if (!path_realize(CONFIG_PATH)) return report_error(spf("cannot create config: '", CONFIG_PATH, "'"));
	LOG_FILE=path_append(CONFIG_PATH, spf(APP_NAME, ".log"));
	MASTER_DB=path_append(CONFIG_PATH, "master.db");
	bool bdbexist=file_exist(MASTER_DB);
	if (DBM.Open(MASTER_DB))
	{
		if (!bdbexist) { if (!DBM.ImplementSchema()) { report_error(spf("Database '", MASTER_DB, "': ", DBM.GetLastError())); return false; }}
		return true;
	}
	else return report_error(spf("cannot open master database '", MASTER_DB, "'"));
}

void ExitBVC() { DBM.Close(); }

bool StartBVC()
{
	if (is_running()) return report_error(spf(APP_NAME, " is already running")); //not an error
	if (daemon(0,0)==0) { if (set_status()) { LogVerbose("Started"); return true; }}
	report_error(spf(APP_NAME, " failed to start"));
	return false;
}

bool StopBVC()
{
	if (set_status(false)) { LogVerbose("Stopped"); return true; }
	report_error(spf(APP_NAME, " failed to stop"));
	return false;
}

bool PostRestart()	{ pid_t lpid=get_lock_pid(); if (lpid!=0) { kill(lpid, RESTART_SIGNAL); return true; } return false; }
bool PostQuit()		{ pid_t lpid=get_lock_pid(); if (lpid!=0) { kill(lpid, STOP_SIGNAL); return true; } return false; }

void PostDebug(bool bOn) { pid_t lpid=get_lock_pid(); if (lpid!=0) { kill(lpid, (bOn)?DEBUG_ON_SIGNAL:DEBUG_OFF_SIGNAL); }}
void PostVerbose(bool bOn) { pid_t lpid=get_lock_pid(); if (lpid!=0) { kill(lpid, (bOn)?VERBOSE_ON_SIGNAL:VERBOSE_OFF_SIGNAL); }}

