// Copyright (c) 2016-2018 The Currency Network developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TOKEN_DB_H
#define BITCOIN_TOKEN_DB_H

#include <string>
#include "dbwrapper.h"
#include "uint256.h"
#include "amount.h"
#include "token/token.h"

class CTokenDB : public CDBWrapper {

public:


    CTokenDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);
    bool GetTokenInfo(const CTokenId& TokenId, CTokenInfo& tokenInfo);
    bool WriteTokenInfo(const CTokenInfo& tokenInfo);
    bool EraseTokenInfo(const CTokenId& TokenId);
    bool ExistsTokenInfo(const CTokenId& TokenId);
    const std::vector<CTokenInfo> ListTokenInfo();
};

class CSymbolDB : public CDBWrapper {
public:
    CSymbolDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);
    bool GetTokenIdFromSymbol(const std::string& symbol, CTokenId& tokenId );
    bool WriteSymbolTokenId(const std::string& symbol, const CTokenId& tokenId);
    bool EraseSymbolTokenId(const std::string& symbol);
    bool ExistsSymbol(const std::string& symbol);
    const std::vector<std::string> ListSymbols();
};

extern CTokenDB* ptokendbview;
extern CSymbolDB* psymboldbview;

#endif // BITCOIN_TOKEN_DB_H
