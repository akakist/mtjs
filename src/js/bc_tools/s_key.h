#pragma once
#include <string>
#include "Key.h"

bool verify(const Key& key, const std::string& sig_bin, const std::string& msg);
