#pragma once

#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <map>
#include <any>
#include "SHA1/sha1.hpp" // https://github.com/stbrumme/hash-library
//#include "nlohmann/json.hpp"
//#include "TOML++/toml.hpp"
#include "toml/toml.hpp"

#include "util.hpp"

namespace conf {
   toml::table loadConfig();
}