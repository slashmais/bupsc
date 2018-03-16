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
#include "utilfuncs.h"
#include "bscglobals.h"
#include "monitor.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <map>


//=====Switch verbose- & debug-logs on when developing...
//bool bIS_DEV_MODE=true;
bool bIS_DEV_MODE=false;
//=====


volatile bool bQuit;
volatile bool bRestart;
std::mutex MUX;
std::condition_variable CVMain;

void signal_handler(int sig)
{
	if (sig==RESTART_SIGNAL)				{ bRestart=true; CVMain.notify_one(); }
	if ((sig==SIGINT)||(sig==STOP_SIGNAL))	{ bQuit=true; CVMain.notify_one(); }
	if (sig==DEBUG_ON_SIGNAL)				{ bDEBUG_LOG=true; }
	if (sig==DEBUG_OFF_SIGNAL)				{ bDEBUG_LOG=false; }
	if (sig==VERBOSE_ON_SIGNAL)				{ bVERBOSE_LOG=true; }
	if (sig==VERBOSE_OFF_SIGNAL)			{ bVERBOSE_LOG=false; }
}

void show_help()
{
	telluser("\nDoes real-time deep backup of changed/added regular files and",
			 "\ndirectories from a source-directory to (optional multiple)",
			 "\nbackup-directories. (Devices/Pipes/Sockets/Links/? are ignored)",
			 (is_running())?spf("\n\n *** An instance of ", AppName(), " is active ***"):"",

			 "\n\nUsage: ", AppName(), " [-D|--DEBUG] [-V|--VERBOSE] [OPTION]",

			 "\n\nDefinitions:",
			 "\n    SOURCE   => existing directory to be backed-up",
			 "\n    DEX, FEX => quoted strings - see Exclusions below",
			 "\n    BDIR     => path to directory for backup",
			 "\n    XDIR     => path to directory for extraction",
			 "\n    DATETIME => quoted datetime, format: \"yyyymmdd[HH[MM[SS]]]\"",
			 
			 "\n\nIf no option specified, runs ", AppName(), " as a daemon,",
			 "\notherwise one of the following options per invocation applies:",
			 "\nOption              Result",
			 "\n -?, --help       display this help and exit",
			 "\n -a, --add SOURCE [DEX [FEX]]",
			 "\n                  adds SOURCE, DEX and FEX to database",
			 "\n -b, --backup SOURCE BDIR",
			 "\n                  adds BDIR to list of backups-list",
			 "\n -D, --DEBUG      toggles writing to ", LOG_FILE+".DEBUG",
			 "\n                    with debug information",
			 "\n -l, --list       displays current list of monitored sources",
			 "\n RESTART          stop, reload data, and restart the daemon",
			 "\n -r, --remove SOURCE [BDIR]",
			 "\n                  removes either SOURCE or only BDIR from the",
			 "\n                    lists, does NOT remove the content",
			 "\n STOP             terminate the daemon",
			 "\n -V, --VERBOSE    toggles writing to ", LOG_FILE+".VERBOSE",
			 "\n                    with information on current state & activity",
			 "\n -v, --version    display version information and exit",
			 "\n -x, --extract SOURCE [XDIR [DATETIME]]",
			 "\n                  extracts full current (or current-at-DATETIME)",
			 "\n                    backup for SOURCE to XDIR; if XDIR is empty",
			 "\n                    extraction will be to SOURCE",

			 "\n\nExclusions",
			 "\nDEX (Dir-EXcludes), FEX (File EXcludes) are each a comma-delimited list",
			 "\nof wildcard-names for which matching entries must be excluded from the",
			  "\nbackup - may be empty quoted strings.",
			 "\nWilcards are '*' for a range of, and '?' for single characters",
			 "\nExamples: \"temp, data/scratch\" \"*~, *.temp\"",
			 "\nThis will exclude from the backup '/path/to/source/temp' and",
			 "\n '/path/to/source/data/scratch' sub-directories, and all files ending",
			 "\nwith a ~ or .temp, in /path/to/source and it's sub-directories.",
			 "\n\n");
}

void show_version() { telluser(AppName(), Version(), "\n"); }

enum
{
	OPT_IGNORE=0,
	OPT_HELP=1, //default option
	OPT_ADD,
	OPT_BACKUP,
	OPT_LIST,
	OPT_REMOVE,
	OPT_EXTRACT,
	OPT_ERROR,
};

