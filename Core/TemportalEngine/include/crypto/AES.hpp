#pragma once

#include "crypto/Crypto.hpp"
#include "Singleton.hpp"

NS_CRYPTO

class AES : public Crypto, public Singleton<AES>
{

};

NS_END