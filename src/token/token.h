#ifndef TOKEN_H
#define TOKEN_H

#include "tinyformat.h"
#include "script/script.h"
#include "uint256.h"
#include "serialize.h"
#include "amount.h"

#include <vector>

static const CAmount TOKEN_DEFAULT_VALUE = 0.001 * COIN;
static const CAmount TOKEN_ISSUE_FEE = 0.01 * COIN;

class CTransaction;

tokencode GetTokenCodeFromScript(const CScript& script, std::vector<unsigned char>* pTokenData = NULL);
tokencode GetTxTokenCode(const CTransaction& tx, std::vector<unsigned char>* pTokenData = NULL);

class CTokenId : public uint288
{
private:
    class CBase58Id;
public:
    CTokenId() : uint288() {}
    CTokenId(const base_blob<288>& b) : uint288(b) {}
    CTokenId(const uint256& left, const unsigned int& right){
        SetHex( strprintf("%s%08x", left.ToString().c_str(), right) );
    }

    explicit CTokenId(const std::vector<unsigned char>& vch) : uint288(vch) {}

    std::string ToBase58String() const;
    void FromBase58String(const std::string& strBase58Id);
};

struct CTokenTxIssueInfo
{
    uint8_t type;
    CAmount amount;
    std::string issueAddress;
    std::string symbol;
    std::string name;
    std::string url;
    std::string description;

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(type);
        READWRITE(amount);
        READWRITE(issueAddress);
        READWRITE(symbol);
        READWRITE(name);
        READWRITE(url);
        READWRITE(description);
    }

};

CScript CreateIssuanceScript(const CTokenTxIssueInfo& issueinfo);
bool GetIssueInfoFromScriptData(CTokenTxIssueInfo& issueinfo, const std::vector<unsigned char>& scriptdata);

CScript CreateSendScript(const CTokenId& tokenid);
bool GetSendInfoFromScriptData(CTokenId& tokenid, const std::vector<unsigned char>& scriptdata);

struct CTokenInfo {

    // HTODO change or add what u want
	CTokenId tokenId;
    uint8_t type;
    CAmount moneySupply;
    std::string symbol;
    std::string name;
    std::string url;
    std::string description;
    std::string issueToAddress;
    uint256 txHash;


    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(tokenId);
        READWRITE(type);
        READWRITE(moneySupply);
        READWRITE(symbol);
        READWRITE(name);
        READWRITE(url);
        READWRITE(description);
        READWRITE(issueToAddress);
        READWRITE(txHash);
    }

    CTokenInfo(const CTokenTxIssueInfo& info);
    CTokenInfo() {
        setnull();
    }
    void setnull() {
        tokenId = CTokenId();
        type = 0;
        moneySupply = 0;
        symbol.clear();
        name.clear();
        url.clear();
        description.clear();
        issueToAddress.clear();
        txHash = uint256();
    }

};

static const CTokenId TOKENID_ZERO;

#endif // TOKEN_H
