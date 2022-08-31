//
// Created by christian on 8/30/22.
//

#ifndef GRYPHHACKS__HMAC_H_
#define GRYPHHACKS__HMAC_H_

#include <vector>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <string>
#include <iostream>

auto hs256(std::string key, std::string data) -> std::vector<uint8_t> {
  unsigned int len = 0;
  auto res = HMAC(EVP_sha256(),
                  (const void *) key.data(),
                  key.size(),
                  (const unsigned char *) data.data(),
                  data.size(),
                  nullptr,
                  &len);
  return {res, res + len};
}

unsigned char *mx_hmac_sha256(const void *key, int keylen,
                              const unsigned char *data, int datalen,
                              unsigned char *result, unsigned int *resultlen) {

  return HMAC(EVP_sha256(), key, keylen, data, datalen, result, resultlen);
}

#endif //GRYPHHACKS__HMAC_H_
