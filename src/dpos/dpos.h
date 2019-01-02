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

struct DelegateInfo{
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

    static bool CheckTransaction(const CCoinsViewCache& view, const CTransaction& tx, CValidationState &state);
    bool CheckBlock(const CCoinsViewCache& view, const CBlockIndex& blockindex, CValidationState &state);
    bool IsMining(uint160& delegatesHash, const std::string& strDelegateAddress, time_t t);

    static CScript DelegateInfoToScript(const uint160& delegatesHash, const CKey& delegatekey, time_t t);
    static bool ScriptToDelegateInfo(uint160& delegatesHash, time_t &t, std::vector<unsigned char>* pvctPublicKey, const CScript& script);

    uint64_t GetStartTime() {return nDposStartTime;}
    void SetStartTime(uint64_t t) {nDposStartTime = t;}

    uint32_t GetStartHeight() {return nDposStartHeight;}

    uint160 DelegatesToHash(const DelegateInfo& cDelegateInfo);

    bool UpdateDelegates(const int height);
    bool GetDelegates(const int height, DelegateInfo& cDelegateInfo);

private:
    DPOS();

    static bool GetDelegateID(CKeyID& keyid, const std::string& address);
    static bool GetDelegateID(CKeyID& keyid, const CBlock& block);
    static std::string GetDelegateAddress(const CBlock& block);
    static bool GetBlockDelegate(uint160& delegateHash, const CBlock& block);

    bool CheckBlock(const CCoinsViewCache& view, const CBlock &block, CValidationState &state);
    static bool CheckCoinbase(const CTransaction& tx);
//    bool CheckTransactionVersion(const CBlock& block) {return true;}

    uint64_t GetLoopIndex(uint64_t time);
    uint32_t GetDelegateIndex(uint64_t time);

    bool CheckBlockDelegateHash(const int blockHeight, const CBlock &block);
    bool GetBlockDelegates(uint160& delegateHash, CBlockIndex* pBlockIndex);
    bool GetBlockDelegates(uint160& delegateHash, const CBlock& block);

    DelegateInfo GetNextDelegates(int64_t t);
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
