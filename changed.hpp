#pragma once

#include<string>
#include<filesystem> 
#include<vector>
#include<tuple>
#include<set>
#include<algorithm>

#include"util.hpp"

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
}