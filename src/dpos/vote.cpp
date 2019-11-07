#include "dpos/vote.h"
#include "streams.h"
#include "wallet/wallet.h"
#include "util.h"

#include <vector>
#include <set>

typedef boost::shared_lock<boost::shared_mutex> read_lock;
typedef boost::unique_lock<boost::shared_mutex> write_lock;

static CScript CreateDposScript(CKeyID keyID, CScript data){

    time_t t = time(NULL);
    CScriptNum num(t);
    auto vch = num.getvch();

    CKey key;
    pwalletMain->GetKey(keyID, key);
    auto pubkey = key.GetPubKey();

    std::vector<unsigned char> vchSig;
    key.Sign(Hash(vch.begin(), vch.end()), vchSig);

    CScript script;
    script.resize(3);
    script[0] = OP_DPOS;
    script[1] = 0x01;
    script[2] = DPOS_PROTOCOL_VERSION;
    script << ToByteVector(pubkey)
           << num
           << vchSig
           << ToByteVector(data);

    return script;
}

CScript CreateDposRegisterScript(CBitcoinAddress address, const std::string& name){

    CScript registeScript;
    registeScript.push_back(dpos_register);
    auto vName = ToByteVector(name);
    vName.resize(32);
    registeScript << vName;

    CKeyID keyID;
    address.GetKeyID(keyID);

    return CreateDposScript(keyID, registeScript);
}

CScript CreateDposVoteScript(CBitcoinAddress voter, std::set<CBitcoinAddress> setDeletate, bool isVote){

    dposType op_code = dpos_vote;
    if(isVote == false) {
        op_code = dpos_revoke;
    }
    CScript voteScript;
    voteScript.push_back(op_code);

    unsigned char delegateKeys[Vote::MaxNumberOfVotes * 20];
    unsigned char *p_delegateKeys = &delegateKeys[0];
    for (auto& a: setDeletate) {
        CKeyID id;
        a.GetKeyID(id);
        memcpy(p_delegateKeys, id.begin(), 20);
        p_delegateKeys += 20;
    }
    voteScript << std::vector<unsigned char>(&delegateKeys[0], p_delegateKeys);

    CKeyID voterKeyID;
    voter.GetKeyID(voterKeyID);
    return CreateDposScript(voterKeyID, voteScript);
}


bool Vote::Init(int64_t nBlockHeight, const std::string& strBlockHash)
{
    //write_lock w(lockVote);
    return true;
}

Vote& Vote::GetInstance(){
    static Vote vote;
    return vote;
}

bool Vote::ProcessVote(CKeyID& voter, const std::set<CKeyID>& delegates, uint256 hash, uint64_t height, bool fUndo)
{
    if(fUndo) {
        return ProcessUndoVote(voter, delegates, hash, height);
    } else {
        return ProcessVote(voter, delegates, hash, height);
    }
}

bool Vote::ProcessVote(CKeyID& voter, const std::set<CKeyID>& delegates, uint256 hash, uint64_t height)
{
    bool ret = ProcessVote(voter, delegates);
    if(ret == false) {
        AddInvalidVote(hash, height);
        LogPrintf("ProcessVote InvalidTx Hash:%s height:%lld\n", hash.ToString().c_str(), height);
    }

    LogPrint("DPoS", "DPoS ProcessVote Height:%u\n", height);
    for(auto i : delegates) {
        LogPrint("DPoS", "DPoS ProcessVote: %s ---> %s(%s)\n", CBitcoinAddress(voter).ToString().c_str(), Vote::GetInstance().GetDelegate(i).c_str(), CBitcoinAddress(i).ToString().c_str());
    }
    if(ret) {
        LogPrint("DPoS", "DPoS ProcessVote success\n");
    } else {
        LogPrintf("DPoS ProcessVote fail\n");
    }

    return ret;
}

