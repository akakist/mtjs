#pragma once
#include <map>
#include <string>
#include <memory>
#include "granule.h"
#include "contract_context.h"

// Глобальное состояние — гранулы, привязанные к контракту-владельцу
extern std::map<std::string, Granule> granules;

// Кэш загруженных контрактов
extern std::map<std::string, std::shared_ptr<ContractContext>> contract_cache;