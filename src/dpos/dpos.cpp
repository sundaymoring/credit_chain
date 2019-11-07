#include "dpos.h"
#include "validation.h"
#include "chainparams.h"
#include "script/standard.h"
#include "base58.h"
#include "dpos/vote.h"

#include <boost/thread.hpp>
//#include <boost/tuple/tuple.hpp>

typedef boost::shared_lock<boost::shared_mutex> read_lock;
typedef boost::unique_lock<boost::shared_mutex> write_lock;

DPOS::DPOS()
{
    nMaxDelegateNumber = 3;
    nBlockIntervalTime = 2;
    nDposStartHeight = Params().LastPOWBlock()+1;

    if(chainActive.Height() >= nDposStartHeight - 1) {
        SetStartTime(chainActive[nDposStartHeight -1]->nTime);
    } else {
        nDposStartTime = 0;
    }

    //HTODO strDelegateAddress not set before nDposStartHeight
    strDelegateAddress = "KUvYTrDtRK866C5bTohzga3J2nX7tonDhz";
}

DPOS& DPOS::GetInstance() {
    static DPOS dpos;
    return dpos;
}

bool DPOS::IsMining(CDPoSBlockInfo& dposBlockInfo, const CKeyID& keyid, time_t t)
{
    CBlockIndex* pBlockIndex = chainActive.Tip();
    if(pBlockIndex->nHeight < nDposStartHeight - 1) {
        if(strDelegateAddress == strDelegateAddress) {
            return true;
        } else {
            return false;
        }
    }

    dposBlockInfo.t = t;

    uint64_t nCurrentLoopIndex = GetLoopIndex(t);
    uint32_t nCurrentDelegateIndex = GetDelegateIndex(t);
    uint64_t nPrevLoopIndex = GetLoopIndex(pBlockIndex->nTime);
    uint32_t nPrevDelegateIndex = GetDelegateIndex(pBlockIndex->nTime);

    //cDelegateInfo = DPoS::GetNextDelegates();
    DelegateList cDelegateList;
    int nextHeight = pBlockIndex->nHeight + 1;
    if (GetDelegateList(nextHeight, cDelegateList) == false){
        LogPrint("IsMining", "GetCurrentDelegates failed");
        return false;
    }

    if(pBlockIndex->nHeight == nDposStartHeight - 1) {
        if(cDelegateList.delegates[nCurrentDelegateIndex].keyid == keyid) {
            LogPrintf("IsMining nCurrentDelegateIndex=%lu", nCurrentDelegateIndex);
            dposBlockInfo.delegateListHash = DelegateListToHash(cDelegateList);
        } else {
            return false;
        }
        return true;
    }

    if(nCurrentLoopIndex > nPrevLoopIndex) {
        if(cDelegateList.delegates[nCurrentDelegateIndex].keyid == keyid) {
            dposBlockInfo.delegateListHash = DelegateListToHash(cDelegateList);
        } else {
            return false;
        }
        return true;
    } else if(nCurrentLoopIndex == nPrevLoopIndex && nCurrentDelegateIndex > nPrevDelegateIndex) {
        uint256 blockDelegatesHash;
        if (GetDelegateListHash(blockDelegatesHash, pBlockIndex)){
            if (nCurrentDelegateIndex + 1> cDelegateList.delegates.size()){
                return false;
            }
            if (cDelegateList.delegates[nCurrentDelegateIndex].keyid == keyid){
                dposBlockInfo.delegateListHash = DelegateListToHash(cDelegateList);
                return blockDelegatesHash == dposBlockInfo.delegateListHash;
            }
        } else {
            return false;
        }
    } else {
        return false;
    }

    return false;
}

bool DPOS::CheckTransaction(const CCoinsViewCache& view, const CTransaction &tx, CValidationState &state){
    if (tx.IsTxHaveDPoS()){
        CAmount valueIn = view.GetValueIn(tx);
        CAmount valueOut = tx.GetValueOut();
        if (valueIn-valueOut < nMinDPoSFee){
            return state.Invalid(false, REJECT_INSUFFICIENTFEE, "insufficient DPoS fee", strprintf("valueIn:%d  valueOut:%d", valueIn, valueOut));
        }
    }
    return true;
}

