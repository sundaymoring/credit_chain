#ifndef TOKEN_H
#define TOKEN_H

#include "script/script.h"

#include <vector>


class CTransaction;

tokencode GetTxTokenCode(const CTransaction& tx, std::vector<unsigned char>* pTokenData = NULL);

#endif // TOKEN_H
