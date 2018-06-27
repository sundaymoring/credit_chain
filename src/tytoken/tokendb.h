#ifndef TOKENDB_H
#define TOKENDB_H

#include "dbwrapper.h"
#include "uint256.h"
#include "amount.h"

class CTransaction;
class CTokenIssure;

struct CTokenInfo {

    // TODO change or add what u want
    CTokenID tokenID;
    uint8_t type;
    CAmount amount;
    std::string symbol;
    std::string fullName;
    std::string description;
    std::string url;
    std::string address;
    uint256 txHash;

    CTokenInfo() : tokenID(TOKENID_ZERO), type(0), amount(0){
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(tokenID);
        READWRITE(type);
        READWRITE(amount);
        READWRITE(symbol);
        READWRITE(fullName);
        READWRITE(description);
        READWRITE(url);
        READWRITE(address);
        READWRITE(txHash);
    }

    void FromTx(const CTransaction& tx, const CTokenIssure& issure);
};


class CTokenDB {

public:
    CDBWrapper db;
    CTokenDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);

    bool GetTokenInfo(const CTokenID& tokenId, CTokenInfo& tokenInfo);
    bool WriteTokenInfo(const CTokenInfo& tokenInfo);
    bool EraseTokenInfo(const CTokenID& tokenID);
    const std::vector<CTokenInfo> ListTokenInfos();
};

extern CTokenDB* pTokenInfos;

#endif // TOKENDB_H
