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
    if (request.fHelp || request.params.size() < 7)
        throw runtime_error(
            "issuretoken assetaddress type amount name\n"
            "\nReturns the txid when issure token success.\n"

            "\nArguments:\n"
            "1. fromaddress          (string, required) the address to own token\n"
            "2. type                 (number, required) the type of the tokens to create: (1 for indivisible tokens, 2 for divisible tokens)\n"
            "3. amount               (string, required) the number of tokens to create\n"
            "4. short name           (string, required) the short name of the new tokens to create\n"
            "5. full name            (string, required) the full name of the new tokens to create\n"
            "6. description          (string, required) a description for the new tokens (can be \"\")\n"
            "7. url                  (string, required) an URL for further information about the new tokens (can be \"\")\n"

            "\nResult:\n"
            "\"hash\"                  (string) the hex-encoded transaction hash\n"

            "\nExamples:\n"
                + HelpExampleCli("issuretoken", "\"3Ck2kEGLJtZw9ENj2tameMCtS3HB7uRar3\" 1 1000000 \"btc\" \"bitcoin\" \"bitcoin is first Cryptocurrency\" \"https://bitcoin.org\"")
            );

    CBitcoinAddress tokenAddress(request.params[0].get_str());
    if (!tokenAddress.IsValid()){
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Bitcoin address");
    }
    uint8_t isDivisible = uint8_t(request.params[1].get_int());
    CAmount amount = request.params[2].get_int64() * COIN;
    std::string symbol = request.params[3].get_str();
    std::string fullName = request.params[4].get_str();
    std::string description = request.params[5].get_str();
    std::string url = request.params[6].get_str();

    symbol = symbol.length()>255 ? symbol.substr(0,255) : symbol;
    fullName = fullName.length()>255 ? fullName.substr(0,255) : fullName;
    description = description.length()>255 ? description.substr(0,255) : description;
    url = url.length()>255 ? url.substr(0,255) : url;

    uint256 txid;
    std::string strFailReason;
    CTokenIssure tIssure(isDivisible, amount, symbol, fullName, description, url);
    bool ret = tIssure.issureToken(tokenAddress, txid,  strFailReason);

    return ret ? txid.ToString() : strFailReason;

}

UniValue listtokens(const JSONRPCRequest& request){

    if (request.fHelp )
        throw runtime_error(
            "listtokens\n"
            "\nlist all token infos\n"
            "\nResult:\n"
            "[\n"
            "    {\n"
            "        \"id\": \"tokenid\",\n"
            "        \"amout\": n,\n"
            "        \"shortName\": \"sn\",\n"
            "        \"fullName\": \"fullname\",\n"
            "        \"description\": \"description of token\",\n"
            "        \"url\": \"url of token\",\n"
            "        \"address\": \"token owner\",\n"
            "        \"txid\": \"generate token transaction\"\n"
            "    }\n"
            "    ...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("listtokens", "")
            + HelpExampleRpc("listtokens", "")
            );

    UniValue result(UniValue::VARR);

    std::vector<CTokenInfo> infos = pTokenInfos->ListTokenInfos();
    for (const auto& info : infos){
        UniValue one(UniValue::VOBJ);
        one.push_back(Pair("id", info.tokenID.ToString()));
        one.push_back(Pair("amout", info.amount));
        one.push_back(Pair("symbol", info.symbol));
        one.push_back(Pair("fullName", info.fullName));
        one.push_back(Pair("description", info.description));
        one.push_back(Pair("url", info.url));
        one.push_back(Pair("address", info.address));
        one.push_back(Pair("txid", info.txHash.ToString()));
        result.push_back(one);
    }
    return result;
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
            + HelpExampleCli("sendtoaddress", "\"tokenID\", \"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 0.1")
            + HelpExampleCli("sendtoaddress", "\"tokenID\", \"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 0.1 \"donation\" \"seans outpost\"")
            + HelpExampleCli("sendtoaddress", "\"tokenID\", \"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 0.1 \"\" \"\" true")
            + HelpExampleRpc("sendtoaddress", "\"tokenID\", \"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\", 0.1, \"donation\", \"seans outpost\"")
        );

    LOCK2(cs_main, pwalletMain->cs_wallet);

    CTokenID tokenID;
    tokenID.SetHex(request.params[0].get_str());

    CTokenInfo tokenInfo;
    if ( !pTokenInfos->GetTokenInfo(tokenID, tokenInfo)){
        throw JSONRPCError(RPC_TOKEN_NOT_FOUND, "Token Id Not Found");
    }

    CBitcoinAddress address(request.params[1].get_str());
    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Bitcoin address");

    // Amount
    CAmount nAmount = AmountFromValue(request.params[2]);
    if (nAmount <= 0)
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount for send`");

    // Wallet comments
    CWalletTx wtx;
    if (request.params.size() > 3 && !request.params[3].isNull() && !request.params[3].get_str().empty())
        wtx.mapValue["comment"] = request.params[3].get_str();
    if (request.params.size() > 4 && !request.params[4].isNull() && !request.params[4].get_str().empty())
        wtx.mapValue["to"]      = request.params[4].get_str();

//    bool fSubtractFeeFromAmount = false;
//    if (request.params.size() > 5)
//        fSubtractFeeFromAmount = request.params[5].get_bool();

    EnsureWalletIsUnlocked();


    uint256 txid;
    std::string strFailReason;
    CTokenSend tSend;
    bool ret = tSend.sendToken(address, tokenID, nAmount, txid,  strFailReason);

    return ret ? txid.ToString() : strFailReason;
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
