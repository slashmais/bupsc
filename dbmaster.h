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

#ifndef _dbmaster_h_
#define _dbmaster_h_

#include "dbsqlite3.h"
#include <map>
#include "bscglobals.h"

struct Monitor;
struct Monitors;

struct DBMaster : public DBsqlite3
{
	virtual ~DBMaster() {}
	bool ImplementSchema();
	bool Save(Monitor &B);
	bool UpdateBackups(IDTYPE id, const std::map<std::string, DTStamp> &m);
	bool Load(Monitors &BS);
	bool LoadBackups(IDTYPE id, std::map<std::string, DTStamp> &m);
	bool Delete(Monitor &B);
};

extern DBMaster DBM;

#endif
