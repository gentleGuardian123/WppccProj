# pragma once

#include <map>
#include <memory>
#include <vector>
#include <string>
#include <chrono>
#include <random>

void import_and_parse_data(std::unique_ptr<uint8_t[]>&, const std::string, const uint8_t);
std::uint8_t gen_rand(std::uint8_t seed);