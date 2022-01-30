#include "changed.hpp"

using namespace std;
using namespace std::filesystem;
using namespace util;

namespace changed {
    string genHash(string fname) {
        SHA1 sha1;
        string fcont = readFile(fname);

        return sha1(fcont);
    }

    void writeFile(string fname, string content) {
        std::ofstream out(fname);
        out << content;
        out.close();
    } 

    void saveHash(string filename) {
        string fname = "./build/" + removeDotSlash(filename + ".mhsh");//replaceExt(filename, "mhsh");
        writeFile(fname, genHash(filename));
    }

    string getPath(string dirname, string filename) {
        if (!startsWith(filename, "./")) {
            if (exists(dirname + "/" + filename)) {
                return dirname + "/" + filename;
            } else if (exists("./" + filename)) {
                return "./" + filename;
            } else if (exists("./include/" + filename)) {
                return "./include/" + filename;
            }
            return "e";
        } else {
            return filename;
        }
    }

    vector<string> includes(string filename) {
        vector<string> out = {};
        string content = readFile(filename);

        for (string &line : split(content, '\n')) {
            if (startsWith(line, "#include")) {
                line = stripComment(line);
                string fname = strip(line.substr(8, line.size()-1));
                if (fname[0] != '<') {
                    fname = strip(fname, "\"");
                    out.push_back(getPath(dirName(filename), fname));
                    for (string& f : includes(getPath(dirName(filename), fname))) {
                        out.push_back(getPath(dirName(filename), f));
                    }
                }
            }
        }

        return out;
    }

    vector<string> includesChanged(string filename) {
        vector<string> changed = {};

        for (string& f : includes(filename)) {
            if (rFileChanged(f)) {
                changed.push_back(f);
            }
        }

        return changed;
    }

    bool rFileChanged(string filename) {
        string fname = "./build/" + removeDotSlash(filename + ".mhsh");//replaceExt(filename, "mhsh");
        if (exists(fname)) {
            if (readFile(fname) != genHash(filename)) {
                return true;
            } else {
                return false;
            }
        } else {
            return true;
        }
    }

    std::tuple<bool, vector<string>> fileChanged(string filename) {
        string fname = "./build/" + removeDotSlash(filename + ".mhsh");//replaceExt(filename, "mhsh");
        if (exists(fname)) {
            if (readFile(fname) != genHash(filename)) {
                return {true, includesChanged(filename)};
            } else {
                return {includesChanged(filename).size() > 0, includesChanged(filename)};
            }
        } else {
            return {true, includesChanged(filename)};
        }
    }
}