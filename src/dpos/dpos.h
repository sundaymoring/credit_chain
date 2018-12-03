#ifndef DPOS_H
#define DPOS_H

#include "chain.h"
#include "pubkey.h"

#include <boost/thread/shared_mutex.hpp>

// HTODO what difference of dposStartHeight and forkHeight in lbtc ?

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
};

struct DelegateInfo{
    std::vector<Delegate> delegates;
};

class DPOS
{
public:
    DPOS();
    static DPOS& GetInstance();

    bool CheckBlock(const CBlockIndex& blockindex);
    bool IsMining(DelegateInfo& cDelegateInfo, const std::string& strDelegateAddress, time_t t);

    static bool ScriptToDelegateInfo(DelegateInfo& cDelegateInfo, time_t &t, std::vector<unsigned char>* pvctPublicKey, const CScript& script);

    static bool GetDelegateID(CKeyID& keyid, const std::string& address);
    static bool GetDelegateID(CKeyID& keyid, const CBlock& block);
    static std::string GetDelegateAddress(const CBlock& block);
    static bool GetBlockDelegate(DelegateInfo& cDelegateInfo, const CBlock& block);

    uint64_t GetStartTime() {return nDposStartTime;}
    void SetStartTime(uint64_t t) {nDposStartTime = t;}

private:

    bool CheckCoinbase(const CTransaction& tx, time_t t, int64_t height);
    bool CheckTransactionVersion(const CBlock& block) {return true;}
    bool CheckBlock(const CBlock &block);
    uint64_t GetLoopIndex(uint64_t time);
    uint32_t GetDelegateIndex(uint64_t time);
    void ProcessIrreversibleBlock(int64_t height, uint256 hash);
    bool CheckBlockDelegate(const CBlock& block);
    bool GetBlockDelegates(DelegateInfo& cDelegateInfo, CBlockIndex* pBlockIndex);
    bool GetBlockDelegates(DelegateInfo& cDelegateInfo, const CBlock& block);
    bool IsOnTheSameChain(const std::pair<int64_t, uint256>& first, const std::pair<int64_t, uint256>& second);
    void AddIrreversibleBlock(int64_t height, uint256 hash);
    DelegateInfo GetNextDelegates(int64_t t);
    std::vector<Delegate> SortDelegate(const std::vector<Delegate>& delegates);

private:
    int nMaxDelegateNumber;
    int nBlockIntervalTime;            //seconds
    int nDposStartHeight;
    uint32_t nDposStartTime;
    std::string strDelegateAddress;
    IrreversibleBlockInfo cIrreversibleBlockInfo;
    boost::shared_mutex lockIrreversibleBlockInfo;

    const int nFirstIrreversibleThreshold = 90;
    const int nSecondIrreversibleThreshold = 67;
    const int nMaxIrreversibleCount = 1000;
};

#endif // DPOS_H
