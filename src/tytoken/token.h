#ifndef TOKEN_H
#define TOKEN_H

#include "uint256.h"
#include "net.h"

#include <string>
#include <vector>
#include <stdint.h>

extern std::unique_ptr<CConnman> g_connman;

std::vector<unsigned char> CreatePayload_IssuanceFixed(uint16_t propertyType, uint64_t amount, std::string name);
bool WalletTxBuilder(const std::string& assetAddress, int64_t referenceAmount, const std::vector<unsigned char>& data, uint256& txid, std::string& rawHex, bool commit=true);

#endif // TOKEN_H
