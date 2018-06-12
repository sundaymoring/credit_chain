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
#include "validation.h"

#include <stdexcept>
#include <univalue.h>

using namespace std;

bool EnsureWalletIsAvailable(bool avoidException);

UniValue issuretoken(const JSONRPCRequest& request){
    if (request.fHelp || request.params.size() != 4)
        throw runtime_error(
            "issuretoken assetaddress type amount name\n"
            "\nReturns the txid when issure token success.\n"
            "\nArguments:\n"
            "1. address     (string, required) The bitcoin address to own token\n.\n"
            "2. height      (numeric, optional, default=-1) To estimate at the time of the given height.\n"
            "\nResult:\n"
            "x             (numeric) Hashes per second estimated\n"
            "\nExamples:\n"
            + HelpExampleCli("getnetworkhashps", "")
            + HelpExampleRpc("getnetworkhashps", "")
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

// TODO getbalance use cache
static void SendToken(const CTxDestination &address, uint272 tokenID, CAmount nValue, bool fSubtractFeeFromAmount, CWalletTx& wtxNew)
{
    CAmount curTokenBalance = pwalletMain->GetBalance(tokenID, false);
    CAmount curBtcBalance = pwalletMain->GetBalance(TOKENID_ZERO, false);

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

UniValue sendtokentoaddress(const JSONRPCRequest& request)
{
    if (!EnsureWalletIsAvailable(request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() < 3 || request.params.size() > 6)
        throw runtime_error(
            "sendtoaddress \"address\" amount ( \"comment\" \"comment_to\" subtractfeefromamount )\n"
            "\nSend an amount to a given address.\n"
            + HelpRequiringPassphrase() +
            "\nArguments:\n"
            "1. \"tokenid\"            (string, required) Choose the tokenid to send.\n"
            "2. \"address\"            (string, required) The bitcoin address to send to.\n"
            "3. \"amount\"             (numeric or string, required) The amount in " + CURRENCY_UNIT + " to send. eg 0.1\n"
            "4. \"comment\"            (string, optional) A comment used to store what the transaction is for. \n"
            "                             This is not part of the transaction, just kept in your wallet.\n"
            "5. \"comment_to\"         (string, optional) A comment to store the name of the person or organization \n"
            "                             to which you're sending the transaction. This is not part of the \n"
            "                             transaction, just kept in your wallet.\n"
            "6. subtractfeefromamount  (boolean, optional, default=false) The fee will be deducted from the amount being sent.\n"
            "                             The recipient will receive less bitcoins than you enter in the amount field.\n"
            "\nResult:\n"
            "\"txid\"                  (string) The transaction id.\n"
            "\nExamples:\n"
            + HelpExampleCli("sendtoaddress", "tokenID, \"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 0.1")
            + HelpExampleCli("sendtoaddress", "tokenID, \"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 0.1 \"donation\" \"seans outpost\"")
            + HelpExampleCli("sendtoaddress", "tokenID, \"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 0.1 \"\" \"\" true")
            + HelpExampleRpc("sendtoaddress", "tokenID, \"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\", 0.1, \"donation\", \"seans outpost\"")
        );

    LOCK2(cs_main, pwalletMain->cs_wallet);

    CTokenID tokenID;
    tokenID.SetHex(request.params[0].get_str());
    //TODO relize function tokenID.IsValid() GetTokenInfo()
//    if (!tokenID.IsValid())
//    	throw JSONRPCError(RPC_TOKEN_INVALID, "Invalid Token Id");
//    CTokenInfo tokenInfo;
//    if ( !GetTokenInfo(tokenID, &tokenInfo)){
//    	throw JSONRPCError(RPC_TOKEN_NOT_FOUND, "Token Id Not Found");
//    }

    CBitcoinAddress address(request.params[1].get_str());
    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Bitcoin address");

    // Amount
    CAmount nAmount = AmountFromValue(request.params[2]);
    if (nAmount <= 0)
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount for send");

    // Wallet comments
    CWalletTx wtx;
    if (request.params.size() > 3 && !request.params[3].isNull() && !request.params[3].get_str().empty())
        wtx.mapValue["comment"] = request.params[3].get_str();
    if (request.params.size() > 4 && !request.params[4].isNull() && !request.params[4].get_str().empty())
        wtx.mapValue["to"]      = request.params[4].get_str();

    bool fSubtractFeeFromAmount = false;
    if (request.params.size() > 5)
        fSubtractFeeFromAmount = request.params[5].get_bool();

    EnsureWalletIsUnlocked();

    SendToken(address.Get(), tokenID, nAmount, fSubtractFeeFromAmount, wtx);

    return wtx.GetHash().GetHex();
}

static const CRPCCommand commands[] =
{ //  category              name                      actor (function)         okSafeMode
  //  --------------------- ------------------------  -----------------------  ----------
    { "token",              "issuretoken",            &issuretoken,            true,  {"asset_address","type","amount","name"} },
    { "token",              "listtokens",             &listtokens,             true,  {} },
    { "token",              "sendtoaddresstoken",     &sendtokentoaddress,     true,  {"toaddress", "tokenID", "amount"} },
};

void RegisterTokenRPCCommands(CRPCTable &t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
