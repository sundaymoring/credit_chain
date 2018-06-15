#ifndef TOKEN_H
#define TOKEN_H

#include "uint256.h"
#include "primitives/transaction.h"
#include "script/script.h"
#include "base58.h"


#include <string>
#include <vector>
#include <stdint.h>

class CTokenProtocol {
public:
    char header[3];
    uint8_t unknown[2];
    uint8_t code;

    CTokenProtocol(const uint8_t codeIn) :  code(codeIn){
        header[0] = 'T';
        header[1] = 'T';
        header[2] = 'K';
        unknown[0] = 0xcc;
        unknown[1] = 0xcc;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        for (auto& h : CTokenProtocol::header){
            READWRITE(h);
        }
        for (auto& h : CTokenProtocol::unknown){
            READWRITE(h);
        }
        READWRITE(code);
    }
    virtual bool createTokenTransaction(const CBitcoinAddress& tokenAddress, uint256& txid, std::string& strFailReason) = 0;
};



class CTokenIssure : public CTokenProtocol {
public:
    uint8_t nIsDivisible;
    CAmount nValue;
    std::string shortName;
    std::string fullName;
    std::string description;
    std::string url;

    CTokenIssure() : CTokenProtocol(TTC_ISSUE) {}
    CTokenIssure(const uint8_t nIsDivisibleIn, const CAmount nValueIn, const std::string& shortNameIn, const std::string& fullNameIn, const std::string& descriptionIn,const std::string& urlIn)
        :CTokenProtocol(TTC_ISSUE), nIsDivisible(nIsDivisibleIn), nValue(nValueIn), shortName(shortNameIn), fullName(fullNameIn), description(descriptionIn), url(urlIn){
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        CTokenProtocol::SerializationOp(s, ser_action);
        READWRITE(nIsDivisible);
        READWRITE(nValue);
        READWRITE(shortName);
        READWRITE(fullName);
        READWRITE(description);
        READWRITE(url);
    }

    bool createTokenTransaction(const CBitcoinAddress& tokenAddress, uint256& txid, std::string& strFailReason);
    bool decodeTokenTransaction(const CTransaction& tx, std::string strFailReason);
};

class CToken {
public:
    CToken(){}
    ~CToken(){}

    uint272 tokenID;
    std::string name;
};

bool WalletTxBuilder(const std::string& assetAddress, int64_t referenceAmount, const std::vector<unsigned char>& data, uint256& txid, std::string& rawHex, bool commit=true);

tokencode GetTxTokenCode(const CTransaction& tx);


#endif // TOKEN_H
