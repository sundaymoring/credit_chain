#ifndef TOKENDB_H
#define TOKENDB_H

#include "dbwrapper.h"
#include "uint256.h"
#include "amount.h"

class CTransaction;

struct CTokenInfo {

    // TODO change or add what u want
    uint272 tokenID;
    CAmount amount;
    std::string address;
    std::string name;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(tokenID);
        READWRITE(amount);
        READWRITE(address);
        READWRITE(name);
    }

    void FromTx(const CTransaction& tx);
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
