#ifndef BITCOIN_TOKEN_DB_H
#define BITCOIN_TOKEN_DB_H

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
    const std::vector<CTokenInfo> ListTokenInfos();
};

extern CTokenDB* ptokendbview;

#endif // BITCOIN_TOKEN_DB_H
