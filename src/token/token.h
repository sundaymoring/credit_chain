#ifndef TOKEN_H
#define TOKEN_H

#include "script/script.h"
#include "uint256.h"

#include <vector>


class CTransaction;

tokencode GetTxTokenCode(const CTransaction& tx, std::vector<unsigned char>* pTokenData = NULL);

//typedef uint272 CTokenID;
class CTokenId : public uint272
{
private:
    class CBase58Id;
public:
    CTokenId() : uint272() {}
    CTokenId(const base_blob<272>& b) : uint272(b) {}
    explicit CTokenId(const std::vector<unsigned char>& vch) : uint272(vch) {}

    std::string ToBase58String() const;
    void FromBase58String(const std::string& strBase58Id);
};

static const CTokenId TOKENID_ZERO;


#endif // TOKEN_H
