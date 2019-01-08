#ifndef DPOS_DB_H
#define DPOS_DB_H

#include "dbwrapper.h"
#include "pubkey.h"
#include "amount.h"

class DelegateList;
class CDPoSDb : public CDBWrapper{
public:
    CDPoSDb(size_t nCacheSize, bool fMemory = false, bool fWipe = false): CDBWrapper(GetDataDir() / "dpos", nCacheSize, fMemory, fWipe){}

    // delegate -> voters
    bool WriteDelegateVoters(const CKeyID& delegateKeyID, const std::set<CKeyID>& votersKeyID);
    bool InsertDelegateVoter(const CKeyID& delegateKeyID, const CKeyID& voterKeyID);
    bool GetDelegateVoters(const CKeyID& delegateKeyID, std::set<CKeyID>& votersKeyID);
    bool EraseDelegateVoter(const CKeyID& delegateKeyID, const CKeyID& voterKeyID);
    bool EraseDelegateVoters(const CKeyID& delegateKeyID);
    bool EraseDelegateVoters(const CKeyID& delegateKeyID, const std::set<CKeyID>& votersKeyID);
    std::map<CKeyID, std::set<CKeyID>> ListDelegateVoters();

    // voter -> delegates
    bool WriteVoterDelegates(const CKeyID& VoterKeyID, const std::set<CKeyID>& delegatesKeyID);
    bool InsertVoterDelegate(const CKeyID& VoterKeyID, const CKeyID& delegateKeyID);
    bool GetVoterDelegates(const CKeyID& VoterKeyID, std::set<CKeyID>& delegatesKeyID);
    bool EraseVoterDelegate(const CKeyID& VoterKeyID, const CKeyID& delegateKeyID);
    bool EraseVoterDelegates(const CKeyID& VoterKeyID);
    bool EraseVoterDelegates(const CKeyID& VoterKeyID, const std::set<CKeyID>& delegateKeyID);

    // delegate -> delegate name
    bool WriteDelegateName(const CKeyID& delegateKeyID, const std::string& name);
    bool GetDelegateName(const CKeyID& delegateKeyID, std::string& name);
    bool EraseDelegateName(const CKeyID& delegateKeyID);
    std::map<CKeyID, std::string> ListDelegateName();

    // delegate name -> delegate
    bool WriteNameDelegate(const std::string& name, const CKeyID& delegateKeyID);
    bool GetNameDelegate(const std::string& name, CKeyID& delegateKeyID);
    bool EraseNameDelegate(const std::string& name);
    std::map<std::string, CKeyID> ListNameDelegate();

    // address -> vote num
    bool WriteAddressVoteNum(const CKeyID& address, const CAmount& amount);
    CAmount GetAddressVoteNum(const CKeyID& address);
    bool EraseAddressVoteNum(const CKeyID& address);

    // current delegates
    bool WriteCurrentDelegates(const int height, const DelegateList& delegateList);
    bool GetCurrentDelegates(const int height, DelegateList& delegateList);
};

extern CDPoSDb* pDPoSDb;

#endif // DB_H
