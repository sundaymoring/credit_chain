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
    //TODO one byte is version
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
};



class CTokenIssure : public CTokenProtocol {
public:
    uint8_t nType;
    CAmount nValue;
    std::string symbol;
    std::string fullName;
    std::string description;
    std::string url;

    CTokenIssure() : CTokenProtocol(TTC_ISSUE) {}
    CTokenIssure(const uint8_t nIsDivisibleIn, const CAmount nValueIn, const std::string& shortNameIn, const std::string& fullNameIn, const std::string& descriptionIn,const std::string& urlIn)
        :CTokenProtocol(TTC_ISSUE), nType(nIsDivisibleIn), nValue(nValueIn), symbol(shortNameIn), fullName(fullNameIn), description(descriptionIn), url(urlIn){
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        CTokenProtocol::SerializationOp(s, ser_action);
        READWRITE(nType);
        READWRITE(nValue);
        READWRITE(symbol);
        READWRITE(fullName);
        READWRITE(description);
        READWRITE(url);
    }

    bool issureToken(const CBitcoinAddress& tokenAddress, uint256& txid, std::string& strFailReason);
    bool decodeTokenTransaction(const CTransaction& tx, std::string strFailReason);
};

//TODO add send protocol data
class CTokenSend : public CTokenProtocol{
public:
    CTokenSend() : CTokenProtocol(TTC_SEND) {}

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        CTokenProtocol::SerializationOp(s, ser_action);
    }
    bool sendToken(const CBitcoinAddress& tokenAddress, const CTokenID& tokenID, const CAmount& tokenValue, uint256& txid, std::string& strFailReason);
};

tokencode GetTxTokenCode(const CTransaction& tx);


#endif // TOKEN_H
