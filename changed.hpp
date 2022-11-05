#pragma once

#include <algorithm>
#include <filesystem>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "doctest.h"
#include "util.hpp"

extern std::string outname;

namespace changed {
std::string genHash(std::string fname);
void writeFile(std::string fname, std::string content);
void saveHash(std::string filename);
bool fileChanged(std::string filename);
bool fileChanged(std::string filename, std::string filevec);
bool rFileChanged(std::string filename);
std::vector<std::string> getIncludes(std::string filename);
std::filesystem::file_time_type modTime(std::string fname);
} // namespace changed