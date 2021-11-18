#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <map>
#include <any>
#include "nlohmann/json.hpp"

using namespace std;
using namespace std::filesystem;
using json = nlohmann::json;

vector<string> split(string splitting, char delimiter) { // https://stackoverflow.com/a/10058725

    stringstream tosplit(splitting);
    string segment;
    vector<string> seglist;

    while(getline(tosplit, segment, delimiter)) {
        //cout << segment << endl;
        seglist.push_back(segment);
    }

    return seglist;
}

string join(vector<string> v) { // https://stackoverflow.com/a/20986194
    stringstream ss;
    const int v_size = v.size();
    for(size_t i = 0; i < v_size; ++i)  // v is your vector of string
    {
        if(i != 0)
            ss << " ";
        ss << v[i];
    }
    string s = ss.str();

    return s;
}

vector<string> wrap(vector<string> towrap) {
    vector<string> wrappedv = {};

    for (string const& arg : towrap) {
        string wrapped = "\"" + arg + "\"";
        wrappedv.push_back(wrapped);
    }

    return wrappedv;
}

vector<string> getFiles(string dir = "./") {
    vector<string> files = {};

    for(auto const& dir_entry: recursive_directory_iterator{dir}) {
        files.push_back(dir_entry.path());
    }

    return files;
}

vector<string> filterFiles(vector<string> files, vector<string> exts = {"cpp", "c"}) {
    vector<string> ffiles = {};

    for (string const& file : files) {
        string sfile = split(file, '/').back();
        if (sfile.find(".") == string::npos) {
            continue;
        }

        string fext = split(sfile, '.').back();
        bool inexts = false;
        for (string const& ext : exts) {
            if ((fext == ext)) {
                inexts = true;
                break;
            }
        } 

        if (inexts) {
            ffiles.push_back(file);
        }

    } // I am disgusted by whatever this is that I have just made, but it works.

    return ffiles;
}

int comp(string com = "g++", vector<string> args = {}) {
    string jargs = join(args);
    vector<string> wfiles = wrap(filterFiles(getFiles()));
    string jfiles = join(wfiles);

    string comm = join({com, jfiles, jargs});

    const char * ccomm = comm.c_str();

    return system(ccomm);
}

json loadConfig() {
    if ( exists(".acmp") ) {
        ifstream t (".acmp");
        stringstream buffer;
        buffer << t.rdbuf();

        json cfg = json::parse(buffer.str());

        return cfg;
    } else {
        json cfg = json::parse("{}");

        return cfg;
    }
    
}

void compTarget(string target) {
    json config = loadConfig();
    if (!config.count(target)) {

        cout << "Target " << target << " not found!" << endl;
        return;

    } else {

        string com = "g++";
        string oarg = "";
        string link = "";
        string out = "-oa.out";

        auto ctarg = config.at(target);

        if (ctarg.count("com")) {
            com = ctarg.at("com");
        }

        if (ctarg.count("oarg")) {
            oarg = join(ctarg.at("oarg"));
        }

        if (ctarg.count("l")) {
            for (string const& li : ctarg.at("l")) {
                link += "-l" + li + " ";
            }
        }

        if (ctarg.count("out")) {
            out = "-o" + string(ctarg.at("out"));
        }

        if (ctarg.count("after")) {
            if (comp(com, {out, oarg, link})) {
                int rcode = system(string(ctarg.at("after")).c_str());
            }
        } else {
            comp(com, {out, oarg, link});
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        compTarget("_default");
    } else {
        compTarget(argv[1]);
    }
}
