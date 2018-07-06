#include "tytoken/token.h"

#include "wallet/coincontrol.h"
#include "wallet/wallet.h"
#include "script/script.h"
#include "consensus/validation.h"
#include "script/standard.h"
#include "net.h"
#include "policy/policy.h"
#include "script/standard.h"
#include "rpc/server.h"
#include "utilmoneystr.h"

#include "util.h"

#include <vector>


static bool createTokenTransaction(const std::vector<CRecipient>& vecRecipients, uint256& txid, int& nChangePosInOut, std::string& strFailReason, const CCoinControl* coinControl, bool sign, tokencode tokenType)
{
    if (pwalletMain == NULL) return false;

    if (vecRecipients.size() < 2){
        strFailReason = _("token transaction recipient number should less than 2");
        return false;
    }

    CAmount curBtcBalance = pwalletMain->GetBalance(TOKENID_ZERO);
    if (curBtcBalance <= vecRecipients[1].nAmount)
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Insufficient funds");

    // Check token amount
    if (vecRecipients[1].nTokenAmount <= 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid token amount");
    if (vecRecipients[1].tokenID != TOKENID_ZERO){
        CAmount curTokenBalance = pwalletMain->GetBalance(vecRecipients[1].tokenID);
        if (vecRecipients[1].nTokenAmount > curTokenBalance)
            throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Insufficient token funds");
    }

    if (pwalletMain->GetBroadcastTransactions() && !g_connman)
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");

    CWalletTx wtxNew;
    CAmount nFeeRequired;
    CReserveKey reserveKey(pwalletMain);
    if (!pwalletMain->CreateTransaction(vecRecipients, wtxNew, reserveKey, nFeeRequired, nChangePosInOut, strFailReason, coinControl, sign, tokenType)) {
        if (nFeeRequired + vecRecipients[1].nAmount > curBtcBalance){
            std::string strError = strprintf("Error: This transaction requires a transaction fee of at least %s", FormatMoney(nFeeRequired));
            throw JSONRPCError(RPC_WALLET_ERROR, strError);
        }
        return false;
    }

    // Commit the transaction to the wallet and broadcast)
    LogPrintf("%s: %s; nFeeRet = %d\n", __func__, wtxNew.tx->ToString(), nFeeRequired);
    CValidationState state;
    if (!pwalletMain->CommitTransaction(wtxNew, reserveKey, g_connman.get(), state)){
        std::string strError = strprintf("Error: The transaction was rejected! Reason given: %s", state.GetRejectReason());
        throw JSONRPCError(RPC_WALLET_ERROR, strError);
    }
    txid = wtxNew.GetHash();
    return true;
}

//TODO bitcoin sendfrom assetAddress, and token in assetAddress. now bitcoin sendfrom other address.
bool CTokenIssure::issureToken(const CBitcoinAddress& tokenAddress, uint256& txid, std::string& strFailReason)
{
    CScript scriptReturn;
    CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
    ds << *(static_cast<CTokenProtocol*>(this));
    scriptReturn << OP_RETURN << std::vector<unsigned char>(ds.begin(), ds.end());
    ds.clear();
    ds << *this;
    scriptReturn << std::vector<unsigned char>(ds.begin(), ds.end());

    CScript scriptToken = GetScriptForDestination(tokenAddress.Get());

    std::vector<CRecipient> vecRecipients;
    vecRecipients.push_back(CRecipient{scriptReturn, 0, false, TOKENID_ZERO, 0});
    vecRecipients.push_back(CRecipient{scriptToken, GetDustThreshold(scriptToken), false, TOKENID_ZERO, nValue});

    int nChangePosInOut = 2;

    return createTokenTransaction(vecRecipients, txid, nChangePosInOut, strFailReason, NULL, true, TTC_ISSUE);
}

bool CTokenIssure::decodeTokenTransaction(const CTransaction &tx, std::string strFailReason)
{
//    if ( TTC_ISSUE != tx.GetTokenCode() ){
//        strFailReason = "token procotol is not issure";
//        return false;
//    }

    txnouttype type;
    std::vector<unsigned char> vReturnData;
    bool ret = ExtractPushDatas(tx.vout[0].scriptPubKey, type, vReturnData);
    if (!ret || type!=TX_NULL_DATA){
        strFailReason = "issure token procotol error";
        return false;
    }
    CDataStream ds(vReturnData, SER_NETWORK, CLIENT_VERSION);
    ds >> *(static_cast<CTokenProtocol*>(this));
    ds >> *this;

    //TODO check token is already exist
    //TODO check value is equal to vout[1] nTokenvalue

    return true;
}


bool CTokenSend::sendToken(const CBitcoinAddress& tokenAddress, const CTokenID& tokenID, const CAmount& tokenValue, uint256& txid, std::string& strFailReason)
{
    CScript scriptReturn, scriptToken;
    CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
    ds << *(static_cast<CTokenProtocol*>(this));
    scriptReturn << OP_RETURN << std::vector<unsigned char>(ds.begin(), ds.end());
    ds.clear();
    ds << *this;
    scriptReturn << std::vector<unsigned char>(ds.begin(), ds.end());

    scriptToken = GetScriptForDestination(tokenAddress.Get());

    std::vector<CRecipient> vecRecipients;
    vecRecipients.push_back(CRecipient{scriptReturn, 0, false, tokenID, 0});
    vecRecipients.push_back(CRecipient{scriptToken, GetDustThreshold(scriptToken), false, tokenID, tokenValue});

    int nChangePosInOut = 2;
    return createTokenTransaction(vecRecipients, txid, nChangePosInOut, strFailReason, NULL, true, TTC_SEND);
}

//TODO solve it like witness
//TODO is TTC_BITCOIN  need define again
tokencode GetTxReturnTokenCode(const CTransaction& tx)
{
    assert(tx.vout.size()>0);
    if (tx.IsCoinBase() || tx.IsCoinStake())
        return TTC_NONE;

    // vout[0] op_return
    // vout[1] dust bitcoin, token own address
    // vout[2] charge address
    if (tx.vout.size() < 2)
        return TTC_NONE;

    if (tx.vout[0].nValue >0)
        return TTC_NONE;

    txnouttype whichType;
    std::vector<unsigned char> vPushData;
    // get the scriptPubKey corresponding to this input:
    if (!ExtractPushDatas(tx.vout[0].scriptPubKey, whichType, vPushData)){
        return TTC_NONE;
    }


//    if (whichType != TX_NULL_DATA){
//        for (const auto& out:tx.vout){
//            if (out.tokenID != TOKENID_ZERO && out.nTokenValue >0)
//                return TTC_SEND;
//        }
//        return TTC_NONE;
//    } else {
//        if (vPushData[0] == 'T'&& vPushData[1] == 'T' && vPushData[2] == 'K'){
//            if (vPushData[5]>=TTC_ISSUE && vPushData[5]<=TTC_BURN){
//                return tokencode(vPushData[5]);
//            }
//            return TTC_NONE;
//        }
//    }

    if (whichType == TX_NULL_DATA){
        if (vPushData.size() < 6){
            return TTC_NONE;
        }
        if (vPushData[0] == 'T'&& vPushData[1] == 'T' && vPushData[2] == 'K'){
            if (vPushData[5]>=TTC_ISSUE && vPushData[5]<=TTC_BURN){
                return tokencode(vPushData[5]);
            }
            return TTC_NONE;
        }
    }
    return TTC_NONE;
}

