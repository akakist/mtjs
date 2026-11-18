#pragma once
#include <string>
#include "Key.h"

bool verify(const Key& key, const std::string& sig_bin, const std::string& msg);

/// Подпись приватным ключом указанного типа (sk — сырой секрет: 64 байта ed25519, 32 байта ECDSA).
/// Формат подписи ровно тот, что ожидает verify(): 64 байта ed25519 либо r||s по 32 байта для ECDSA.
std::string sign(const Key& key, const std::string& sk_bin, const std::string& msg);

/// Публичный ключ из приватного в формате, который принимает verify()
/// (ed25519 — 32 байта, ECDSA — 33 сжатый либо 65 несжатый).
std::string public_key(uint8_t type, const std::string& sk_bin, bool compressed=true);
