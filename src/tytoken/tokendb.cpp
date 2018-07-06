#include "tytoken/tokendb.h"
#include "primitives/transaction.h"
#include "script/standard.h"
#include "base58.h"
#include "tytoken/token.h"

static const char DB_TOKEN = 'T';

CTokenDB* pTokenInfos;

CTokenDB::CTokenDB(size_t nCacheSize, bool fMemory, bool fWipe) : db(GetDataDir() / "token", nCacheSize, fMemory, fWipe, true)
{
}

bool CTokenDB::GetTokenInfo(const CTokenID& tokenId, CTokenInfo &tokenInfo)
{
    return db.Read(std::make_pair(DB_TOKEN, tokenId), tokenInfo);
}

bool CTokenDB::WriteTokenInfo(const CTokenInfo &tokenInfo)
{
    return db.Write(std::make_pair(DB_TOKEN, tokenInfo.tokenID), tokenInfo);
}

bool CTokenDB::EraseTokenInfo(const CTokenID& tokenID)
{
    return db.Erase(std::make_pair(DB_TOKEN, tokenID));
}

const std::vector<CTokenInfo> CTokenDB::ListTokenInfos()
{
    std::vector<CTokenInfo> infos;
    CDBIterator* iter = db.NewIterator();
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()){
        std::pair<char, CTokenID> k;
        iter->GetKey(k);
        if (k.first == DB_TOKEN){
            CTokenInfo v;
            if (iter->GetValue(v))
                infos.push_back(v);
        }
    }
    delete iter;
    return std::move(infos);
}

void CTokenInfo::FromTx(const CTransaction& tx, const CTokenIssue& issue)
{
    //TODO extract name
    tokenID = tx.vout[1].tokenID;
    amount = tx.vout[1].nTokenValue;
    type = issue.nType;
    symbol = issue.symbol;
    fullName = issue.fullName;
    description = issue.description;
    url = issue.url;
    CTxDestination destination;
    if (ExtractDestination(tx.vout[1].scriptPubKey, destination) ){
        address = CBitcoinAddress(destination).ToString();
    }
    txHash = tx.GetHash();
}
