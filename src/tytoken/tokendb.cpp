#include "tytoken/tokendb.h"

static const char DB_TOKEN = 'T';

CTokenDB* pTokenInfos;

CTokenDB::CTokenDB(size_t nCacheSize, bool fMemory, bool fWipe) : db(GetDataDir() / "token", nCacheSize, fMemory, fWipe, true)
{
}

bool CTokenDB::GetTokenInfo(const int32_t tokenId, CTokenInfo &tokenInfo)
{
    return db.Read(std::make_pair(DB_TOKEN, tokenId), tokenInfo);
}

bool CTokenDB::WriteTokenInfo(const CTokenInfo &tokenInfo)
{
    return db.Write(std::make_pair(DB_TOKEN, tokenInfo.tokenID), tokenInfo);
}

const std::vector<CTokenInfo> CTokenDB::ListTokenInfos()
{
    std::vector<CTokenInfo> infos;
    CDBIterator* iter = db.NewIterator();
    for(iter->SeekToFirst(); iter->Valid(); iter->Next()){
        CTokenInfo t;
        if( iter->GetValue(t) )
            infos.push_back(t);
    }
    delete iter;
    return infos;
}
