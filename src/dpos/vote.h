#ifndef VOTE_H
#define VOTE_H

#include "dpos/dpos.h"
#include "script/script.h"
#include "base58.h"

#include <string>
#include <unordered_map>
#include <boost/thread/shared_mutex.hpp>
#include <boost/filesystem.hpp>

enum dposType {
    dpos_register,
    dpos_vote,
    dpos_revoke,
};

CScript CreateDposRegisterScript(CBitcoinAddress address, const std::string& name);

struct key_hash
{
    std::size_t operator()(CKeyID const& k) const {
        std::size_t hash = 0;
        boost::hash_range( hash, k.begin(), k.end() );
        return hash;
    }
};

class Vote
{
public:
    static Vote& GetInstance();

    std::vector<Delegate> GetTopDelegateInfo(uint64_t nMinHoldBalance, uint32_t nDelegateNum);

    bool ProcessVote(CKeyID& voter, const std::set<CKeyID>& delegates, uint256 hash, uint64_t height, bool fUndo);
    bool ProcessCancelVote(CKeyID& voter, const std::set<CKeyID>& delegates, uint256 hash, uint64_t height, bool fUndo);
    bool ProcessRegister(CKeyID& delegate, const std::string& strDelegateName, uint256 hash, uint64_t height, bool fUndo);

    uint64_t GetDelegateVotes(const CKeyID& delegate);
    std::set<CKeyID> GetDelegateVoters(CKeyID& delegate);
    std::set<CKeyID> GetVotedDelegates(CKeyID& delegate);
    std::map<std::string, CKeyID> ListDelegates();

    uint64_t GetAddressBalance(const CKeyID& id);
    void UpdateAddressBalance(const std::vector<std::pair<CKeyID, int64_t>>& addressBalances);

    CKeyID GetDelegate(const std::string& name);
    std::string GetDelegate(CKeyID keyid);
    bool HaveDelegate(const std::string& name, CKeyID keyid);
    bool HaveDelegate_Unlock(const std::string& name, CKeyID keyid);
    bool HaveVote(CKeyID voter, CKeyID delegate);

    bool HaveDelegate(std::string name);
    bool HaveDelegate(CKeyID keyID);
    int64_t GetOldBlockHeight() {return nOldBlockHeight;}
    std::string GetOldBlockHash() {return strOldBlockHash;}

    void AddInvalidVote(uint256 hash, uint64_t height);
    void DeleteInvalidVote(uint64_t height);
    bool FindInvalidVote(uint256 hash);

    static const int MaxNumberOfVotes = 51;
private:
    Vote() {}

    uint64_t _GetAddressBalance(const CKeyID& address);
    uint64_t _GetDelegateVotes(const CKeyID& delegate);
    uint64_t _UpdateAddressBalance(const CKeyID& id, int64_t value);

    bool ProcessVote(CKeyID& voter, const std::set<CKeyID>& delegates, uint256 hash, uint64_t height);
    bool ProcessCancelVote(CKeyID& voter, const std::set<CKeyID>& delegates, uint256 hash, uint64_t height);
    bool ProcessRegister(CKeyID& delegate, const std::string& strDelegateName, uint256 hash, uint64_t height);
    bool ProcessUnregister(CKeyID& delegate, const std::string& strDelegateName, uint256 hash, uint64_t height);

    bool ProcessUndoVote(CKeyID& voter, const std::set<CKeyID>& delegates, uint256 hash, uint64_t height);
    bool ProcessUndoCancelVote(CKeyID& voter, const std::set<CKeyID>& delegates, uint256 hash, uint64_t height);
    bool ProcessUndoRegister(CKeyID& delegate, const std::string& strDelegateName, uint256 hash, uint64_t height);

    bool ProcessVote(CKeyID& voter, const std::set<CKeyID>& delegates);
    bool ProcessCancelVote(CKeyID& voter, const std::set<CKeyID>& delegates);
    bool ProcessRegister(CKeyID& delegate, const std::string& strDelegateName);
    bool ProcessUnregister(CKeyID& delegate, const std::string& strDelegateName);


private:
    boost::shared_mutex lockVote;

    std::map<CKeyID, std::set<CKeyID>> mapDelegateVoters;
    std::map<CKeyID, std::set<CKeyID>> mapVoterDelegates;
    std::map<CKeyID, std::string> mapDelegateName;
    std::map<std::string, CKeyID> mapNameDelegate;
    std::map<uint256, uint64_t>  mapHashHeightInvalidVote;
    boost::shared_mutex lockMapHashHeightInvalidVote;

    //std::unordered_map<CKeyID, uint64_t, key_hash> mapAddressBalance;
    std::unordered_map<CKeyID, int64_t, key_hash> mapAddressBalance;

    std::string strOldBlockHash;
    int64_t nOldBlockHeight;
};

#endif // VOTE_H
