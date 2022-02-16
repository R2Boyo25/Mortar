#include "changed.hpp"

using namespace std;
using namespace std::filesystem;
using namespace util;

//set<string> gincludes = {};
extern bool compileheaders;

namespace changed {
    void writeFile(string fname, string content) {
        std::ofstream out(fname);
        out << content;
        out.close();
    } 

    string getPath(string dirname, string filename) {
        if (!startsWith(filename, "./")) {
            if (exists(dirname + "/" + filename)) {
                return dirname + "/" + filename;
            } else if (exists("./" + filename)) {
                return "./" + filename;
            } else if (exists("./include/" + filename)) {
                return "./include/" + filename;
            } else {
                return "";
            }
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
                    /*if (!(gincludes.find(getPath(dirName(filename), fname)) != gincludes.end())) {
                        cout << getPath(dirName(filename), fname) << endl;
                        gincludes.insert(getPath(dirName(filename), fname));
                        includes(getPath(dirName(filename), fname));
                    }*/
                    out.push_back(getPath(dirName(filename), fname));
                    for (string& f : includes(getPath(dirName(filename), fname))) {
                        out.push_back(getPath(dirName(filename), f));
                    }
                }
            }
        }

        //vector<string> out(gincludes.begin(), gincludes.end());
        return out;
    }

    vector<string> includesChanged(string filename) {
        vector<string> changed = {};

        for (string& f : includes(filename)) {
            if (fileChanged(f)) {
                changed.push_back(f);
            }
        }

        return changed;
    }

    file_time_type modTime(string fname) {
        return std::filesystem::last_write_time(fname);
    }

    bool fileChanged(string filename) {
        //cout << filename << endl;
        if (exists(outname)) {
            if (modTime(".mort") > modTime(outname)) {
                return true;
            } else if (includesChanged(filename).size() > 0) {
                return true;
            } else if (getExt(filename) == "cpp" or getExt(filename) == "c") {
                return ((modTime(filename) > modTime(outname)) or !exists((string("./build/")) + removeDotSlash(filename + ".o")));
            } else {
                if (compileheaders) {
                    return (modTime(filename) > modTime(outname) or !exists((string("./build/")) + removeDotSlash(filename + ".gch")));
                } else {
                    return (modTime(filename) > modTime(outname));
                }
            } 
        } else {
            if (includesChanged(filename).size() > 0) {
                return true;
            } else if (!exists(string("./build/") + removeDotSlash(filename + ".o")) && !exists((string("./build/")) + removeDotSlash(filename + ".gch"))) {
                return true;
            } else if (getExt(filename) == "cpp" or getExt(filename) == "c") {
                return (modTime(filename) > modTime(string("./build/") + removeDotSlash(filename + ".o")));
            } else if (getExt(filename) == "h" or getExt(filename) == "hpp") {
                if (compileheaders) {
                    return (!exists(((string("./build/")) + removeDotSlash(filename + ".gch"))));
                } else {
                    return true;
                }
            } else {
                return true;
            }
        }
    }
}