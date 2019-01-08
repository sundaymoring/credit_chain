#ifndef DPOS_H
#define DPOS_H

#include "chain.h"
#include "pubkey.h"
#include "key.h"
#include "consensus/validation.h"
#include "coins.h"

#include <boost/thread/shared_mutex.hpp>

const CAmount nMinDPoSFee = 1 * COIN;
const int nMaxConfirmBlockCount = 2;
struct IrreversibleBlockInfo{
    int64_t heights[nMaxConfirmBlockCount];
    uint256 hashs[nMaxConfirmBlockCount];
    std::map<int64_t, uint256> mapHeightHash;

    IrreversibleBlockInfo()
    {
        for(auto i = 0; i < nMaxConfirmBlockCount; ++i) {
            heights[i] = -1;
        }
    }
};

class CDPoSBlockInfo{
public:
    CDPoSBlockInfo(){
        delegateListHash.SetNull();
        t = 0;
    }

    CPubKey delegatePubKey;
    uint256 delegateListHash;
    time_t t;
};

struct Delegate{
    CKeyID keyid;
    uint64_t votes;
    Delegate(){votes = 0;}
    Delegate(const CKeyID& keyid, uint64_t votes) {this->keyid = keyid; this->votes = votes;}


    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(keyid);
        READWRITE(votes);
    }
};

struct DelegateList{
    std::vector<Delegate> delegates;

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(delegates);
    }
};

// HTODO consider minHoldBalance of delegate address
class DPOS
{
public:
    static DPOS& GetInstance();

    bool IsMining(CDPoSBlockInfo& dposBlockInfo, const CKeyID& keyid, time_t t);

    static bool CheckTransaction(const CCoinsViewCache& view, const CTransaction& tx, CValidationState &state);
    bool CheckBlock(const CCoinsViewCache& view, const CBlock &block, CValidationState &state);
    bool CheckBlock(const CCoinsViewCache& view, const CBlockIndex& blockindex, CValidationState &state);

    uint64_t GetStartTime() {return nDposStartTime;}
    void SetStartTime(uint64_t t) {nDposStartTime = t;}
    uint32_t GetStartHeight() {return nDposStartHeight;}

    bool UpdateDelegateList(const int height);
    bool GetDelegateList(const int height, DelegateList& cDelegateInfo);

private:
    DPOS();

    bool GetAddressKeyID(CKeyID& keyid, const std::string& strAddress);

    uint64_t GetLoopIndex(uint64_t time);
    uint32_t GetDelegateIndex(uint64_t time);

    uint256 DelegateListToHash(const DelegateList& cDelegateInfo);
    bool CheckDelegateListHash(const int blockHeight, const CBlock &block);
    bool GetDelegateListHash(uint256& delegateHash, CBlockIndex* pBlockIndex);

    std::vector<Delegate> SortDelegate(const std::vector<Delegate>& delegates);

    // HTODO add func `void ProcessIrreversibleBlock(int64_t height, uint256 hash);`

private:
    int nMaxDelegateNumber;
    int nBlockIntervalTime;            //seconds
    int nDposStartHeight;
    uint32_t nDposStartTime;  // start time is mapblockindex[nDposStartHeight-1] time
    std::string strDelegateAddress;
    IrreversibleBlockInfo cIrreversibleBlockInfo;
    boost::shared_mutex lockIrreversibleBlockInfo;
};

#endif // DPOS_H
