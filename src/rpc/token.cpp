#include "base58.h"
#include "utilmoneystr.h"
#include "utilstrencodings.h"
#include "consensus/validation.h"
#include "amount.h"
#include "rpc/server.h"
#include "tytoken/token.h"
#include "tytoken/tokendb.h"
#include "wallet/wallet.h"
#include "net.h"
#include "policy/policy.h"

#include <stdexcept>
#include <univalue.h>

using namespace std;

UniValue issuretoken(const JSONRPCRequest& request){
    if (request.fHelp || request.params.size() != 4)
        throw runtime_error(
            "issuretoken assetaddress type amount name\n"
            );

    std::string assetAddress =request.params[0].get_str();
    int type = request.params[1].get_int();
    CAmount amount = request.params[2].get_int64() * COIN;
    std::string tokenName = request.params[3].get_str();

    // create a payload for the transaction
    vector<unsigned char> payload = CreatePayload_IssuanceFixed(uint16_t(type), amount, tokenName);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    bool ret = WalletTxBuilder(assetAddress, amount, payload, txid, rawHex);

    UniValue result(UniValue::VOBJ);
    result.push_back(Pair("result", ret?"ok":"false"));
    if(ret){
        result.push_back(Pair("txid", txid.ToString()));
    }
    return result;
}

UniValue listtokens(const JSONRPCRequest& request){

    if (request.fHelp )
        throw runtime_error(
            "list all token infos\n"
            );

    UniValue result(UniValue::VARR);

    std::vector<CTokenInfo> infos = pTokenInfos->ListTokenInfos();
    for (const auto& info : infos){
        UniValue one(UniValue::VOBJ);
        one.push_back(Pair("id", info.tokenID.ToString()));
        one.push_back(Pair("amout", info.amount));
        one.push_back(Pair("address", info.address));
        result.push_back(one);
    }
    return result;
}

static void SendToken(const CTxDestination &address, uint272 tokenID, CAmount nValue, bool fSubtractFeeFromAmount, CWalletTx& wtxNew)
{
    CAmount curTokenBalance = pwalletMain->GetBalance(tokenID, false);
    CAmount curBtcBalance = pwalletMain->GetBalance(UINT272_ZERO, false);

    // Check amount
    if (nValue <= 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid amount");

    if (nValue > curTokenBalance)
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Insufficient token funds");

    //TODO use right min tbc, not 0
    if (curBtcBalance <= 0)
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Insufficient funds");

    if (pwalletMain->GetBroadcastTransactions() && !g_connman)
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");

    // Parse Bitcoin address
    CScript scriptPubKey = GetScriptForDestination(address);

    // Create and send the transaction
    CReserveKey reservekey(pwalletMain);
    CAmount nFeeRequired;
    std::string strError;
    vector<CRecipient> vecSend;
    int nChangePosRet = -1;
    CRecipient recipient = {scriptPubKey, GetDustThreshold(scriptPubKey), fSubtractFeeFromAmount, tokenID, nValue};

    vecSend.push_back(recipient);
    if (!pwalletMain->CreateTransaction(vecSend, wtxNew, reservekey, nFeeRequired, nChangePosRet, strError, NULL, true, TTC_SEND)) {
        throw JSONRPCError(RPC_WALLET_ERROR, strError);
    }
    CValidationState state;
    if (!pwalletMain->CommitTransaction(wtxNew, reservekey, g_connman.get(), state)) {
        strError = strprintf("Error: The transaction was rejected! Reason given: %s", state.GetRejectReason());
        throw JSONRPCError(RPC_WALLET_ERROR, strError);
    }
}

UniValue sendToAddressToken(const JSONRPCRequest& request){
    if (request.fHelp || request.params.size() <3)
        throw runtime_error(
            "issuretoken assetaddress type amount name\n"
            );

    CBitcoinAddress address(request.params[0].get_str());
    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Bitcoin address");

    uint272 tokenID;
    tokenID.SetHex(request.params[1].get_str());

    // Amount
    CAmount nAmount = AmountFromValue(request.params[2]);
    if (nAmount <= 0)
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount for send");

    bool fSubtractFeeFromAmount = false;
    CWalletTx wtx;

    SendToken(address.Get(), tokenID, nAmount, fSubtractFeeFromAmount, wtx);

    return wtx.GetHash().GetHex();


}

static const CRPCCommand commands[] =
{ //  category              name                      actor (function)         okSafeMode
  //  --------------------- ------------------------  -----------------------  ----------
    { "token",              "issuretoken",            &issuretoken,            true,  {"asset_address","type","amount","name"} },
    { "token",              "listtokens",             &listtokens,             true,  {} },
    { "token",              "sendtoaddresstoken",     &sendToAddressToken,     true,  {"toaddress", "tokenID", "amount"} },
};

void RegisterTokenRPCCommands(CRPCTable &t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
