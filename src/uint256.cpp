// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "uint256.h"

#include "utilstrencodings.h"
#include "base58.h"

#include <stdio.h>
#include <string.h>

template <unsigned int BITS>
base_blob<BITS>::base_blob(const std::vector<unsigned char>& vch)
{
    assert(vch.size() == sizeof(data));
    memcpy(data, &vch[0], sizeof(data));
}

template <unsigned int BITS>
std::string base_blob<BITS>::GetHex() const
{
    char psz[sizeof(data) * 2 + 1];
    for (unsigned int i = 0; i < sizeof(data); i++)
        sprintf(psz + i * 2, "%02x", data[sizeof(data) - i - 1]);
    return std::string(psz, psz + sizeof(data) * 2);
}

template <unsigned int BITS>
void base_blob<BITS>::SetHex(const char* psz)
{
    memset(data, 0, sizeof(data));

    // skip leading spaces
    while (isspace(*psz))
        psz++;

    // skip 0x
    if (psz[0] == '0' && tolower(psz[1]) == 'x')
        psz += 2;

    // hex string to uint
    const char* pbegin = psz;
    while (::HexDigit(*psz) != -1)
        psz++;
    psz--;
    unsigned char* p1 = (unsigned char*)data;
    unsigned char* pend = p1 + WIDTH;
    while (psz >= pbegin && p1 < pend) {
        *p1 = ::HexDigit(*psz--);
        if (psz >= pbegin) {
            *p1 |= ((unsigned char)::HexDigit(*psz--) << 4);
            p1++;
        }
    }
}

template <unsigned int BITS>
void base_blob<BITS>::SetHex(const std::string& str)
{
    SetHex(str.c_str());
}

template <unsigned int BITS>
std::string base_blob<BITS>::ToString() const
{
    return (GetHex());
}

// Explicit instantiations for base_blob<160>
template base_blob<160>::base_blob(const std::vector<unsigned char>&);
template std::string base_blob<160>::GetHex() const;
template std::string base_blob<160>::ToString() const;
template void base_blob<160>::SetHex(const char*);
template void base_blob<160>::SetHex(const std::string&);

// Explicit instantiations for base_blob<256>
template base_blob<256>::base_blob(const std::vector<unsigned char>&);
template std::string base_blob<256>::GetHex() const;
template std::string base_blob<256>::ToString() const;
template void base_blob<256>::SetHex(const char*);
template void base_blob<256>::SetHex(const std::string&);

// Explicit instantiations for base_blob<272>
template base_blob<272>::base_blob(const std::vector<unsigned char>&);
template std::string base_blob<272>::GetHex() const;
template std::string base_blob<272>::ToString() const;
template void base_blob<272>::SetHex(const char*);
template void base_blob<272>::SetHex(const std::string&);



class CTokenID::CBase58ID : public CBase58Data
{
public:
    CBase58ID(const CTokenID& id)
    {
        //prefix code do not same to address
        //40 or 41 prefix 'T'
        vchVersion.assign(1, 40);
        vchData.resize(id.size());
        if (!vchData.empty())
            memcpy(&vchData[0], id.begin(), id.size());
    }

    CBase58ID(const std::string& id)
    {
        size_t nVersionBytes = 1;
        std::vector<unsigned char> vchTemp;
        bool rc58 = DecodeBase58Check(id.data(), vchTemp);
        if ((!rc58) || (vchTemp.size() < nVersionBytes)) {
            vchData.clear();
            vchVersion.clear();
            return;
        }
        vchVersion.assign(vchTemp.begin(), vchTemp.begin() + nVersionBytes);
        vchData.resize(vchTemp.size() - nVersionBytes);
        if (!vchData.empty())
            memcpy(&vchData[0], &vchTemp[nVersionBytes], vchData.size());
        memory_cleanse(&vchTemp[0], vchTemp.size());
    }

    void ConvertToTokenID(CTokenID& tokenID)
    {
        memcpy(tokenID.begin(), &vchData[0], tokenID.size());
    }
};

std::string CTokenID::ToBase58IDString() const
{
    CBase58ID base58ID(*this);
    return base58ID.ToString();
}

void CTokenID::FromBase58IDString(const std::string& strBase58ID)
{
    CBase58ID base58ID(strBase58ID);
    base58ID.ConvertToTokenID(*this);
}