bool Vote::ProcessUndoVote(CKeyID& voter, const std::set<CKeyID>& delegates, uint256 hash, uint64_t height)
{
    if(FindInvalidVote(hash)) {
        LogPrintf("ProcessUndoVote InvalidTx Hash:%s height:%lld\n", hash.ToString().c_str(), height);
        return true;
    }

    bool ret = ProcessCancelVote(voter, delegates);

    LogPrint("DPoS", "DPoS ProcessUndoVote Height:%u\n", height);
    for(auto i : delegates) {
        LogPrint("DPoS", "DPoS ProcessUndoVote: %s ---> %s(%s)\n", CBitcoinAddress(voter).ToString().c_str(), Vote::GetInstance().GetDelegate(i).c_str(), CBitcoinAddress(i).ToString().c_str());
    }
    if(ret) {
        LogPrint("DPoS", "DPoS ProcessUndoVote success\n");
    } else {
        LogPrintf("DPoS ProcessUndoVote fail\n");
    }

    return ret;
}

bool Vote::ProcessVote(CKeyID& voter, const std::set<CKeyID>& delegates)
{
    write_lock w(lockVote);

    {
        std::set<CKeyID> d;
        pDPoSDb->GetVoterDelegates(voter, d);
        // check limit of MaxNumberOfVotes
        if (d.size() + delegates.size() > Vote::MaxNumberOfVotes) {
            return false;
        }
    }


    // check duplicate vote
    for(const auto& item : delegates){
        std::string n;
        if (!pDPoSDb->GetDelegateName(item, n)){
            return false;
        }
        std::set<CKeyID> v;
        if (pDPoSDb->GetDelegateVoters(item, v)){
            if ( v.find(voter) != v.end() ){
                return false;
            }
        }
    }

    // update delegate -> voter
    // update voter -> delegate
    for(auto item : delegates)
    {
        pDPoSDb->InsertDelegateVoter(item, voter);
        pDPoSDb->InsertVoterDelegate(voter, item);
    }
    return true;

}

bool Vote::ProcessCancelVote(CKeyID& voter, const std::set<CKeyID>& delegates, uint256 hash, uint64_t height, bool fUndo)
{
    if(fUndo) {
        return ProcessUndoCancelVote(voter, delegates, hash, height);
    } else {
        return ProcessCancelVote(voter, delegates, hash, height);
    }
}

bool Vote::ProcessCancelVote(CKeyID& voter, const std::set<CKeyID>& delegates, uint256 hash, uint64_t height)
{
    bool ret = ProcessCancelVote(voter, delegates);
    if(ret == false) {
        AddInvalidVote(hash, height);
        LogPrintf("ProcessCancelVote InvalidTx Hash:%s height:%lld\n", hash.ToString().c_str(), height);
    }

    LogPrint("DPoS", "DPoS ProcessCancelVote Height:%u\n", height);
    for(auto i : delegates) {
        LogPrint("DPoS", "DPoS ProcessCancelVote: %s ---> %s(%s)\n", CBitcoinAddress(voter).ToString().c_str(), Vote::GetInstance().GetDelegate(i).c_str(), CBitcoinAddress(i).ToString().c_str());
    }
    if(ret) {
        LogPrint("DPoS", "DPoS ProcessCancelVote success\n");
    } else {
        LogPrintf("DPoS ProcessCancelVote fail\n");
    }

    return ret;
}

bool Vote::ProcessUndoCancelVote(CKeyID& voter, const std::set<CKeyID>& delegates, uint256 hash, uint64_t height)
{
    if(FindInvalidVote(hash)) {
        LogPrintf("ProcessUndoCancelVote InvalidTx Hash:%s height:%lld\n", hash.ToString().c_str(), height);
        return true;
    }

    bool ret = ProcessVote(voter, delegates);
    LogPrint("DPoS", "DPoS ProcessUndoCancelVote Height:%u\n", height);
    for(auto i : delegates) {
        LogPrint("DPoS", "DPoS ProcessUndoCancelVote: %s ---> %s(%s)\n", CBitcoinAddress(voter).ToString().c_str(), Vote::GetInstance().GetDelegate(i).c_str(), CBitcoinAddress(i).ToString().c_str());
    }
    if(ret) {
        LogPrint("DPoS", "DPoS ProcessUndoCancelVote success\n");
    } else {
        LogPrintf("DPoS ProcessUndoCancelVote fail\n");
    }

    return ret;
}

