#include<SHA1/sha1.hpp> // https://github.com/stbrumme/hash-library
#include<string>
#include<filesystem>
#include<vector>
#include<tuple>

#include"util.hpp"

namespace changed {
    std::string genHash(std::string fname);
    void writeFile(std::string fname, std::string content);
    void saveHash(std::string filename);
    std::tuple<bool, std::vector<std::string>> fileChanged(std::string filename);
    bool rFileChanged(std::string filename);
    std::vector<std::string> getIncludes(std::string filename);
}