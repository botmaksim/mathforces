#pragma once
#include <string>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <userver/crypto/base64.hpp>
#include <userver/formats/json.hpp>
#include <userver/utils/datetime.hpp>

inline std::string Sha3_256(const std::string& data) {
    unsigned int len = 0;
    unsigned char hash[EVP_MAX_MD_SIZE];
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha3_256(), nullptr);
    EVP_DigestUpdate(ctx, data.data(), data.size());
    EVP_DigestFinal_ex(ctx, hash, &len);
    EVP_MD_CTX_free(ctx);
    
    std::string hex;
    const char hex_chars[] = "0123456789abcdef";
    for(unsigned int i=0; i<len; ++i) {
        hex += hex_chars[(hash[i] >> 4) & 0xF];
        hex += hex_chars[hash[i] & 0xF];
    }
    return hex;
}

inline std::string HmacSha256(const std::string& key, const std::string& data) {
    unsigned int len = EVP_MAX_MD_SIZE;
    unsigned char hash[EVP_MAX_MD_SIZE];
    HMAC(EVP_sha256(), key.data(), key.size(), (const unsigned char*)data.data(), data.size(), hash, &len);
    return std::string((char*)hash, len);
}

inline std::string CreateJwt(int user_id, const std::string& role) {
    userver::formats::json::ValueBuilder header;
    header["alg"] = "HS256";
    header["typ"] = "JWT";

    userver::formats::json::ValueBuilder payload;
    payload["userId"] = user_id;
    payload["role"] = role;
    payload["exp"] = userver::utils::datetime::Now().time_since_epoch().count() / 1000000 + 86400 * 7;

    std::string unsigned_token = userver::crypto::base64::Base64UrlEncode(
        userver::formats::json::ToString(header.ExtractValue())) + "." +
        userver::crypto::base64::Base64UrlEncode(
        userver::formats::json::ToString(payload.ExtractValue()));

    const char* jwt_env = std::getenv("JWT_SECRET");
    std::string secret = jwt_env ? jwt_env : "super_secret_jwt_key_that_needs_to_be_secure";
    auto sig = HmacSha256(secret, unsigned_token);
    return unsigned_token + "." + userver::crypto::base64::Base64UrlEncode(sig);
}

inline bool VerifyJwt(const std::string& token, int& out_user, std::string& out_role) {
    auto dot1 = token.find('.');
    if (dot1 == std::string::npos) return false;
    auto dot2 = token.find('.', dot1 + 1);
    if (dot2 == std::string::npos) return false;

    auto unsigned_part = token.substr(0, dot2);
    auto signature_part = token.substr(dot2 + 1);

    const char* jwt_env = std::getenv("JWT_SECRET");
    std::string secret = jwt_env ? jwt_env : "super_secret_jwt_key_that_needs_to_be_secure";
    auto expected_sig = HmacSha256(secret, unsigned_part);
    auto expected_sig_b64 = userver::crypto::base64::Base64UrlEncode(expected_sig);

    if (signature_part != expected_sig_b64) {
        return false;
    }

    auto payload_b64 = token.substr(dot1 + 1, dot2 - dot1 - 1);
    while (payload_b64.size() % 4 != 0) payload_b64 += "=";
    for (char& c : payload_b64) {
        if (c == '-') c = '+';
        else if (c == '_') c = '/';
    }

    try {
        std::string payload_json = userver::crypto::base64::Base64Decode(payload_b64);
        auto val = userver::formats::json::FromString(payload_json);
        out_user = val["userId"].As<int>();
        out_role = val["role"].As<std::string>("");
        return true;
    } catch (...) {
        return false;
    }
}
