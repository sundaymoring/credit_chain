#include "dpos/db.h"
#include "dpos/dpos.h"


CDPoSDb* pDPoSDb;

static const std::string DB_DELEGATE_VOTER = "D_V";
static const std::string DB_VOTER_DELEGATE = "V_D";
static const std::string DB_DELEGATE_NAME = "D_N";
static const std::string DB_NAME_DELEGATE = "N_D";
static const std::string DB_ADDRESS_NUM = "A_N";
static const std::string DB_CURRENT_DELEGATE = "C_D";

//CVoteDb::CVoteDb(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "dpos", nCacheSize, fMemory, fWipe) {
//}

bool CDPoSDb::WriteDelegateVoters(const CKeyID &delegateKeyID, const std::set<CKeyID> &votersKeyID)
{
    return Write(std::make_pair(DB_DELEGATE_VOTER, delegateKeyID), votersKeyID);
}

bool CDPoSDb::InsertDelegateVoter(const CKeyID& delegateKeyID, const CKeyID& voterKeyID){
    std::set<CKeyID> v;
    GetDelegateVoters(delegateKeyID, v);
    v.insert(voterKeyID);
    return WriteDelegateVoters(delegateKeyID, v);
}

bool CDPoSDb::GetDelegateVoters(const CKeyID &delegateKeyID, std::set<CKeyID> &votersKeyID)
{
    return Read(std::make_pair(DB_DELEGATE_VOTER, delegateKeyID), votersKeyID);
}

bool CDPoSDb::EraseDelegateVoter(const CKeyID &delegateKeyID, const CKeyID& voterKeyID)
{
    std::set<CKeyID> v;
    GetDelegateVoters(delegateKeyID, v);
    if (v.find(voterKeyID) != v.end()){
        v.erase(voterKeyID);
    }

    if (v.size() > 0){
        return WriteDelegateVoters(delegateKeyID, v);
    }

    return Erase(std::make_pair(DB_DELEGATE_VOTER, delegateKeyID));
}

bool CDPoSDb::EraseDelegateVoters(const CKeyID &delegateKeyID){
    return Erase(std::make_pair(DB_DELEGATE_VOTER, delegateKeyID));
}

bool CDPoSDb::EraseDelegateVoters(const CKeyID& delegateKeyID, const std::set<CKeyID>& votersKeyID){
    bool ret = true;
    for (const auto& item : votersKeyID){
        ret &= EraseDelegateVoter(delegateKeyID, item);
    }
    return ret;
}

std::map<CKeyID, std::set<CKeyID>> CDPoSDb::ListDelegateVoters()
{
    std::map<CKeyID, std::set<CKeyID>> result;

    CDBIterator* iter = NewIterator();
    for(iter->SeekToFirst(); iter->Valid(); iter->Next()){
        std::pair<std::string, CKeyID> key;
        std::set<CKeyID> value;
        if( iter->GetKey(key) && key.first == DB_DELEGATE_VOTER && iter->GetValue(value)){
            result.insert(std::make_pair(key.second, value));
        }
    }
    delete iter;
    return result;
}

bool CDPoSDb::WriteVoterDelegates(const CKeyID &VoterKeyID, const std::set<CKeyID> &delegatesKeyID)
{
    return Write(std::make_pair(DB_VOTER_DELEGATE, VoterKeyID), delegatesKeyID);
}

bool CDPoSDb::InsertVoterDelegate(const CKeyID& VoterKeyID, const CKeyID& delegateKeyID){
    std::set<CKeyID> d;
    GetVoterDelegates(VoterKeyID, d);
    d.insert(delegateKeyID);
    return WriteVoterDelegates(VoterKeyID, d);
}


bool CDPoSDb::GetVoterDelegates(const CKeyID &VoterKeyID, std::set<CKeyID> &delegatesKeyID)
{
    return Read(std::make_pair(DB_VOTER_DELEGATE, VoterKeyID), delegatesKeyID);
}

