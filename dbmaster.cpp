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

#include "dbmaster.h"
#include "utilfuncs.h"
#include <string>
#include "monitor.h"

DBMaster DBM; //main db

bool DBMaster::ImplementSchema()
{
	bool b=bDBOK;
	DBResult RS;
	std::string sSQL;

	if (b)
	{
		sSQL = "CREATE TABLE IF NOT EXISTS IDS (name, id, del)";
		ExecSQL(&RS, sSQL); //create/reuse ids: init_ids(tabelname) once after CREATE TABLE.. and new_id() / del_id() after
		b=NoError(this);
	}
	
	if (b) b = (MakeTable("sources", "id, sourcedir, direxcludes, fileexcludes")&&init_ids("sources"));
	if (b) b = MakeTable("backups", "id, bupdir, dts");

	return b;
}

bool DBMaster::Save(Monitor &B)
{
	std::string sSQL;
	DBResult RS;
	if (B.id)
	{
		sSQL=spf("UPDATE sources SET",
				" sourcedir = ", SQLSafeQ(B.sourcedir),
				", direxcludes = ", (B.direxcludes.empty()?"''":SQLSafeQ(B.direxcludes)),
				", fileexcludes = ", (B.fileexcludes.empty()?"''":SQLSafeQ(B.fileexcludes)),
				" WHERE id = ", B.id);
		ExecSQL(&RS, sSQL);
	}
	else
	{
		B.id=new_id("sources");
		sSQL=spf("INSERT INTO sources(id, sourcedir, direxcludes, fileexcludes) VALUES(",
					B.id,
					", ", SQLSafeQ(B.sourcedir),
					", ", SQLSafeQ(B.direxcludes),
					", ", SQLSafeQ(B.fileexcludes), ")");
		ExecSQL(&RS, sSQL);
	}
	if (NoError(this)) return UpdateBackups(B.id, B.bupdirs);
	return false;
}

bool DBMaster::UpdateBackups(IDTYPE id, const std::map<std::string, DTStamp> &m)
{
	std::string sSQL;
	DBResult RS;
	sSQL=spf("DELETE FROM backups WHERE id=", id);
	ExecSQL(&RS, sSQL);
	if (NoError(this)&&(m.size()>0))
	{
		for (auto p:m)
		{
			sSQL=spf("INSERT INTO backups(id, bupdir, dts) VALUES(", id, ", ", SQLSafeQ(p.first), ", ", p.second, ")");
			ExecSQL(&RS, sSQL);
			if (!NoError(this)) break;
		}
	}
	return NoError(this);
}

bool DBMaster::Load(Monitors &BS)
{
	std::string sSQL;
	DBResult RS;
	size_t i=0,n;
	n=ExecSQL(&RS, "SELECT * FROM sources");
	if (NoError(this))
	{
		BS.clear();
		while (i<n)
		{
			Monitor B;
			B.id = stot<IDTYPE>(RS.GetVal("id", i));
			B.sourcedir = SQLRestore(RS.GetVal("sourcedir", i));
			B.direxcludes = SQLRestore(RS.GetVal("direxcludes", i));
			B.fileexcludes = SQLRestore(RS.GetVal("fileexcludes", i));
			if (!LoadBackups(B.id, B.bupdirs)) return false;
			BS.push_back(B);
			i++;
		}
		return true;
	}
	return false;
}

bool DBMaster::LoadBackups(IDTYPE id, std::map<std::string, DTStamp> &m)
{
	std::string sSQL;
	DBResult RS;
	size_t i=0,n;
	n=ExecSQL(&RS, spf("SELECT * FROM backups WHERE id=",id));
	if (NoError(this))
	{
		m.clear();
		while (i<n) { m[SQLRestore(RS.GetVal("bupdir", i))]=stot<DTStamp>(RS.GetVal("dts", i)); i++; }
		return true;
	}
	return false;
}

bool DBMaster::Delete(Monitor &B)
{
	if (B.id)
	{
		std::string sSQL;
		DBResult RS;
		sSQL=spf("DELETE FROM sources WHERE id = ", B.id);
		ExecSQL(&RS, sSQL);
		if (NoError(this)) { sSQL=spf("DELETE FROM backups WHERE id=", B.id); ExecSQL(&RS, sSQL); }
		return NoError(this);
	}
	SetLastError("cannot delete monitor: invalid id");
	return false;
}


