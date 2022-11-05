#pragma once
#include <filesystem>
#include <map>
#include <string>
#include <toml/toml.hpp>
#include <vector>

extern toml::table CONFIG;

toml::table loadConfig();

bool configValueExists(std::map<toml::key, toml::value> table, std::string key);

template <class T>
T getConfigValue(std::map<toml::key, toml::value> table, std::string key) {
  return toml::get<T>(table.at(key));
};

std::map<toml::key, toml::value> tableToMap(toml::value table);

bool configInherits(std::map<toml::key, toml::value> table);

toml::value getConfigInheritance(std::map<toml::key, toml::value> table);

template <class T>
T getInheritedValue(std::map<toml::key, toml::value> table, std::string key) {
  return getConfigValue<T>(tableToMap(getConfigInheritance(table)), key);
}