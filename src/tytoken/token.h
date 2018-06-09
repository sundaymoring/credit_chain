#ifndef TOKEN_H
#define TOKEN_H

#include "uint256.h"
#include "primitives/transaction.h"


#include <string>
#include <vector>
#include <stdint.h>

typedef enum TOKEN_OP_CODE{
    TOKEN_OP_ISSURE = 1,
    TOKEN_OP_SEND   = 2,
    TOKEN_OP_BURN   = 3,
}tokenOpCode;

class CToken {
public:
    CToken(){}
    ~CToken(){}

    uint272 tokenID;
    std::string name;
};

std::vector<unsigned char> CreatePayload_IssuanceFixed(uint16_t propertyType, uint64_t amount, std::string name);
bool WalletTxBuilder(const std::string& assetAddress, int64_t referenceAmount, const std::vector<unsigned char>& data, uint256& txid, std::string& rawHex, bool commit=true);

tokencode GetTxTokenCode(const CTransaction& tx);


#endif // TOKEN_H
