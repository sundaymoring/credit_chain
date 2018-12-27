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

bool DPOS::CheckTransaction(const CCoinsViewCache& view, const CTransaction &tx, CValidationState &state){
    // coinbase dpos
    if (tx.IsCoinBaseDPoS()){
        return DPOS::CheckCoinbase(tx);
    } else if (tx.IsTxHaveDPoS()){
        CAmount valueIn = view.GetValueIn(tx);
        CAmount valueOut = tx.GetValueOut();
        if (valueIn-valueOut < nMinDPoSFee){
            return state.Invalid(false, REJECT_INSUFFICIENTFEE, "insufficient DPoS fee", strprintf("valueIn:%d  valueOut:%d", valueIn, valueOut));
        }
    }
    return true;
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

bool DPOS::IsMining(DelegateInfo& cDelegateInfo, const std::string& strDelegateAddress, time_t t)
{
    CBlockIndex* pBlockIndex = chainActive.Tip();
    if(pBlockIndex->nHeight < nDposStartHeight - 1) {
        if(strDelegateAddress == strDelegateAddress) {
            return true;
        } else {
            return false;
        }
    }

    uint64_t nCurrentLoopIndex = GetLoopIndex(t);
    uint32_t nCurrentDelegateIndex = GetDelegateIndex(t);
    uint64_t nPrevLoopIndex = GetLoopIndex(pBlockIndex->nTime);
    uint32_t nPrevDelegateIndex = GetDelegateIndex(pBlockIndex->nTime);

    //cDelegateInfo = DPoS::GetNextDelegates();
    CKeyID keyid;
    if(GetDelegateID(keyid, strDelegateAddress) == false) {
        LogPrint("IsMining", "GetDelegateID(%s) failed", strDelegateAddress.c_str());
        return false;
    }

    if(pBlockIndex->nHeight == nDposStartHeight - 1) {
        cDelegateInfo = DPOS::GetNextDelegates(t);
        if(cDelegateInfo.delegates[nCurrentDelegateIndex].keyid == keyid) {
            LogPrintf("IsMining nCurrentDelegateIndex=%lu", nCurrentDelegateIndex);
            return true;
        } else {
            return false;
        }
    }

    if(nCurrentLoopIndex > nPrevLoopIndex) {
        cDelegateInfo = DPOS::GetNextDelegates(t);
        if(cDelegateInfo.delegates[nCurrentDelegateIndex].keyid == keyid) {
            return true;
        } else {
            return false;
        }
    } else if(nCurrentLoopIndex == nPrevLoopIndex && nCurrentDelegateIndex > nPrevDelegateIndex) {
        DelegateInfo cCurrentDelegateInfo;
        if(GetBlockDelegates(cCurrentDelegateInfo, pBlockIndex)) {
            if(nCurrentDelegateIndex + 1 > cCurrentDelegateInfo.delegates.size()) {
                return false;
            } else if(cCurrentDelegateInfo.delegates[nCurrentDelegateIndex].keyid == keyid) {
                //cDelegateInfo.delegates.clear();
                return true;
            } else {
                return false;
            }
        } else {
            return false;
        }
    } else {
        return false;
    }

    return false;
}

//OP_DPOS delegateName PUBLICK_KEY HASH(time) SIG OP_CODE DELEGATE_IDS
CScript DPOS::DelegateInfoToScript(const DelegateInfo& cDelegateInfo, const CKey& delegatekey, time_t t){

    const std::vector<Delegate>& delegates = cDelegateInfo.delegates;
    int nDataLen = 1 + delegates.size() * 20;
    std::vector<unsigned char> data;

    if(delegates.empty() == false) {
        data.resize(nDataLen);
//        data[0] = 0x7;
        data[0] = dpos_list;

        unsigned char* pData = &data[1];
        for(unsigned int i =0; i < delegates.size(); ++i) {
            memcpy(pData, delegates[i].keyid.begin(), 20);
            pData += 20;
        }
    }

    std::vector<unsigned char> vchSig;
    std::string ts = std::to_string(t);
    delegatekey.Sign(Hash(ts.begin(), ts.end()), vchSig);

    auto pubkey = delegatekey.GetPubKey();
    std::string delegateName = Vote::GetInstance().GetDelegate(pubkey.GetID());
    CScript script;
    script.resize(3);
    script[0] = OP_DPOS;
    script[1] = 0x01;
    script[2] = DPOS_PROTOCOL_VERSION;
    if(cDelegateInfo.delegates.empty() == false) {
        script << std::vector<unsigned char>(delegateName.begin(), delegateName.end())
               << ToByteVector(pubkey)
               << std::vector<unsigned char>(ts.begin(), ts.end())
               << vchSig
               << data;
    } else {
        script << std::vector<unsigned char>(delegateName.begin(), delegateName.end())
               << ToByteVector(pubkey)
               << std::vector<unsigned char>(ts.begin(), ts.end())
               << vchSig;
    }
    return script;
}

//OP_DPOS delegateName PUBLICK_KEY HASH(time) SIG OP_CODE DELEGATE_IDS
bool DPOS::ScriptToDelegateInfo(DelegateInfo& cDelegateInfo, time_t &t, std::vector<unsigned char>* pvctPublicKey, const CScript& script)
{
    opcodetype op;
    std::vector<unsigned char> data;
    CScript::const_iterator it = script.begin();
    if(*it == OP_DPOS) {
        if (*++it != 0x01 || *++it != DPOS_PROTOCOL_VERSION)
            return false;
        ++it;
        std::vector<unsigned char> vctDelegateName;
        std::vector<unsigned char> vctPublicKey;
        std::vector<unsigned char> vctTime;
        std::vector<unsigned char> vctSig;
        data.clear();
        if( script.GetOp2(it, op, &vctDelegateName) == false
            || script.GetOp2(it, op, &vctPublicKey) == false
            || script.GetOp2(it, op, &vctTime) == false
            || script.GetOp2(it, op, &vctSig) == false
            ) {
            return false;
        }

        t = std::stoul(std::string(vctTime.begin(), vctTime.end()));

        auto hTimestamp = Hash(vctTime.begin(), vctTime.end());
        CPubKey pubkey;
        pubkey.Set(vctPublicKey.begin(), vctPublicKey.end());
        if(pubkey.Verify(hTimestamp, vctSig) == false) {
            return false;
        }

        if(pvctPublicKey) {
            *pvctPublicKey = vctPublicKey;
        }

        if(script.GetOp2(it, op, &data)) {
            if((data.size() - (1)) % (20) == 0) {
                if (data[0] != dpos_list){
                    return false;
                }
                unsigned char* pData = &data[1];
                uint32_t nDelegateNum = (data.size() - (1)) / (20);
                for(unsigned int i =0; i < nDelegateNum; ++i) {
                    std::vector<unsigned char> vct(pData, pData + 20);
                    cDelegateInfo.delegates.push_back(Delegate(CKeyID(base_blob<160>(vct)), 0));
                    pData += 20;
                }

                return true;
            }
        }
    }
    return true;
}

bool DPOS::GetDelegateID(CKeyID& keyid, const std::string& address)
{
    bool ret = false;

    CBitcoinAddress addr(address);
    if(addr.GetKeyID(keyid)) {
        ret = true;
    } else {
        LogPrint("DPos", "%s : address:%s get keyid error\n", __func__, address.c_str());
    }
    return ret;
}

bool DPOS::GetDelegateID(CKeyID& keyid, const CBlock& block)
{
    bool ret = false;
    std::string address = GetDelegateAddress(block);
    if(address.empty() == false) {
        ret = GetDelegateID(keyid, address);
    }

    return ret;
}

std::string DPOS::GetDelegateAddress(const CBlock& block)
{
    std::string address;
    auto tx = block.vtx[0];
    if(tx->IsCoinBase() && tx->vout.size() == 3) {
        opcodetype op;
        std::vector<unsigned char> vch;
        auto script = tx->vout[1].scriptPubKey;
        CScript::const_iterator it = script.begin();
        script.GetOp2(it, op, &vch);
        CPubKey pubkey(vch.begin(), vch.end());
        CKeyID keyID = pubkey.GetID();
        address = CBitcoinAddress(keyID).ToString();
    }

    return address;
}

bool DPOS::GetBlockDelegate(DelegateInfo& cDelegateInfo, const CBlock& block)
{
    bool ret = false;

    auto tx = block.vtx[0];
    if(tx->IsCoinBase() && tx->vout.size() == 3) {
        auto script = tx->vout[0].scriptPubKey;
        time_t t;
        ret = ScriptToDelegateInfo(cDelegateInfo, t, NULL, script);
    }

    return ret;
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
    if (! CheckBlockSignature(block, block.GetHash())){
        return state.Invalid(false, REJECT_INVALID, "bad DPoS block signature");
    }

    // HTODO let state return back
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
    DelegateInfo cDelegateInfo;

    if(nBlockHeight == nDposStartHeight) {
        LogPrint("HZY", strprintf("1---height=%lld nCurrentLoopIndex=%llu nPrevLoopIndex=%llu nCurrentDelegateIndex=%llu nPrevDelegateIndex=%llu\n",
                                  nBlockHeight, nCurrentLoopIndex, nPrevLoopIndex, nCurrentDelegateIndex, nPrevDelegateIndex));
        if(CheckBlockDelegate(block) == false) {
            return ret;
        }

        GetBlockDelegate(cDelegateInfo, block);
    } else if(nCurrentLoopIndex < nPrevLoopIndex) {
        return state.Invalid(false, REJECT_INVALID, strprintf("%s nCurrentLoopIndex < nCurrentLoopIndex", __func__));
    } else if(nCurrentLoopIndex > nPrevLoopIndex) {
        LogPrint("HZY", strprintf("2---height=%lld nCurrentLoopIndex=%llu nPrevLoopIndex=%llu nCurrentDelegateIndex=%llu nPrevDelegateIndex=%llu\n",
                                  nBlockHeight, nCurrentLoopIndex, nPrevLoopIndex, nCurrentDelegateIndex, nPrevDelegateIndex));
        if(CheckBlockDelegate(block) == false) {
            return ret;
        }
//        ProcessIrreversibleBlock(nBlockHeight, block.GetHash());

        GetBlockDelegate(cDelegateInfo, block);
    } else if(nCurrentLoopIndex == nPrevLoopIndex) {
        LogPrint("HZY", strprintf("3---height=%lld nCurrentLoopIndex=%llu nPrevLoopIndex=%llu nCurrentDelegateIndex=%llu nPrevDelegateIndex=%llu\n",
                                  nBlockHeight, nCurrentLoopIndex, nPrevLoopIndex, nCurrentDelegateIndex, nPrevDelegateIndex));
        if(nCurrentDelegateIndex <= nPrevDelegateIndex) {
            return state.Invalid(false, REJECT_INVALID, strprintf("%s nCurrentDelegateIndex <= nPrevDelegateIndex, pretime:%u", __func__, pPrevBlockIndex->nTime));
        }

        GetBlockDelegates(cDelegateInfo, pPrevBlockIndex);
    }

    CKeyID delegate;
    if (!GetDelegateID(delegate, block))
        return ret;
    if(nCurrentDelegateIndex < cDelegateInfo.delegates.size()
        && cDelegateInfo.delegates[nCurrentDelegateIndex].keyid == delegate) {
        ret = true;
    } else {
        return state.Invalid(false, REJECT_INVALID, strprintf("%s GetDelegateID blockhash:%s", __func__, block.ToString().c_str()));
    }

    return ret;
}

bool DPOS::CheckCoinbase(const CTransaction& tx){
    bool ret = false;
    if(tx.vout.size() == 3) {
        DelegateInfo cDelegateInfo;
        time_t tDelegateInfo;
        std::vector<unsigned char> vctPublicKey;
        if(ScriptToDelegateInfo(cDelegateInfo, tDelegateInfo, &vctPublicKey, tx.vout[0].scriptPubKey)) {
            if(tx.nTime == tDelegateInfo) {
                CPubKey pubkey(vctPublicKey);
                CTxDestination address;
                ExtractDestination(tx.vout[1].scriptPubKey, address);

                if(address.type() == typeid(CKeyID)
                    && boost::get<CKeyID>(address) == pubkey.GetID()) {
                    ret = true;
                } else {
                    ret = false;
                }
            } else {
                // HTODO reason ???
                ret = false;
            }
        }
    }

    if(ret == false) {
        LogPrintf("Check dpos Coinbase failed!");
    }
    return ret;
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

bool DPOS::CheckBlockDelegate(const CBlock& block)
{
    DelegateInfo cDelegateInfo;
    if(GetBlockDelegate(cDelegateInfo, block) == false) {
        LogPrintf("CheckBlockDelegate GetBlockDelegate hash:%s error\n", block.GetHash().ToString().c_str());
        return false;
    }

    bool ret = true;
    DelegateInfo cNextDelegateInfo = GetNextDelegates(block.nTime);
    if(cDelegateInfo.delegates.size() == cNextDelegateInfo.delegates.size()) {
        for(unsigned int i =0; i < cDelegateInfo.delegates.size(); ++i) {
            if(cDelegateInfo.delegates[i].keyid != cNextDelegateInfo.delegates[i].keyid) {
                ret = false;
                break;
            }
        }
    }

    if(ret == false) {
        for(unsigned int i =0; i < cDelegateInfo.delegates.size(); ++i) {
            LogPrintf("CheckBlockDelegate BlockDelegate[%u]: %s\n", i, CBitcoinAddress(cDelegateInfo.delegates[i].keyid).ToString().c_str());
        }

        for(unsigned int i =0; i < cNextDelegateInfo.delegates.size(); ++i) {
            LogPrintf("CheckBlockDelegate NextBlockDelegate[%u]: %s %llu\n", i, CBitcoinAddress(cNextDelegateInfo.delegates[i].keyid).ToString().c_str(), cNextDelegateInfo.delegates[i].votes);
        }
    }

      return ret;
}

bool DPOS::GetBlockDelegates(DelegateInfo& cDelegateInfo, CBlockIndex* pBlockIndex)
{
    bool ret = false;
    uint64_t nLoopIndex = GetLoopIndex(pBlockIndex->nTime);
    while(pBlockIndex) {
        if(pBlockIndex->nHeight == nDposStartHeight || GetLoopIndex(pBlockIndex->pprev->nTime) < nLoopIndex) {
            CBlock block;
            if(ReadBlockFromDisk(block, pBlockIndex, Params().GetConsensus())) {
                ret = GetBlockDelegate(cDelegateInfo, block);
            }
            break;
        }

        pBlockIndex = pBlockIndex->pprev;
    }

    return ret;
}

bool DPOS::GetBlockDelegates(DelegateInfo& cDelegateInfo, const CBlock& block)
{
    CBlockIndex blockindex;
    blockindex.nTime = block.nTime;
    BlockMap::iterator miSelf = mapBlockIndex.find(block.hashPrevBlock);
    if(miSelf == mapBlockIndex.end()) {
        LogPrintf("GetBlockDelegates find blockindex(%s) error\n", block.hashPrevBlock.ToString().c_str());
        return false;
    }
    blockindex.pprev = miSelf->second;
    return GetBlockDelegates(cDelegateInfo, &blockindex);
}

DelegateInfo DPOS::GetNextDelegates(int64_t t)
{
    uint64_t nLoopIndex = GetLoopIndex(t);
    uint64_t nMinHoldBalance = 0;
//    if(Params().NetworkIDString() == "main") {
//        if(nLoopIndex >= 47169) {
//            nMinHoldBalance = 500000000000;
//        }
//    } else {
//        if(nLoopIndex >= 277019) {
//            nMinHoldBalance = 1000000000000;
//        }
//    }

    std::vector<Delegate> delegates = Vote::GetInstance().GetTopDelegateInfo(nMinHoldBalance, nMaxDelegateNumber - 1);

    LogPrint("DPoS", "GetNextDelegates start\n");
    for(auto i : delegates)
        LogPrint("DPoS", "delegate %s %lu\n", CBitcoinAddress(i.keyid).ToString().c_str(), i.votes);
    LogPrint("DPoS", "GetNextDelegates end\n");

    Delegate delegate;
    GetDelegateID(delegate.keyid, strDelegateAddress);
    delegate.votes = 7;
    delegates.insert(delegates.begin(), delegate);

    delegates.resize(nMaxDelegateNumber);

    DelegateInfo cDelegateInfo;
    if(Params().NetworkIDString() == "main") {
        if(nLoopIndex >= 47169) {
            cDelegateInfo.delegates = SortDelegate(delegates);
        } else {
            cDelegateInfo.delegates = delegates;
        }
    } else {
        cDelegateInfo.delegates = SortDelegate(delegates);
    }

    return cDelegateInfo;
}

std::vector<Delegate> DPOS::SortDelegate(const std::vector<Delegate>& delegates)
{
    std::vector<Delegate> result;
    std::vector<const Delegate*> tmp(delegates.size(), NULL);
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