bool DPOS::CheckBlock(const CCoinsViewCache& view, const CBlock &block, CValidationState &state){
    if(block.hashPrevBlock.IsNull()) {
        return true;
    }

    BlockMap::iterator miSelf = mapBlockIndex.find(block.hashPrevBlock);
    if(miSelf == mapBlockIndex.end()) {
        return state.DoS(100, error("%s: prev block not found, hash: ", __func__, block.hashPrevBlock.ToString().c_str()), 0, "bad-prevblk");
    }

    CBlockIndex* pPrevBlockIndex = miSelf->second;

    int64_t nBlockHeight = pPrevBlockIndex->nHeight + 1;
    if (nBlockHeight < nDposStartHeight)
        return true;

    // check sign
    if (! block.delegatorPubKey.Verify(block.GetHash(), block.vchBlockSig))
    {
        return state.Invalid(false, REJECT_INVALID, "bad DPoS block signature");
    }

    for (const auto& tx : block.vtx ){
        if (!DPOS::CheckTransaction(view, *tx, state))
            return false;
    }

    // HTODO considor starttime ???
    if(nDposStartTime == 0 && chainActive.Height() >= nDposStartHeight - 1) {
        SetStartTime(chainActive[nDposStartHeight -1]->nTime);
    }

    // HTODO height < nDposStartHeight, only strDelegateAddress can be coinbase address, unreasonable ??

    uint64_t nCurrentLoopIndex = GetLoopIndex(block.nTime);
    uint32_t nCurrentDelegateIndex = GetDelegateIndex(block.nTime);
    uint64_t nPrevLoopIndex = 0;
    uint32_t nPrevDelegateIndex = 0;

    nPrevLoopIndex = GetLoopIndex(pPrevBlockIndex->nTime);
    nPrevDelegateIndex = GetDelegateIndex(pPrevBlockIndex->nTime);

    bool ret = false;

    if(nBlockHeight == nDposStartHeight) {
        if(CheckDelegateListHash(nBlockHeight, block) == false) {
            return ret;
        }
    } else if(nCurrentLoopIndex < nPrevLoopIndex) {
        return state.Invalid(false, REJECT_INVALID, strprintf("%s nCurrentLoopIndex < nCurrentLoopIndex", __func__));
    } else if(nCurrentLoopIndex > nPrevLoopIndex) {
        if(CheckDelegateListHash(nBlockHeight, block) == false) {
            return ret;
        }
//        ProcessIrreversibleBlock(nBlockHeight, block.GetHash());
    } else if(nCurrentLoopIndex == nPrevLoopIndex) {
        if(nCurrentDelegateIndex <= nPrevDelegateIndex) {
            return state.Invalid(false, REJECT_INVALID, strprintf("%s nCurrentDelegateIndex <= nPrevDelegateIndex, pretime:%u", __func__, pPrevBlockIndex->nTime));
        }

        uint256 preBlockDelegateListHash;
        GetDelegateListHash(preBlockDelegateListHash, pPrevBlockIndex);
        if (preBlockDelegateListHash != block.delegatorListHash){
            return state.Invalid(false, REJECT_INVALID, strprintf("%s preBlockDelegateListHash != block.delegatorListHash at height:%d", __func__, nBlockHeight));
        }
    }

    // check delegate index is right
    DelegateList cDelegateList;
    if (GetDelegateList(nBlockHeight, cDelegateList) == false){
        return state.DoS(100, error("%s: delegateInfo not found at height:%d", __func__, nBlockHeight), 0, "bad-delegateinfo");
    }
    CKeyID delegateKeyID = block.delegatorPubKey.GetID();
    if(nCurrentDelegateIndex < cDelegateList.delegates.size()
        && cDelegateList.delegates[nCurrentDelegateIndex].keyid == delegateKeyID) {
        ret = true;
    } else {
        return state.Invalid(false, REJECT_INVALID, strprintf("%s GetDelegateID blockhash:%s", __func__, block.ToString().c_str()));
    }

    return ret;
}

bool DPOS::CheckBlock(const CCoinsViewCache& view, const CBlockIndex &blockindex, CValidationState &state)
{
    CBlock block;
    if(ReadBlockFromDisk(block, &blockindex, Params().GetConsensus()) == false) {
        return error("DPoS CheckBlock(): *** ReadBlockFromDisk failed at %d, hash=%s", blockindex.nHeight, blockindex.GetBlockHash().ToString());
    }

    if(chainActive.Height() == nDposStartHeight - 1) {
        SetStartTime(chainActive[nDposStartHeight -1]->nTime);
    }
    bool ret = CheckBlock(view, block, state);
    return ret;
}

