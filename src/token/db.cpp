// Copyright (c) 2016-2018 The Currency Network developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "token/db.h"

static const char DB_TOKEN_INFO = 'I';
static const char DB_SYMBOL = 'S';

CTokenDB* ptokendbview;
CSymbolDB* psymboldbview;

CTokenDB::CTokenDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "token", nCacheSize, fMemory, fWipe) {
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

CSymbolDB::CSymbolDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "symbol", nCacheSize, fMemory, fWipe)
{

}

bool CSymbolDB::GetTokenIdFromSymbol(const std::string &symbol, CTokenId &tokenId)
{
    return Read(std::make_pair(DB_SYMBOL, symbol), tokenId);
}

bool CSymbolDB::WriteSymbolTokenId(const std::string &symbol, const CTokenId &tokenId)
{
    return Write(std::make_pair(DB_SYMBOL, symbol), tokenId);
}

bool CSymbolDB::EraseSymbolTokenId(const std::string &symbol)
{
    return Erase(std::make_pair(DB_SYMBOL, symbol));
}

bool CSymbolDB::ExistsSymbol(const std::string &symbol)
{
    return Exists(std::make_pair(DB_SYMBOL, symbol));
}

const std::vector<std::string> CSymbolDB::ListSymbols()
{
    std::vector<std::string> ids;
    CDBIterator* iter = NewIterator();
    for(iter->SeekToFirst(); iter->Valid(); iter->Next()){
        std::string t;
        if( iter->GetValue(t) )
            ids.push_back(t);
    }
    delete iter;
    return ids;
}