bool process_command(int c, char **pp)
{
	// looked at the getopt..-stuff - maybe later
	int n=0;
	auto nextc=[&](std::string &s)->bool{ if (++n<c) s=pp[n]; else s.clear(); return !s.empty(); };
	auto setopt=[&](int &o, int i){ o=(o<=OPT_HELP)?i:OPT_ERROR; };
	if (c>1)
	{
		std::string s, p1, p2, p3;
		int opt=OPT_HELP;
		while (nextc(s)&&((opt==OPT_HELP)||(opt==OPT_IGNORE)))
		{
			TRIM(s, "- "); //the minus-chars are placebos for pedants :)
			if (seqs(s, "?")||sieqs(s, "help"))			{ while (nextc(s)); }
			else if (seqs(s, "a")||sieqs(s, "add"))		{ setopt(opt, OPT_ADD); nextc(p1); nextc(p2); nextc(p3); }
			else if (seqs(s, "b")||sieqs(s, "backup"))	{ setopt(opt, OPT_BACKUP); nextc(p1); nextc(p2); }
			else if (seqs(s, "D")||sieqs(s, "DEBUG"))
			{
				setopt(opt, OPT_IGNORE); //other options may follow
				bDEBUG_LOG=!bDEBUG_LOG;
				if (is_running()) PostDebug(bDEBUG_LOG);
			}
			else if (seqs(s, "r")||sieqs(s, "remove"))	{ setopt(opt, OPT_REMOVE); nextc(p1); nextc(p2); }
			else if (sieqs(s, "restart"))				{ PostRestart(); return true; }
			else if (sieqs(s, "stop"))					{ PostQuit(); return true; }
			else if (seqs(s, "l")||sieqs(s, "list"))	{ setopt(opt, OPT_LIST); }
			else if (seqs(s, "V")||sieqs(s, "VERBOSE"))
			{
				setopt(opt, OPT_IGNORE); //other options may follow
				bVERBOSE_LOG=!bVERBOSE_LOG;
				if (is_running()) PostVerbose(bVERBOSE_LOG);
			}
			else if (seqs(s, "v")||sieqs(s, "version"))	{ show_version(); return true; }
			else if (seqs(s, "x")||sieqs(s, "extract"))	{ setopt(opt, OPT_EXTRACT); nextc(p1);  nextc(p2);  nextc(p3); }
			else { opt=OPT_ERROR; break; }
		}

		Monitors Mons;
		Mons.Load();

		switch(opt)
		{
			case OPT_IGNORE:	return (is_running()); break; //carry on if startup
			case OPT_ADD:		Mons.AddSource(p1, p2, p3); break;
			case OPT_BACKUP:	{ if (Mons.AddBackup(p1, p2)) PostRestart(); } break;
			case OPT_LIST:
					{
						if (Mons.size()>0)
						{
							for (auto M:Mons)
								{
									telluser(M.sourcedir,"\n");
									if (M.bupdirs.size()>0) { for (auto p:M.bupdirs) telluser("\t", p.first, "\n"); }
									else telluser("\t(no backups)\n");
								}
						}
						else telluser("\t(no sources)\n");
					} break;
			case OPT_REMOVE:	Mons.Remove(p1, p2); break;
			case OPT_EXTRACT:
					{
						///todo:...single file extract...
						if (p1.empty()) { tellerror("missing extract options\n"); show_help(); }
						else
						{
							DTStamp dts=ToDTStamp(p3);
							if ((dts==0)&&!p3.empty()) { tellerror("invalid DATETIME '", p3, "' for extract\n"); show_help(); }
							else
							{
								Monitor *pM=Mons.GetMonitor(p1);
								if (pM)
								{
									if (p2.empty()) p2=p1;
									if (pM->bupdirs.size()>0) pM->ExtractBackup(p2, dts);
									else  telluser("extract specified, but there are no backups to extract from\n");
								}
								else { tellerror("invalid SOURCE '", p1, "' for extract\n"); show_help(); }
							}
						}
					} break;
			case OPT_ERROR: telluser("Error: invalid command-line options\n"); //fall-thru
			default: show_help();
		}
		return true;
	}
	return false;
}

void set_signals()
{
	signal(RESTART_SIGNAL, signal_handler);
	signal(STOP_SIGNAL, signal_handler);
	signal(SIGINT, signal_handler);
	signal(DEBUG_ON_SIGNAL, signal_handler);
	signal(DEBUG_OFF_SIGNAL, signal_handler);
	signal(VERBOSE_ON_SIGNAL, signal_handler);
	signal(VERBOSE_OFF_SIGNAL, signal_handler);
}

int main(int argc, char *argv[])
{
	bVERBOSE_LOG=bIS_DEV_MODE;
	bDEBUG_LOG=bIS_DEV_MODE;
	
	AppName(argv[0]);
	if (!InitBVC()) return tellerror("Cannot initialize ", AppName());
	bRestart=bQuit=false;
	if (process_command(argc, argv)) return 0;
	telluser("Starting ", AppName(), " version ", Version(), "\n");
	if (StartBVC())
	{
		Monitors monitors;
		set_signals();
		bQuit=!monitors.StartAll();
		while (!bQuit)
		{
			if (bRestart)
			{
				LogVerbose("..stopping..");
				if (monitors.StopAll()) { LogVerbose("..restarting.."); monitors.StartAll(); }
				else LogLog("BUG!: active monitors");
				bRestart=false;
			}
			std::unique_lock<std::mutex> LOCK(MUX);
			CVMain.wait(LOCK);
		}
		monitors.StopAll();
		StopBVC();
	}
	else show_help();
	ExitBVC();
    return 0;
}

