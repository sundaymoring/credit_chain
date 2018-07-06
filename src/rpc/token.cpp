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

UniValue issuetoken(const JSONRPCRequest& request){
    if (request.fHelp || request.params.size() < 7)
        throw runtime_error(
            "issuetoken assetaddress type amount name\n"
            "\nReturns the txid when issue token success.\n"

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
                + HelpExampleCli("issuetoken", "\"3Ck2kEGLJtZw9ENj2tameMCtS3HB7uRar3\" 1 1000000 \"btc\" \"bitcoin\" \"bitcoin is first Cryptocurrency\" \"https://bitcoin.org\"")
            );

    LOCK2(cs_main, pwalletMain->cs_wallet);

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
    CTokenIssue tIssue(isDivisible, amount, symbol, fullName, description, url);
    bool ret = tIssue.issueToken(tokenAddress, txid,  strFailReason);

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
            "        \"amount\": n,\n"
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

    LOCK(cs_main);

    UniValue result(UniValue::VARR);

    std::vector<CTokenInfo> infos = pTokenInfos->ListTokenInfos();
    BOOST_FOREACH (const auto& info, infos){
        UniValue entry(UniValue::VOBJ);
        entry.push_back(Pair("id", info.tokenID.ToString()));
        entry.push_back(Pair("amount", info.amount));
        entry.push_back(Pair("type", info.type));
        entry.push_back(Pair("symbol", info.symbol));
        entry.push_back(Pair("fullName", info.fullName));
        entry.push_back(Pair("description", info.description));
        entry.push_back(Pair("url", info.url));
        entry.push_back(Pair("address", info.address));
        entry.push_back(Pair("txid", info.txHash.ToString()));
        result.push_back(entry);
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


UniValue gettokenbalance(const JSONRPCRequest& request)
{
    if (!EnsureWalletIsAvailable(request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() > 4 )
        throw runtime_error(
            "getbalance ( \"account\" minconf include_watchonly )\n"
            "\nIf account is not specified, returns the server's total available balance.\n"
            "If account is specified (DEPRECATED), returns the balance in the account.\n"
            "Note that the account \"\" is not the same as leaving the parameter out.\n"
            "The server total may be different to the balance in the default \"\" account.\n"
            "\nArguments:\n"
            "1. \"tokenid\"         (string, optional) the selected tokenid\n"
            "2. \"account\"         (string, optional) DEPRECATED. The account string may be given as a\n"
            "                     specific account name to find the balance associated with wallet keys in\n"
            "                     a named account, or as the empty string (\"\") to find the balance\n"
            "                     associated with wallet keys not in any named account, or as \"*\" to find\n"
            "                     the balance associated with all wallet keys regardless of account.\n"
            "                     When this option is specified, it calculates the balance in a different\n"
            "                     way than when it is not specified, and which can count spends twice when\n"
            "                     there are conflicting pending transactions (such as those created by\n"
            "                     the bumpfee command), temporarily resulting in low or even negative\n"
            "                     balances. In general, account balance calculation is not considered\n"
            "                     reliable and has resulted in confusing outcomes, so it is recommended to\n"
            "                     avoid passing this argument.\n"
            "3. minconf           (numeric, optional, default=1) Only include transactions confirmed at least this many times.\n"
            "4. include_watchonly (bool, optional, default=false) Also include balance in watch-only addresses (see 'importaddress')\n"
            "\nResult:\n"
            "amount              (numeric) The total amount in " + CURRENCY_UNIT + " received for this account.\n"
            "\nExamples:\n"
            "\nThe total amount in the wallet\n"
            + HelpExampleCli("getbalance", "") +
            "\nThe total amount in the wallet at least 5 blocks confirmed\n"
            + HelpExampleCli("getbalance", "\"*\" 6") +
            "\nAs a json rpc call\n"
            + HelpExampleRpc("getbalance", "\"*\", 6")
        );

    LOCK2(cs_main, pwalletMain->cs_wallet);

    std::map<CTokenID, CAmount> mTokenBalances;
    if (request.params.size() == 0){
        mTokenBalances = pwalletMain->GetAllTokenBalance();
    }

    CTokenID tokenID = TOKENID_ZERO;

    if (request.params.size() > 0)
        tokenID.SetHex(request.params[0].getValStr());

    if (request.params.size() == 1)
        mTokenBalances[tokenID] = pwalletMain->GetBalance(tokenID);

    int nMinDepth = 1;
    if (request.params.size() > 2)
        nMinDepth = request.params[2].get_int();
    isminefilter filter = ISMINE_SPENDABLE;
    if(request.params.size() > 3)
        if(request.params[3].get_bool())
            filter = filter | ISMINE_WATCH_ONLY;

    if (request.params.size() > 1){
        if (request.params[1].get_str() == "*") {
            // Calculate total balance in a very different way from GetBalance().
            // The biggest difference is that GetBalance() sums up all unspent
            // TxOuts paying to the wallet, while this sums up both spent and
            // unspent TxOuts paying to the wallet, and then subtracts the values of
            // TxIns spending from the wallet. This also has fewer restrictions on
            // which unconfirmed transactions are considered trusted.
            CAmount nBalance = 0;
            for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
            {
                const CWalletTx& wtx = (*it).second;
                if (!CheckFinalTx(wtx) || wtx.GetBlocksToMaturity() > 0 || wtx.GetDepthInMainChain() < 0)
                    continue;

                CAmount allFee;
                string strSentAccount;
                list<COutputEntry> listReceived;
                list<COutputEntry> listSent;
                wtx.GetAmounts(listReceived, listSent, allFee, strSentAccount, filter);
                if (wtx.GetDepthInMainChain() >= nMinDepth)
                {
                    BOOST_FOREACH(const COutputEntry& r, listReceived)
                    {
                        if (r.tokenID == tokenID)
                            nBalance += r.tokenAmount;
                    }
                }
                BOOST_FOREACH(const COutputEntry& s, listSent)
                {
                    if (s.tokenID == tokenID)
                        nBalance -= s.tokenAmount;
                }
            }
            mTokenBalances[tokenID] = nBalance;
        }else{
            string strAccount = request.params[1].get_str();
            if (strAccount == "*")
                throw JSONRPCError(RPC_WALLET_INVALID_ACCOUNT_NAME, "Invalid account name");

            CAmount nBalance = pwalletMain->GetAccountBalance(strAccount, nMinDepth, filter, tokenID);
            mTokenBalances[tokenID] = nBalance;
        }
    }

    UniValue result(UniValue::VARR);
    BOOST_FOREACH (const auto& t, mTokenBalances){
        UniValue entry(UniValue::VOBJ);
        entry.push_back(Pair("tokenid", t.first.ToString()));
        entry.push_back(Pair("tokenvalue", t.second));
        result.push_back(entry);
    }
    return result;
}

bool getAddressesFromParams(const UniValue& params, std::vector<std::pair<uint160, int> > &addresses);

UniValue getaddresstokenbalance(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw runtime_error(
            "getaddresstokenbalance\n"
            "\nReturns the balance for an address(es) (requires addressindex to be enabled).\n"
            "\nArguments:\n"
            "{\n"
            "  \"addresses\"\n"
            "    [\n"
            "      \"address\"  (string) The base58check encoded address\n"
            "      ,...\n"
            "    ]\n"
            "}\n"
            "\nResult:\n"
            "[\n"
            "   {\n"
            "       \"tokenid\"   (string) The token id\n"
            "       \"balance\"  (string) The current balance in satoshis\n"
            "       \"received\"  (string) The total number of satoshis received (including change)\n"
            "   }\n"
            "   ...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("getaddressbalance", "'{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"]}'")
            + HelpExampleRpc("getaddressbalance", "{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"]}")
        );

    LOCK(cs_main);

    std::vector<std::pair<uint160, int> > addresses;

    if (!getAddressesFromParams(request.params, addresses)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }

    std::vector<std::pair<CAddressIndexKey, COutValue>> addressIndex;

    for (std::vector<std::pair<uint160, int> >::iterator it = addresses.begin(); it != addresses.end(); it++) {
        if (!GetAddressIndex((*it).first, (*it).second, addressIndex)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
        }
    }

    // map value first is recived, map value second is balanced;
    std::map<CTokenID, std::pair<CAmount,CAmount>> tokenValue;

    for (std::vector<std::pair<CAddressIndexKey, COutValue> >::const_iterator it=addressIndex.begin(); it!=addressIndex.end(); it++) {
        if (it->second.tokenID == TOKENID_ZERO){
            continue;
        }
        if (tokenValue.find(it->second.tokenID) == tokenValue.end()){
            tokenValue[it->second.tokenID] = std::make_pair(0,0);
        }
        if (it->second.nTokenValue > 0) {
            tokenValue[it->second.tokenID].first += it->second.nTokenValue;
        }
        tokenValue[it->second.tokenID].second += it->second.nTokenValue;
    }


    UniValue result(UniValue::VARR);
    BOOST_FOREACH (const auto& t, tokenValue){
        UniValue entry(UniValue::VOBJ);
        entry.push_back(Pair("tokenid", t.first.ToString()));
        CTokenInfo info;
        if (pTokenInfos->GetTokenInfo(t.first, info))
            entry.push_back(Pair("symbol", info.symbol));
        entry.push_back(Pair("balance", t.second.second));
        entry.push_back(Pair("received", t.second.first));
        result.push_back(entry);
    }
    return result;
}


UniValue getreceivedtokenbyaddress(const JSONRPCRequest& request)
{
    if (!EnsureWalletIsAvailable(request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() < 1 || request.params.size() > 2)
        throw runtime_error(
            "getreceivedbyaddress \"address\" ( minconf )\n"
            "\nReturns the total amount received by the given address in transactions with at least minconf confirmations.\n"
            "\nArguments:\n"
            "1. \"address\"         (string, required) The bitcoin address for transactions.\n"
            "2. minconf             (numeric, optional, default=1) Only include transactions confirmed at least this many times.\n"
            "\nResult:\n"
            "[\n"
            "   {\n"
            "       \"tokenid\"  (string) The token ID at this address\n"
            "       \"amount\"   (numeric) The total amount in tokenID received at this address.\n"
            "   }\n"
            "   ...\n"
            "]\n"
            "\nExamples:\n"
            "\nThe amount from transactions with at least 1 confirmation\n"
            + HelpExampleCli("getreceivedbyaddress", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XX\"") +
            "\nThe amount including unconfirmed transactions, zero confirmations\n"
            + HelpExampleCli("getreceivedbyaddress", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XX\" 0") +
            "\nThe amount with at least 6 confirmation, very safe\n"
            + HelpExampleCli("getreceivedbyaddress", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XX\" 6") +
            "\nAs a json rpc call\n"
            + HelpExampleRpc("getreceivedbyaddress", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XX\", 6")
       );

    LOCK2(cs_main, pwalletMain->cs_wallet);

    // Bitcoin address
    CBitcoinAddress address = CBitcoinAddress(request.params[0].get_str());
    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Bitcoin address");
    CScript scriptPubKey = GetScriptForDestination(address.Get());
    if (!IsMine(*pwalletMain, scriptPubKey))
        return ValueFromAmount(0);

    // Minimum confirmations
    int nMinDepth = 1;
    if (request.params.size() > 1)
        nMinDepth = request.params[1].get_int();

    // Tally
    std::map<CTokenID, CAmount> mTokenAmount;

    for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
    {
        const CWalletTx& wtx = (*it).second;
        if (wtx.IsCoinBase() || !CheckFinalTx(*wtx.tx))
            continue;

        BOOST_FOREACH(const CTxOut& txout, wtx.tx->vout)
            if (txout.scriptPubKey == scriptPubKey && txout.tokenID != TOKENID_ZERO)
                if (wtx.GetDepthInMainChain() >= nMinDepth){
                    if (mTokenAmount.find(txout.tokenID) == mTokenAmount.end())
                        mTokenAmount[txout.tokenID] = 0;
                    mTokenAmount[txout.tokenID] += txout.nTokenValue;
                }
    }

    UniValue result(UniValue::VARR);
    BOOST_FOREACH (const auto& t, mTokenAmount){
        UniValue entry(UniValue::VOBJ);
        entry.push_back(Pair("tokenid", t.first.ToString()));
        CTokenInfo info;
        if (pTokenInfos->GetTokenInfo(t.first, info))
            entry.push_back(Pair("symbol", info.symbol));
        entry.push_back(Pair("amout", t.second));
        result.push_back(entry);
    }
    return result;
}

UniValue getunconfirmedtokenbalance(const JSONRPCRequest &request)
{
    if (!EnsureWalletIsAvailable(request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() > 0)
        throw runtime_error(
                "getunconfirmedbalance\n"
                "Returns the server's total unconfirmed balance\n");

    LOCK2(cs_main, pwalletMain->cs_wallet);

    std::map<CTokenID, CAmount> mTokenAmount = pwalletMain->GetUnconfirmedTokenBalance();

    UniValue result(UniValue::VARR);
    BOOST_FOREACH (const auto& t, mTokenAmount){
        UniValue entry(UniValue::VOBJ);
        entry.push_back(Pair("tokenid", t.first.ToString()));
        CTokenInfo info;
        if (pTokenInfos->GetTokenInfo(t.first, info))
            entry.push_back(Pair("symbol", info.symbol));
        entry.push_back(Pair("amout", t.second));
        result.push_back(entry);
    }
    return result;
}


static const CRPCCommand commands[] =
{ //  category      name                            actor (function)                okSafeMode
  //  -----------   ----------------------------    ---------------------------     ----------
    { "token",      "issuetoken",                   &issuetoken,                    false,  {"asset_address","type","amount","name"} },
    { "token",      "listtokens",                   &listtokens,                    false,  {} },
    { "token",      "sendtoaddresstoken",           &sendtokentoaddress,            false,  {"toaddress", "tokenID", "amount"} },
    { "token",      "gettokenbalance",              &gettokenbalance,               false,  {"tokenid","account","minconf","include_watchonly"} },
    { "token",      "getaddresstokenbalance",       &getaddresstokenbalance,        false,  {"verbose"} },
    { "token",      "getreceivedtokenbyaddress",    &getreceivedtokenbyaddress,     false,  {"address","minconf"} },
    { "token",      "getunconfirmedtokenbalance",   &getunconfirmedtokenbalance,    false,  {} },
};

void RegisterTokenRPCCommands(CRPCTable &t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
