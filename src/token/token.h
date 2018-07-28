#ifndef TOKEN_H
#define TOKEN_H

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

struct CTokenTxIssueInfo
{
    uint8_t type;
    CAmount amount;
    std::string issueAddress;
    std::string symbol;
    std::string name;
    std::string url;
    std::string description;
};

CScript CreateIssuanceScript(CTokenTxIssueInfo issueinfo);
bool GetIssueInfoFromScriptData(CTokenTxIssueInfo issueinfo, std::vector<unsigned char> scriptdata);

CScript CreateSendScript(CTokenId tokenid);
bool GetSendInfoFromScriptData(CTokenId tokenid, std::vector<unsigned char> scriptdata);

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

    CTokenInfo(const CTokenTxIssueInfo info);
    CTokenInfo() {
        setnull();
    }
    void setnull() {
        tokenId = uint272();
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

#endif // TOKEN_H
