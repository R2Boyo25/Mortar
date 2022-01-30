#include<SHA1/sha1.hpp> // https://github.com/stbrumme/hash-library
#include<string>
#include<filesystem>
#include<vector>
#include<tuple>

#include"util.hpp"

extern std::string outname;

namespace changed {
    std::string genHash(std::string fname);
    void writeFile(std::string fname, std::string content);
    void saveHash(std::string filename);
    bool fileChanged(std::string filename);
    bool rFileChanged(std::string filename);
    std::vector<std::string> getIncludes(std::string filename);
    std::filesystem::file_time_type modTime(std::string fname);
}