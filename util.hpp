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

namespace util {
    std::vector<std::string> removeDotSlash(std::vector<std::string> dotted);
    int Popen(std::string command, std::string STDIN);
    std::string readFile(std::string fname);
    bool startsWith(std::string str, std::string strwth);
    std::string replaceExt(std::string filename, std::string newext);
    std::vector<std::string> replaceExts(std::vector<std::string> files, std::string newext);
    std::vector<std::string> filterFiles(std::vector<std::string> files, std::vector<std::string> exts = {"cpp", "c"});
    std::vector<std::string> getFiles(std::string dir = "./");
    std::vector<std::string> wrap(std::vector<std::string> towrap);
    std::string join(std::vector<std::string> v, std::string delimiter = " ");
    std::vector<std::string> split(std::string splitting, char delimiter);

    std::vector< std::vector<std::string> > splitvs(std::vector<std::string> vec, int n);
}