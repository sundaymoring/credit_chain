#ifndef TOKENDB_H
#define TOKENDB_H

#include "dbwrapper.h"
#include "uint256.h"
#include "amount.h"

struct CTokenInfo {

    // TODO change or add what u want
    uint272 tokenID;
    std::string name;
    CAmount amount;
    std::string address;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(tokenID);
        READWRITE(name);
        READWRITE(amount);
        READWRITE(address);
    }
};


class CTokenDB {

public:
    CDBWrapper db;
    CTokenDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);

    bool GetTokenInfo(const int32_t tokenId, CTokenInfo& tokenInfo);
    bool WriteTokenInfo(const CTokenInfo& tokenInfo);
    const std::vector<CTokenInfo> ListTokenInfos();
};

extern CTokenDB* pTokenInfos;

#endif // TOKENDB_H