bool Vote::ProcessCancelVote(CKeyID& voter, const std::set<CKeyID>& delegates)
{
    write_lock w(lockVote);
    if(delegates.size() > Vote::MaxNumberOfVotes) {
        return false;
    }

    // confirm is or not vote to delegate
    for(auto item : delegates)
    {
        std::set<CKeyID> v;
        if (pDPoSDb->GetDelegateVoters(item, v)){
            if (v.find(voter) == v.end()){
                return false;
            }
        } else {
            return false;
        }
    }

    // erase from delegateVoter
    for(auto item : delegates)
    {
        pDPoSDb->EraseDelegateVoter(item, voter);
    }

    pDPoSDb->EraseVoterDelegates(voter, delegates);

    return true;
}

bool Vote::ProcessRegister(CKeyID& delegate, const std::string& strDelegateName, uint256 hash, uint64_t height, bool fUndo)
{
    if(fUndo) {
        return ProcessUndoRegister(delegate, strDelegateName, hash, height);
    } else {
        return ProcessRegister(delegate, strDelegateName, hash, height);
    }
}

bool Vote::ProcessRegister(CKeyID& delegate, const std::string& strDelegateName, uint256 hash, uint64_t height)
{
    bool ret = ProcessRegister(delegate, strDelegateName);
    if(ret == false) {
        AddInvalidVote(hash, height);
        LogPrint("DPoS", "ProcessRegister InvalidTx Hash:%s height:%lld\n", hash.ToString().c_str(), height);
    }

    if(ret) {
        LogPrint("DPoS", "DPoS ProcessRegister: Height:%u %s ---> %s success\n", height, CBitcoinAddress(delegate).ToString().c_str(), strDelegateName.c_str());
    } else {
        LogPrintf("DPoS ProcessRegister: Height:%u %s ---> %s fail\n", height, CBitcoinAddress(delegate).ToString().c_str(), strDelegateName.c_str());
    }

    return ret;
}

bool Vote::ProcessUndoRegister(CKeyID& delegate, const std::string& strDelegateName, uint256 hash, uint64_t height)
{
    if(FindInvalidVote(hash)) {
        LogPrintf("ProcessUndoRegister InvalidTx Hash:%s height:%lld\n", hash.ToString().c_str(), height);
        return true;
    }

    bool ret = ProcessUnregister(delegate, strDelegateName);

    if(ret) {
        LogPrint("DPoS", "DPoS ProcessUndoRegister: Height:%u %s ---> %s success\n", height, CBitcoinAddress(delegate).ToString().c_str(), strDelegateName.c_str());
    } else {
        LogPrintf("DPoS ProcessUndoRegister: Height:%u %s ---> %s fail\n", height, CBitcoinAddress(delegate).ToString().c_str(), strDelegateName.c_str());
    }

    return ret;
}

bool Vote::ProcessRegister(CKeyID& delegate, const std::string& strDelegateName)
{
    write_lock w(lockVote);

    std::string n;
    if (pDPoSDb->GetDelegateName(delegate, n) && !n.empty()){
        return false;
    }

    CKeyID d;
    if (pDPoSDb->GetNameDelegate(strDelegateName, d) && !d.IsNull() ){
        return false;
    }

    pDPoSDb->WriteDelegateName(delegate, strDelegateName);
    pDPoSDb->WriteNameDelegate(strDelegateName, delegate);

    return true;
}

