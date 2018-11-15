// Copyright (c) 2016-2018 The Currency Network developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TOKEN_H
#define TOKEN_H

#include "tinyformat.h"
#include "script/script.h"
#include "uint256.h"
#include "serialize.h"
#include "amount.h"
#include "utilstrencodings.h"

#include <vector>

extern const CAmount TOKEN_DEFAULT_VALUE;
extern const CAmount TOKEN_ISSUE_FEE;

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
    CTokenId(const std::string& str){
        SetHex(str);
    }

    explicit CTokenId(const std::vector<unsigned char>& vch) : uint288(vch) {}

    bool FromString(const std::string& str){
        if (str.length() != size()*2){
            return false;
        }
        if (!IsHex(str)) {
            return false;
        }
        SetHex(str);
        return true;
    }

    std::string ToBase58String() const;
    bool FromBase58String(const std::string& strBase58Id);
};

struct CScriptTokenIssueInfo
{
    uint8_t type;
    CAmount totalSupply;
    CTokenId tokenid;
    std::string symbol;
    std::string name;
    std::string url;
    std::string description;
    std::string ownerAddress;

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(type);
        READWRITE(totalSupply);
        READWRITE(tokenid);
        READWRITE(symbol);
        READWRITE(name);
        READWRITE(url);
        READWRITE(description);
        READWRITE(ownerAddress);
    }
};

struct CScriptTokenSendInfo
{
    CTokenId tokenid;
    CAmount sendAmount;

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(tokenid);
        READWRITE(sendAmount);
    }
};

struct CScriptTokenBurnInfo
{
    CTokenId tokenid;
    CAmount burnAmount;

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(tokenid);
        READWRITE(burnAmount);
    }
};

//CScript CreateIssuanceScript(const CScriptTokenIssueInfo& issueinfo);
CScript CreateIssuanceScriptDummy(CScriptTokenIssueInfo &issueinfo);
CScript AppendIssuanceScript(CScriptTokenIssueInfo& issueinfo, const CTokenId tokenid);
bool GetIssueInfoFromScriptData(CScriptTokenIssueInfo& issueinfo, const std::vector<unsigned char>& scriptdata);

CScript CreateSendScript(const CTokenId tokenid, const CAmount sendAmount);
CScript CreateSendScriptDummy(const CTokenId tokenid);
bool GetSendInfoFromScriptData(CScriptTokenSendInfo& sendinfo, const std::vector<unsigned char>& scriptdata);

CScript CreateBurnScript(const CTokenId tokenid, const CAmount burnAmount);
bool GetBurnInfoFromScriptData(CScriptTokenBurnInfo& burninfo, const std::vector<unsigned char>& scriptdata);

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

    CTokenInfo(const CScriptTokenIssueInfo& info);
    CTokenInfo() {
        setnull();
    }
    void setnull() {
        tokenId.SetNull();
        type = 0;
        moneySupply = 0;
        symbol.clear();
        name.clear();
        url.clear();
        description.clear();
        issueToAddress.clear();
        txHash.SetNull();
    }

};

static const CTokenId TOKENID_ZERO;

#endif // TOKEN_H