// save delegates at height {nDposStartHeight-1 + n * nMaxDelegateNumber} n(0...n)
bool DPOS::UpdateDelegateList(const int height) {
    int nStartUpdateHeight = nDposStartHeight;
    if (0 != (height-nStartUpdateHeight) % nMaxDelegateNumber ){
        return true;
    }

    uint64_t nMinHoldBalance = 0;
    std::vector<Delegate> vecDelegate = Vote::GetInstance().GetTopDelegateList(nMinHoldBalance, nMaxDelegateNumber-1);

    // insert default delegate address
    Delegate delegate;
    if (GetAddressKeyID(delegate.keyid, strDelegateAddress) == false){
        return false;
    }
    delegate.votes = 7;
    vecDelegate.insert(vecDelegate.begin(), delegate);

    vecDelegate.resize(nMaxDelegateNumber);

    DelegateList cDelegateList;
    cDelegateList.delegates = SortDelegate(vecDelegate);

    return pDPoSDb->WriteCurrentDelegates(height, cDelegateList);
}

bool DPOS::GetDelegateList(const int height, DelegateList& cDelegateList){
    int nStartUpdateHeight = nDposStartHeight;
    int nSaveHeight = (height -nDposStartHeight) / nMaxDelegateNumber * nMaxDelegateNumber + nStartUpdateHeight;

    return pDPoSDb->GetCurrentDelegates(nSaveHeight, cDelegateList);
}

bool DPOS::GetAddressKeyID(CKeyID &keyid, const std::string &strAddress){
    CBitcoinAddress addr(strAddress);
    if(addr.GetKeyID(keyid) == false) {
        LogPrint("IsMining", "%s: get address(%s) keyid failed\n", __func__, strAddress.c_str());
        return false;
    }
    return true;
}

uint256 DPOS::DelegateListToHash(const DelegateList& cDelegateList){
    std::string strDelegates;
    for (const auto& d : cDelegateList.delegates){
        strDelegates += d.keyid.ToString();
    }

    return Hash(strDelegates.begin(), strDelegates.end());
}


uint64_t DPOS::GetLoopIndex(uint64_t time)
{
    if(time < nDposStartTime) {
        return 0;
    } else {
        return (time - nDposStartTime) / (nMaxDelegateNumber * nBlockIntervalTime);
    }
}

uint32_t DPOS::GetDelegateIndex(uint64_t time)
{
    if(time < nDposStartTime) {
        return 0;
    } else {
        return (time - nDposStartTime) % (nMaxDelegateNumber * nBlockIntervalTime) / nBlockIntervalTime;
    }
}

bool DPOS::CheckDelegateListHash(const int blockHeight, const CBlock &block)
{
    DelegateList cNextDelegateList;
    if (GetDelegateList(blockHeight, cNextDelegateList) == false){
        LogPrintf("%s GetDelegates failed", __func__);
        return false;
    }

    return block.delegatorListHash == DelegateListToHash(cNextDelegateList);
}

bool DPOS::GetDelegateListHash(uint256& delegateHash, CBlockIndex* pBlockIndex)
{
    bool ret = false;
    uint64_t nLoopIndex = GetLoopIndex(pBlockIndex->nTime);
    while(pBlockIndex) {
        if(pBlockIndex->nHeight == nDposStartHeight || GetLoopIndex(pBlockIndex->pprev->nTime) < nLoopIndex) {
            CBlock block;
            if(ReadBlockFromDisk(block, pBlockIndex, Params().GetConsensus())) {
                delegateHash = block.delegatorListHash;
                ret = true;
            }
            break;
        }

        pBlockIndex = pBlockIndex->pprev;
    }

    return ret;
}

std::vector<Delegate> DPOS::SortDelegate(const std::vector<Delegate>& delegates)
{
    std::vector<Delegate> result;
    std::vector<const Delegate*> tmp(delegates.size(), NULL);
    // HTODO set real rand
    uint64_t rand = 987654321123456823;

    for(unsigned int i=0; i < delegates.size(); i++) {
        uint64_t index = 0;
        if(delegates[i].votes) {
            index = (rand / delegates[i].votes) % delegates.size();
        } else {
            index = (rand / 1) % delegates.size();
        }

        while(1) {
            uint32_t id = (uint32_t)(index++) % delegates.size();
            if(tmp[id] == 0) {
                tmp[id]    = &delegates[i];
                break;
            }
        }
    }

    for(unsigned int i=0; i < delegates.size(); i++) {
        result.push_back(*tmp[i]);
    }

    return result;
}
