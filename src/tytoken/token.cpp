#include "tytoken/token.h"

#include "base58.h"
#include "wallet/coincontrol.h"
#include "wallet/wallet.h"
#include "script/script.h"
#include "consensus/validation.h"

#include "util.h"

#include <vector>

/**
 * Pushes bytes to the end of a vector.
 */
#define PUSH_BACK_BYTES(vector, value)\
    vector.insert(vector.end(), reinterpret_cast<unsigned char *>(&(value)),\
    reinterpret_cast<unsigned char *>(&(value)) + sizeof((value)));

std::vector<unsigned char> CreatePayload_IssuanceFixed(uint16_t propertyType, uint64_t amount, std::string name)
{
    std::vector<unsigned char> payload;
    uint16_t messageType = 50;

    //TODO int convert net byte serlize
    if (name.size() > 255) name = name.substr(0,255);
    payload.push_back('T');
    payload.push_back('T');
    PUSH_BACK_BYTES(payload, messageType)
    PUSH_BACK_BYTES(payload, propertyType);
    PUSH_BACK_BYTES(payload, amount);
    size_t nameLen = name.length();
    PUSH_BACK_BYTES(payload, nameLen);
    payload.insert(payload.end(), name.begin(), name.end());

    return payload;
}

// This function requests the wallet create an Omni transaction using the supplied parameters and payload
bool WalletTxBuilder(const std::string& assetAddress, int64_t referenceAmount, const std::vector<unsigned char>& data, uint256& txid, std::string& rawHex, bool commit)
{
//#ifdef ENABLE_WALLET
    if (pwalletMain == NULL) return false;

    CWalletTx wtxNew;
    int64_t nFeeRet = 0;
    int nChangePosInOut = -1;
    std::string strFailReason;
    std::vector<std::pair<CScript, int64_t> > vecSend;
    CReserveKey reserveKey(pwalletMain);

    //TODO bitcoin sendfrom assetAddress, and token in assetAddress. now bitcoin sendfrom other address.

    // Encode the data outputs
    CBitcoinAddress addr = CBitcoinAddress(assetAddress);
    CScript script;
    script << OP_RETURN << data;
    vecSend.push_back(std::make_pair(script, 0));
    script = GetScriptForDestination(addr.Get());
    vecSend.push_back(std::make_pair(script, 1*COIN));


    std::vector<CRecipient> vecRecipients;
    for (size_t i = 0; i < vecSend.size(); ++i) {
        const std::pair<CScript, int64_t>& vec = vecSend[i];
        CRecipient recipient = {vec.first, vec.second, false};
        vecRecipients.push_back(recipient);
    }

    // Ask the wallet to create the transaction (note mining fee determined by Bitcoin Core params)
    if (!pwalletMain->CreateTransaction(vecRecipients, wtxNew, reserveKey, nFeeRet, nChangePosInOut, strFailReason)) {
        LogPrintf("%s: ERROR: wallet transaction creation failed: %s\n", __func__, strFailReason);
        return false;
    }

    // If this request is only to create, but not commit the transaction then display it and exit
    if (!commit) {
//        rawHex = EncodeHexTx(wtxNew);
        return true;
    } else {
        // Commit the transaction to the wallet and broadcast)
        LogPrintf("%s: %s; nFeeRet = %d\n", __func__, wtxNew.tx->ToString(), nFeeRet);
        CValidationState state;
        if (!pwalletMain->CommitTransaction(wtxNew, reserveKey, g_connman.get(), state)) return false;
        txid = wtxNew.GetHash();
        return true;
    }
//#else
//    return MP_ERR_WALLET_ACCESS;
//#endif
}
