#include "base58.h"
#include "amount.h"
#include "rpc/server.h"
#include "tytoken/token.h"

#include "utilstrencodings.h"

#include <stdexcept>
#include <univalue.h>

using namespace std;

UniValue issureasset(const JSONRPCRequest& request){
    if (request.fHelp || request.params.size() != 4)
        throw runtime_error(
            "issureasset assetaddress type amount name\n"
            );

    std::string assetAddress =request.params[0].get_str();
    int type = request.params[1].get_int();
    CAmount amount = request.params[2].get_int64();
    std::string tokenName = request.params[3].get_str();

    // create a payload for the transaction
    vector<unsigned char> payload = CreatePayload_IssuanceFixed(uint16_t(type), amount, tokenName);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    bool ret = WalletTxBuilder(assetAddress, 0, payload, txid, rawHex);

    UniValue result(UniValue::VOBJ);
    result.push_back(Pair("result", ret?"ok":"false"));
    return result;
}

static const CRPCCommand commands[] =
{ //  category              name                      actor (function)         okSafeMode
  //  --------------------- ------------------------  -----------------------  ----------
    { "token",              "issureasset",            &issureasset,            true,  {"asset_address","type","amount","name"} },
};

void RegisterTokenRPCCommands(CRPCTable &t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
