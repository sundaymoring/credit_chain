#ifndef TOKENDB_H
#define TOKENDB_H

#include "dbwrapper.h"
#include "uint256.h"
#include "amount.h"

class CTransaction;
class CTokenIssure;

struct CTokenInfo {

    // TODO change or add what u want
    uint272 tokenID;
    CAmount amount;
    std::string shortName;
    std::string fullName;
    std::string description;
    std::string url;
    std::string address;
    uint256 txHash;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(tokenID);
        READWRITE(amount);
        READWRITE(shortName);
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

    bool GetTokenInfo(const uint272& tokenId, CTokenInfo& tokenInfo);
    bool WriteTokenInfo(const CTokenInfo& tokenInfo);
    bool EraseTokenInfo(const uint272& tokenID);
    const std::vector<CTokenInfo> ListTokenInfos();
};

extern CTokenDB* pTokenInfos;

#endif // TOKENDB_H
