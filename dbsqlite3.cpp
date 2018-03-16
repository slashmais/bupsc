
#include "dbsqlite3.h"
#include "utilfuncs.h"
#include <string>

DBsqlite3::DBResult::DBResult() { Clear(); }
DBsqlite3::DBResult::~DBResult() { Clear(); }

void DBsqlite3::DBResult::Clear()	{ Rows.clear(); sQuery.clear(); }
void DBsqlite3::DBResult::AddColVal(const std::string scol, const std::string sval, row_t &r) { r.push_back(ColVal(scol,sval)); }
void DBsqlite3::DBResult::AddRow(row_t &r) { Rows.push_back(r); }

const std::string DBsqlite3::DBResult::GetQuery() { return sQuery; }

int DBsqlite3::DBResult::GetColCount() { return ((Rows.size()>0)?Rows.at(0).size():0); }
int DBsqlite3::DBResult::GetRowCount() { return Rows.size(); }

const std::string DBsqlite3::DBResult::GetName(int nCol, int nRow)
{
	return (((nRow<(int)Rows.size())&&(nCol<GetColCount()))?Rows.at(nRow).at(nCol).col:"");
}

const std::string DBsqlite3::DBResult::GetVal(int nCol, int nRow)
{
	return (((nRow<(int)Rows.size())&&(nCol<GetColCount()))?Rows.at(nRow).at(nCol).val:"");
}

const std::string DBsqlite3::DBResult::GetVal(const std::string sCol, int nRow)
{
	if (nRow<(int)Rows.size())
	{
		auto it = Rows.at(nRow).begin();
		while (it != Rows.at(nRow).end()) { if (sieqs((*it).col, sCol)) return ((*it).val); it++; }
	}
	return "";
}

DBsqlite3::DBsqlite3() { bDBOK=false; pRS=NULL; sDB.clear(); sLastError.clear(); sLastSQL.clear(); }

DBsqlite3::~DBsqlite3() { Close(); }

bool DBsqlite3::init_ids(const std::string &tname)
{
	DBResult RS;
	std::string sSQL;

	sSQL=spf("SELECT * FROM IDS WHERE name = ", SQLSafeQ(tname));
	if (ExecSQL(&RS, sSQL)>0) return true; //exists
	sSQL=spf("INSERT INTO IDS (name, id, del) VALUES(", SQLSafeQ(tname), ", 0, '')");
	ExecSQL(&RS, sSQL);
	return NoError(this);
}

IDTYPE DBsqlite3::new_id(const std::string &tname)
{
	std::string sSQL, s;
	DBResult RS;
	IDTYPE id=0;
	
	sSQL=spf("SELECT id, del FROM IDS WHERE name = ", SQLSafeQ(tname));
	ExecSQL(&RS, sSQL);
	if (NoError(this))
	{
		std::vector<IDTYPE> v;
		s=SQLRestore(RS.GetVal("del", 0));
		if (desv<IDTYPE>(s, IDS_DELIM, v, false)>0)
		{
			id=v[0];
			v.erase(v.begin());
			ensv<IDTYPE>(v, IDS_DELIM, s, false);
			sSQL=spf("UPDATE IDS SET del = ", SQLSafeQ(s), " WHERE name = ", SQLSafeQ(tname));
			ExecSQL(&RS, sSQL);
			if (!NoError(this)) id=0;
		}
		else
		{
			id=stot<IDTYPE>(RS.GetVal("id",0))+1;
			sSQL=spf("UPDATE IDS SET id = ", id, " WHERE name = ", SQLSafeQ(tname));
			ExecSQL(&RS, sSQL);
			if (!NoError(this)) id=0;
		}
	}
	return id;
}

bool DBsqlite3::del_id(const std::string &tname, IDTYPE id)
{
	std::string sSQL, s;
	DBResult RS;
	
	sSQL=spf("SELECT id, del FROM IDS WHERE name = ", SQLSafeQ(tname));
	ExecSQL(&RS, sSQL);
	if (NoError(this))
	{
		std::vector<IDTYPE> v;
		s=SQLRestore(RS.GetVal("del", 0));
		desv<IDTYPE>(s, IDS_DELIM, v, false);
		v.push_back(id);
		ensv<IDTYPE>(v, IDS_DELIM, s, false);
		sSQL=spf("UPDATE IDS SET del = ", SQLSafeQ(s), " WHERE name = ", SQLSafeQ(tname));
		ExecSQL(&RS, sSQL);
	}
	return NoError(this);
}

int DBsqlite3::Callback(void *pCaller, int argc, char **argv, char **azColName)
{
	DBsqlite3 *pDB=(DBsqlite3*)pCaller;
	if (!pDB) return 1;
	if (argc>0)
	{
		DBResult::row_t r;
		for(int i=0; i<argc; i++) pDB->pRS->AddColVal(azColName[i], ((argv[i])?argv[i]:"NULL"), r);
		pDB->pRS->AddRow(r);
	}
	return 0;
}

bool DBsqlite3::Open(const std::string sDBName)
{
	sLastError.clear();
	sLastSQL.clear();
	if (!bDBOK)
	{
		sDB = sDBName;
		bDBOK = (sqlite3_open(sDB.c_str(), &pDB)==0);
		if (!bDBOK) sLastError = "Cannot open database";
	}
	return bDBOK;
}

void DBsqlite3::Close() { if (bDBOK) sqlite3_close(pDB); bDBOK=false; }
	
size_t DBsqlite3::ExecSQL(DBResult *pRSet, const std::string sSQL)
{
	if (!bDBOK) { sLastError = "Database not open"; return 0; }
	char *szErrMsg = 0;
	pRSet->sQuery = sSQL;
	pRS = pRSet;
	int rc = sqlite3_exec(pDB, sSQL.c_str(), &DBsqlite3::Callback, this, &szErrMsg);
	if(rc!=SQLITE_OK) { sLastError = szErrMsg; sqlite3_free(szErrMsg); }
	sLastSQL=sSQL;
	return pRSet->GetRowCount();
}

bool DBsqlite3::MakeTable(const std::string sT, const std::string sF)
{
	DBResult RS;
	std::string sSQL;
	sSQL=spf("CREATE TABLE IF NOT EXISTS ", sT.c_str(), "(", sF.c_str(), ")");
	ExecSQL(&RS, sSQL);
	return (sLastError.empty());
}

const std::string DBsqlite3::GetLastError() { return sLastError; }

const std::string SQLSafeQ(const std::string &sval)
{
	std::string t=sval, r;
	if (!t.empty())
	{
		ReplaceChars(t, "\"", "\"\"");
		ReplaceChars(t, "'", "''");
	}
	r="'"; r+=t; r+="'";
	return r;
}

const std::string SQLRestore(const std::string &sval)
{
	std::string r=sval;
	if (!r.empty())
	{
		ReplacePhrase(r, "\"\"", "\"");
		ReplacePhrase(r, "''", "'");
	}
	return r;
}

bool NoError(DBsqlite3 *pdb)
{
	return (pdb->GetLastError().empty());
}

