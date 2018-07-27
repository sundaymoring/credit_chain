#include "token/db.h"

static const char DB_TOKEN_INFO = 'I';

CTokenDB* ptokendbview;

CTokenDB::CTokenDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "tokens", nCacheSize, fMemory, fWipe) {
}

bool CTokenDB::GetTokenInfo(const CTokenId& tokenId, CTokenInfo& tokenInfo)
{
    return Read(std::make_pair(DB_TOKEN_INFO, tokenId), tokenInfo);
}

bool CTokenDB::WriteTokenInfo(const CTokenInfo& tokenInfo)
{
    return Write(std::make_pair(DB_TOKEN_INFO, tokenInfo.tokenId), tokenInfo);
}

bool CTokenDB::EraseTokenInfo(const CTokenId& tokenId)
{
    return Erase(std::make_pair(DB_TOKEN_INFO, tokenId));
}

bool CTokenDB::ExistsTokenInfo(const CTokenId& tokenId)
{
    return Exists(std::make_pair(DB_TOKEN_INFO, tokenId));
}

const std::vector<CTokenInfo> CTokenDB::ListTokenInfo()
{
    std::vector<CTokenInfo> info;
    CDBIterator* iter = NewIterator();
    for(iter->SeekToFirst(); iter->Valid(); iter->Next()){
        CTokenInfo t;
        if( iter->GetValue(t) )
            info.push_back(t);
    }
    delete iter;
    return info;
}
