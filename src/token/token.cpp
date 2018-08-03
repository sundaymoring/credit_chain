#include "token.h"
#include "script/standard.h"
#include "base58.h"
#include "streams.h"
#include "clientversion.h"
#include "util.h"

tokencode GetTokenCodeFromScript(const CScript& script, std::vector<unsigned char>* pTokenData)
{
    txnouttype whichType;
    std::vector<std::vector<unsigned char>> vPushData;

    if (!Solver(script, whichType, vPushData)){
        return TTC_NONE;
    }

    if (whichType == TX_TOKEN){
        if (pTokenData && vPushData.size() > 0)
            *pTokenData = vPushData[0];
        return (tokencode)script[3];
    }

    return TTC_NONE;
}

tokencode GetTxTokenCode(const CTransaction& tx, std::vector<unsigned char>* pTokenData)
{
    assert(tx.vout.size()>0);
    if (tx.IsCoinBase() || tx.IsCoinStake())
        return TTC_NONE;

    // vout[0] op_token output
    // vout[1..n] bitcoin or token output
    if (tx.vout.size() < 2)
        return TTC_NONE;

    if (tx.vout[0].nValue > 0)
        return TTC_NONE;

    return GetTokenCodeFromScript(tx.vout[0].scriptPubKey, pTokenData);
}

class CTokenId::CBase58Id : public CBase58Data
{
public:
    CBase58Id(const CTokenId& id)
    {
        //prefix code do not same to address
        //119 120 121 122 123 prefix 'T'
        vchVersion.assign(1, 121);
        vchData.resize(id.size());
        if (!vchData.empty())
            memcpy(&vchData[0], id.begin(), id.size());
    }

    CBase58Id(const std::string& id)
    {
        size_t nVersionBytes = 1;
        std::vector<unsigned char> vchTemp;
        bool rc58 = DecodeBase58Check(id.data(), vchTemp);
        if ((!rc58) || (vchTemp.size() < nVersionBytes)) {
            vchData.clear();
            vchVersion.clear();
            return;
        }
        vchVersion.assign(vchTemp.begin(), vchTemp.begin() + nVersionBytes);
        vchData.resize(vchTemp.size() - nVersionBytes);
        if (!vchData.empty())
            memcpy(&vchData[0], &vchTemp[nVersionBytes], vchData.size());
        memory_cleanse(&vchTemp[0], vchTemp.size());
    }

    bool IsValid() const
    {
        bool fCorrectSize = vchData.size() == 36;
        bool fKnownVersion = vchVersion == std::vector<unsigned char>(1,121);
        return fCorrectSize && fKnownVersion;
    }

    bool ConvertToTokenID(CTokenId& tokenID)
    {
        if (vchData.size() != tokenID.size())
            return false;
        memcpy(tokenID.begin(), &vchData[0], tokenID.size());
        return true;
    }
};

std::string CTokenId::ToBase58String() const
{
    if (*this == TOKENID_ZERO)
        return "";
    return CBase58Id(*this).ToString();
}

bool CTokenId::FromBase58String(const std::string& strBase58Id)
{
    if (strBase58Id.empty())
        return false;

    CBase58Id id(strBase58Id);
    if (!id.IsValid()){
        return false;
    }
    return id.ConvertToTokenID(*this);
}

CTokenInfo::CTokenInfo(const CTokenTxIssueInfo& info)
{
    issueToAddress = info.issueAddress;
    moneySupply = info.amount;
    type = info.type;
    symbol = info.symbol;
    name = info.name;
    url = info.url;
    description = info.description;
}

CScript CreateIssuanceScript(const CTokenTxIssueInfo& issueinfo)
{
    CDataStream ds(SER_DISK, CLIENT_VERSION);
    ds << issueinfo;

    CScript script;
    script.resize(4);
    script[0] = OP_TOKEN;
    script[1] = 0x02;
    script[2] = TOKEN_PROTOCOL_VERSION;
    script[3] = TTC_ISSUE;

    script << std::vector<unsigned char>(ds.begin(), ds.end());

    return script;

}

bool GetIssueInfoFromScriptData(CTokenTxIssueInfo& issueinfo, const std::vector<unsigned char>& scriptdata)
{
    CDataStream ssValue(scriptdata, SER_DISK, CLIENT_VERSION);
    try {
        ssValue >> issueinfo;
    } catch (const std::exception& e) {
        return error("%s: decode token data error : %s", __func__, e.what());
    }
    return true;
}

CScript CreateSendScript(const CTokenId& tokenid)
{
    CDataStream ds(SER_DISK, CLIENT_VERSION);
    ds << tokenid;

    CScript script;
    script.resize(4);
    script[0] = OP_TOKEN;
    script[1] = 0x02;
    script[2] = TOKEN_PROTOCOL_VERSION;
    script[3] = TTC_SEND;

    script << std::vector<unsigned char>(ds.begin(), ds.end());

    return script;
}

bool GetSendInfoFromScriptData(CTokenId& tokenid, const std::vector<unsigned char>& scriptdata)
{
    CDataStream ssValue(scriptdata, SER_DISK, CLIENT_VERSION);
    try {
        ssValue >> tokenid;
    } catch (const std::exception& e) {
        return error("%s: decode token data error : %s", __func__, e.what());
    }
    return true;
}

