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

#include "bscglobals.h"
#include "utilfuncs.h"
#include <unistd.h>
#include <sys/time.h>
#include <sstream>

bool bVERBOSE_LOG;
bool bDEBUG_LOG;

const double VERSION=1.0; // major . revision
const std::string Version() { return ttos(VERSION); }

const std::string DateTime(bool bmillisec)
{
	int m,d,H,M,S,ms;
	std::stringstream ss;
	time_t t;
	struct tm tmcur;

	time(&t);
	tmcur=*localtime(&t);
	m=(tmcur.tm_mon+1);	d=tmcur.tm_mday;
	H=tmcur.tm_hour;	M=tmcur.tm_min;		S=tmcur.tm_sec;

	ss	<< (1900+tmcur.tm_year) << ((m<=9)?"0":"") << m
		<< ((d<=9)?"0":"") << d << "-" << ((H<=9)?"0":"") << H << ":"
		<< ((M<=9)?"0":"") << M << ":" << ((S<=9)?"0":"") << S;
	if (bmillisec)
	{
		struct timeval tv;
		gettimeofday(&tv, NULL);
		ms=(tv.tv_usec % 10000L);
		ss << "." << ms;
	}
	return ss.str();
}

DTStamp dt_stamp()
{
	static uint16_t prev=0;
	time_t t;
	struct tm tmcur;
	struct timeval tv;
	DTStamp dts=0;
//	uint8_t y,m,d,H,M,S;
	uint16_t ms;
	time(&t);
	tmcur = *localtime(&t);
	dts=tmcur.tm_year;
	dts<<=8; dts+=tmcur.tm_mon+1;
	dts<<=8; dts+=tmcur.tm_mday;
	dts<<=8; dts+=tmcur.tm_hour;
	dts<<=8; dts+=tmcur.tm_min;
	dts<<=8; dts+=tmcur.tm_sec;
	gettimeofday(&tv, NULL);
	ms=(uint16_t)(tv.tv_usec % 10000L); //difference in 4-digit microsecond-tail (if need can do % 65535L)
	if (ms==prev) { prev++; ms=prev; } prev=ms;
	dts<<=16; dts+=ms;
	return dts;
}

DTStamp ToDTStamp(const std::string &yyyymmddHHMMSS)
{
	int n, l=yyyymmddHHMMSS.size();
	DTStamp dts=0;
	if (l>=8) //"yyyymmdd" at least
	{
		n=(stot<int>(yyyymmddHHMMSS.substr(0,4))-1900); //"[XXXX]mmddHHMMSS"
		if ((n>0)&&(n<255))
		{
			dts=(uint8_t)n;
			n=stot<int>(yyyymmddHHMMSS.substr(4,2)); //"yyyy[XX]ddHHMMSS"
			if ((n>0)&&(n<=12))
			{
				dts<<=8; dts+=(uint8_t)n;
				n=stot<int>(yyyymmddHHMMSS.substr(6,2)); //"yyyymm[XX]HHMMSS"
				if ((n>0)&&(n<=31))
				{
					dts<<=8; dts+=(uint8_t)n;
					if (l>=10)
					{
						n=stot<int>(yyyymmddHHMMSS.substr(8,2)); //"yyyymmdd[XX]MMSS"
						if ((n>=00)&&(n<24))
						{
							dts<<=8; dts+=(uint8_t)n;
							if (l>=12)
							{
								n=stot<int>(yyyymmddHHMMSS.substr(10,2)); //"yyyymmddHH[XX]SS"
								if ((n>=00)&&(n<60))
								{
									dts<<=8; dts+=(uint8_t)n;
									if (l>=14)
									{
										n=stot<int>(yyyymmddHHMMSS.substr(12,2)); //"yyyymmddHHMM[XX]"
										if ((n>=00)&&(n<60)) { dts<<=8; dts+=(uint8_t)n; }
										else dts<<=8;
									}
									else dts<<=8;
								}
								else dts<<=16;
							}
							else dts<<=16;
						}
						else dts<<=24;
					}
					else dts<<=24;
				}
				else dts=0; //error invalid day
			}
			else dts=0; //error invalid month
		}
		else dts=0; //error invalid year
	}
	if (dts!=0) dts<<=16; //ms
	return dts;
}

const std::string ToDateStr(DTStamp dts)
{
	std::stringstream ss("");
	uint64_t n;
	n=(dts>>56); ss << (int)(1900+n); //y
	n=((dts>>48)&0xff); ss << "-" << (int)n; //m
	n=((dts>>40)&0xff); ss << "-" << (int)n; //d
	n=((dts>>32)&0xff); ss << "-" << (int)n; //H
	n=((dts>>24)&0xff); ss << ":" << (int)n; //M
	n=((dts>>16)&0xff); ss << ":" << (int)n; //S
	n=(dts&0xffff); ss  << "." << (int)n; //ms
	return ss.str();
}

void write_log(const std::string &sData) { file_append(LOG_FILE, spf(DateTime(true), "\t", sData, "\n")); }
bool report_error(const std::string &serr) { LogLog("ERROR: ", serr); if (isatty(1)) tellerror(serr, "\n"); return false; }

bool wildcard_match(const std::string &wcs, const std::string &sval)
{ //wcs=>e.g.: "*~, *.tmp, *.temp, ?-*.bup, *.~*~" ==comma-delimited wildcard-templates
	std::vector<std::string> vex;
	int v=0, n, fn=sval.size();
	bool b=false;
	vex.clear();
	n=(int)desv<std::string>(wcs, ',', vex, false);
	while (!b&&(v<n))
	{
		std::string sw=vex.at(v++);
		int f=0, w=0, wn=sw.length();
		while ((f<fn)&&(w<wn) && (sval.at(f)==sw.at(w))) { f++; w++; }
		if ((w==wn)&&(f==fn)) { b=true; continue; }
		if ((w==wn)||(f>=fn)) continue;
		while ((f<fn)&&(w<wn))
		{
			if (sw.at(w)=='*') { while ((w<wn)&&((sw.at(w)=='*'))) w++; if (w==wn) { b=true; continue; }}
			if (sw.at(w)=='?')
			{
				while ((w<wn)&&(f<fn)&&((sw.at(w)=='?'))) { w++; f++; }
				if ((w==wn)&&(f==fn)) { b=true; continue; }
				if ((w==wn)||(f==fn)) continue;
			}
			std::string sp="";
			while ((w<wn)&&((sw.at(w)!='*')&&(sw.at(w)!='?'))) { sp+=sw.at(w); w++; }

			f=sval.find(sp, f);
			if (f==(int)std::string::npos) { f=fn+1; continue; }
			f+=sp.length();
			if ((w==wn)&&(f==fn)) { b=true; continue; }
			if ((w==wn)||(f==fn)) continue;
		}
		if ((w==wn)&&(f==fn)) { b=true; continue; }
	}
	return b;
}