bool Vote::ProcessUnregister(CKeyID& delegate, const std::string& strDelegateName)
{
    write_lock w(lockVote);
    std::string n;
    if (!pDPoSDb->GetDelegateName(delegate, n)){
        return false;
    }

    CKeyID d;
    if (!pDPoSDb->GetNameDelegate(strDelegateName, d)){
        return false;
    }

    pDPoSDb->EraseDelegateName(delegate);
    pDPoSDb->EraseNameDelegate(strDelegateName);
    return true;




//    write_lock w(lockVote);
//    if(mapDelegateName.find(delegate) == mapDelegateName.end())
//        return false;

//    if(mapNameDelegate.find(strDelegateName) == mapNameDelegate.end())
//        return false;

//    mapDelegateName.erase(delegate);
//    mapNameDelegate.erase(strDelegateName);
//    return true;
}

uint64_t Vote::GetDelegateVotes(const CKeyID& delegate)
{
    read_lock r(lockVote);
    return _GetDelegateVotes(delegate);
}

uint64_t Vote::_GetDelegateVotes(const CKeyID& delegate)
{
    std::set<CKeyID> voters;
    pDPoSDb->GetDelegateVoters(delegate, voters);
    uint64_t voteNum = 0;
    for (const auto& item : voters){
        voteNum += pDPoSDb->GetAddressVoteNum(item);
    }
    return voteNum;
}

std::set<CKeyID> Vote::GetDelegateVoters(CKeyID& delegate)
{
    read_lock r(lockVote);
    std::set<CKeyID> s;
    pDPoSDb->GetDelegateVoters(delegate, s);

    return s;
}

CKeyID Vote::GetDelegate(const std::string& name)
{
    read_lock r(lockVote);
    CKeyID delegate;
    if (pDPoSDb->GetNameDelegate(name, delegate)){
        return delegate;
    }
    return CKeyID();
}

std::string Vote::GetDelegate(CKeyID keyid)
{
    read_lock r(lockVote);
    std::string name;
    if (pDPoSDb->GetDelegateName(keyid, name)){
        return name;
    }
    return std::string();
}

bool Vote::HaveVote(CKeyID voter, CKeyID delegate)
{
    read_lock r(lockVote);
    std::set<CKeyID> vs;
    if (pDPoSDb->GetDelegateVoters(delegate, vs)){
        return vs.find(voter) != vs.end();
    }
    return false;
}

bool Vote::HaveDelegate(const std::string& name, CKeyID keyid)
{
    read_lock r(lockVote);
    std::string n;
    if (pDPoSDb->GetDelegateName(keyid, n)){
        return n == name;
    }
    return false;
}

bool Vote::HaveDelegate(std::string name)
{
    read_lock r(lockVote);
    CKeyID keyid;
    if (pDPoSDb->GetNameDelegate(name, keyid)){
        return !keyid.IsNull();
    }
    return false;
}

bool Vote::HaveDelegate(CKeyID keyID)
{
    read_lock r(lockVote);
    std::string name;
    if (pDPoSDb->GetDelegateName(keyID, name)){
        return !name.empty();
    }
    return false;
}

std::set<CKeyID> Vote::GetVotedDelegates(CKeyID& voter)
{
    read_lock r(lockVote);
    std::set<CKeyID> s;
    pDPoSDb->GetVoterDelegates(voter, s);

    return s;
}

