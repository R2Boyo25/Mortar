#pragma once

#include "doctest.hpp"
#include <any>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

namespace util {
std::vector<std::string> removeDotSlash(std::vector<std::string> dotted);

std::string removeDotSlash(std::string dotted);

int Popen(std::string command, std::string STDIN);

std::string readFile(std::string fname);

bool startsWith(std::string str, std::string strwth);

std::string replaceExt(std::string filename, std::string newext);

std::vector<std::string> replaceExts(std::vector<std::string> files,
                                     std::string newext);

std::vector<std::string> filterFiles(std::vector<std::string> files,

                                     std::vector<std::string> exts = {"cpp",
                                                                      "c"});

std::vector<std::string> orderExts(std::vector<std::string> files);

std::vector<std::string> getFiles(std::string dir = "./");

std::vector<std::string> wrap(std::vector<std::string> towrap);

std::string join(std::vector<std::string> v, std::string delimiter = " ");

std::vector<std::string> split(std::string splitting, char delimiter);

std::vector<std::vector<std::string>> splitvs(std::vector<std::string> vec,
                                              int n);

std::string getExt(std::string filename);

void makedirs(std::string filename);

std::vector<std::string> toBuild(std::vector<std::string> files);

std::string toBuild(std::string file);

std::string lstrip(std::string text, std::string toremove = " ");

std::string rstrip(std::string text, std::string toremove = " ");

std::string strip(std::string text, std::string toremove = " ");

std::string stripComment(std::string text);

std::string dirName(std::string filename);

std::vector<std::string> filterFilesRegex(std::string regex,
                                          std::vector<std::string> files);

std::vector<std::string> filterOutFilesRegex(std::string regex,
                                             std::vector<std::string> files);

std::vector<std::string> includeExclude(std::vector<std::string> include,
                                        std::vector<std::string> exclude,
                                        std::vector<std::string> files);

bool includeExclude(std::vector<std::string> include,
                    std::vector<std::string> exclude, std::string file);

std::vector<std::string> getExcluded(std::vector<std::string> include,
                                     std::vector<std::string> exclude,
                                     std::vector<std::string> files);
} // namespace util
