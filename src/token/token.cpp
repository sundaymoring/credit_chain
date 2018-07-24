#include "token.h"
#include "script/standard.h"


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

    txnouttype whichType;
    std::vector<unsigned char> vPushData;

    if (!Solver(tx.vout[0].scriptPubKey, whichType, vPushData)){
        return TTC_NONE;
    }

    if (whichType == TX_TOKEN){
        if (pTokenData)
            *pTokenData = vPushData;
        return tx.vout[0].scriptPubKey[3];
    }

    return TTC_NONE;
}