bool CDPoSDb::EraseVoterDelegate(const CKeyID& VoterKeyID, const CKeyID& delegateKeyID){
    std::set<CKeyID> d;
    GetVoterDelegates(VoterKeyID, d);
    if (d.find(delegateKeyID) != d.end()){
        d.erase(delegateKeyID);
    }

    if (d.size() > 0){
        return WriteVoterDelegates(VoterKeyID, d);
    }

    return EraseVoterDelegates(VoterKeyID);
}

bool CDPoSDb::EraseVoterDelegates(const CKeyID& VoterKeyID, const std::set<CKeyID>& delegateKeyID){
    bool ret = true;
    for(const auto& item : delegateKeyID){
        ret &= EraseVoterDelegate(VoterKeyID, item);
    }
    return ret;
}

bool CDPoSDb::EraseVoterDelegates(const CKeyID &VoterKeyID)
{
    return Erase(std::make_pair(DB_VOTER_DELEGATE, VoterKeyID));
}

bool CDPoSDb::WriteDelegateName(const CKeyID &delegateKeyID, const std::string &name)
{
    return Write(std::make_pair(DB_DELEGATE_NAME, delegateKeyID), name);
}

bool CDPoSDb::GetDelegateName(const CKeyID &delegateKeyID, std::string &name)
{
    return Read(std::make_pair(DB_DELEGATE_NAME, delegateKeyID), name);
}

bool CDPoSDb::EraseDelegateName(const CKeyID &delegateKeyID)
{
    return Erase(std::make_pair(DB_DELEGATE_NAME, delegateKeyID));
}

std::map<CKeyID, std::string> CDPoSDb::ListDelegateName()
{
    std::map<CKeyID, std::string> result;

    CDBIterator* iter = NewIterator();
    for(iter->SeekToFirst(); iter->Valid(); iter->Next()){
        std::pair<std::string, CKeyID> key;
        std::string value;
        if( iter->GetKey(key) && key.first == DB_DELEGATE_NAME && iter->GetValue(value)){
            result.insert(std::make_pair(key.second, value));
        }
    }
    delete iter;
    return result;
}

bool CDPoSDb::WriteNameDelegate(const std::string &name, const CKeyID &delegateKeyID)
{
    return Write(std::make_pair(DB_NAME_DELEGATE, name), delegateKeyID);
}

bool CDPoSDb::GetNameDelegate(const std::string &name, CKeyID &delegateKeyID)
{
    return Read(std::make_pair(DB_NAME_DELEGATE, name), delegateKeyID);
}

bool CDPoSDb::EraseNameDelegate(const std::string &name)
{
    return Erase(std::make_pair(DB_NAME_DELEGATE, name));
}

std::map<std::string, CKeyID> CDPoSDb::ListNameDelegate(){
    std::map<std::string, CKeyID> result;

    CDBIterator* iter = NewIterator();
    for(iter->SeekToFirst(); iter->Valid(); iter->Next()){
        std::pair<std::string, std::string> key;
        CKeyID value;
        if( iter->GetKey(key) && key.first == DB_NAME_DELEGATE && iter->GetValue(value)){
            result.insert(std::make_pair(key.second, value));
        }
    }
    delete iter;
    return result;
}

bool CDPoSDb::WriteAddressVoteNum(const CKeyID &address, const CAmount &amount)
{
    return Write(std::make_pair(DB_ADDRESS_NUM, address), amount);
}

CAmount CDPoSDb::GetAddressVoteNum(const CKeyID &address)
{
    CAmount amount;
    return Read(std::make_pair(DB_ADDRESS_NUM, address), amount) ? amount : 0;
}

bool CDPoSDb::EraseAddressVoteNum(const CKeyID &address)
{
    return Erase(std::make_pair(DB_ADDRESS_NUM, address));
}

// current delegates
bool CDPoSDb::WriteCurrentDelegates(const int height, const DelegateList& delegateList){
    return Write(std::make_pair(DB_CURRENT_DELEGATE, height), delegateList);
}

bool CDPoSDb::GetCurrentDelegates(const int height, DelegateList& delegateList){
    return Read(std::make_pair(DB_CURRENT_DELEGATE, height), delegateList);
}