std::vector<Delegate> Vote::GetTopDelegateList(CAmount nMinHoldBalance, uint32_t nDelegateNum)
{
    read_lock r(lockVote);
    std::set<std::pair<uint64_t, CKeyID>> delegates;

    pDPoSDb->ListDelegateName();

    // traversal all delegateVote
    std::map<CKeyID, std::set<CKeyID>> mDelegateVoters = pDPoSDb->ListDelegateVoters();
    for(const auto& item : mDelegateVoters)
    {
        // if delegate amount > nMinHoldBalance, then valid
        if (pDPoSDb->GetAddressVoteNum(item.first) >= nMinHoldBalance){
            CAmount vote = 0;
            // HTODO test  delegates.insert(std::make_pair(vote, item.first));????
            for (const auto& item2 : item.second){
                vote += pDPoSDb->GetAddressVoteNum(item2);
//                delegates.insert(std::make_pair(vote, item.first));
            }
            delegates.insert(std::make_pair(vote, item.first));
        }
    }

    // traversal all delegateName that get 0 vote
    std::map<CKeyID, std::string> mDelegateName = pDPoSDb->ListDelegateName();
    for(auto it = mDelegateName.rbegin(); it != mDelegateName.rend(); ++it)
    {
        if (pDPoSDb->GetAddressVoteNum(it->first) < nMinHoldBalance){
            continue;
        }

        if(delegates.size() >= nDelegateNum) {
            break;
        }

        if(mDelegateVoters.find(it->first) == mDelegateVoters.end())
            delegates.insert(std::make_pair(0, it->first));
    }

    // get top nDelegateNum
    std::vector<Delegate> result;
    for(auto it = delegates.rbegin(); it != delegates.rend(); ++it)
    {
        if(result.size() >= nDelegateNum) {
            break;
        }

        result.push_back(Delegate(it->second, it->first));
    }

    return result;
}


std::map<std::string, CKeyID> Vote::ListDelegates()
{
    read_lock r(lockVote);
    return pDPoSDb->ListNameDelegate();
}

uint64_t Vote::GetAddressBalance(const CKeyID& address)
{
    read_lock r(lockVote);
    return _GetAddressBalance(address);
}

uint64_t Vote::_GetAddressBalance(const CKeyID& address)
{
    return pDPoSDb->GetAddressVoteNum(address);
}

void Vote::UpdateAddressBalance(const std::vector<std::pair<CKeyID, int64_t>>& vAddressBalance)
{
    write_lock w(lockVote);
    std::map<CKeyID, int64_t> mapBalance;
    for(auto iter : vAddressBalance) {
        if (iter.second == 0)
            continue;

        mapBalance[iter.first] += iter.second;
    }

    for(auto iter : mapBalance)
    {
        _UpdateAddressBalance(iter.first, iter.second);
    }

    return;
}

uint64_t Vote::_UpdateAddressBalance(const CKeyID& address, int64_t value)
{
    int64_t balance = 0;

    CAmount voteNum = pDPoSDb->GetAddressVoteNum(address);
    if (voteNum > 0){
        balance = voteNum + value;
        if (balance < 0 ){
            abort();
        }

        if (balance == 0){
            pDPoSDb->EraseAddressVoteNum(address);
            return balance;
        }
        pDPoSDb->WriteAddressVoteNum(address, balance);
        return balance;
    } else {
        if (value < 0){
            abort();
        }

        if(value == 0) {
            return 0;
        } else {
            pDPoSDb->WriteAddressVoteNum(address, value);
        }

        return value;
    }

}

void Vote::DeleteInvalidVote(uint64_t height)
{
    write_lock r(lockMapHashHeightInvalidVote);
    for(auto it =  mapHashHeightInvalidVote.begin(); it != mapHashHeightInvalidVote.end();) {
        if(it->second <= height) {
            LogPrint("DPoS", "DeleteInvalidVote Hash:%s Height:%llu\n", it->first.ToString().c_str(), it->second);
            it = mapHashHeightInvalidVote.erase(it);
        } else {
            ++it;
        }
    }
}

void Vote::AddInvalidVote(uint256 hash, uint64_t height)
{
    write_lock r(lockMapHashHeightInvalidVote);
    mapHashHeightInvalidVote[hash] = height;
    LogPrint("DPoS", "AddInvalidVote Hash:%s Height:%llu\n", hash.ToString().c_str(), height);
}

bool Vote::FindInvalidVote(uint256 hash)
{
    read_lock r(lockMapHashHeightInvalidVote);
    return mapHashHeightInvalidVote.find(hash) != mapHashHeightInvalidVote.end();
}
