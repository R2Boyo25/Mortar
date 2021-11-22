#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <map>
#include <any>
#include "nlohmann/json.hpp"
#include "SHA1/sha1.hpp" // https://github.com/stbrumme/hash-library


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

string join(vector<string> v, string delimiter = " ") { // https://stackoverflow.com/a/20986194
    stringstream ss;
    const int v_size = v.size();
    for(size_t i = 0; i < v_size; ++i)
    {
        if(i != 0)
            ss << delimiter;
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

string replaceExt(string filename, string newext) {
    vector<string> splt = split(filename, '.');
    splt[splt.size() - 1] = newext;
    return join(splt, ".");
}

vector<string> replaceExts(vector<string> files, string newext) {
    vector<string> newfiles = {};
    for (string const& file : files) {
        newfiles.push_back(replaceExt(file, newext));
    }
    return newfiles;
}

bool startsWith(string str, string strwth) {
    return str.rfind(strwth, 0) == 0;
}

string readFile(string fname) {
    // https://stackoverflow.com/a/2602258/14639101
    std::ifstream t(fname);
    std::stringstream buffer;
    buffer << t.rdbuf();
    return buffer.str();
}

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

bool fileChanged(string filename) {
    string fname = replaceExt(filename, "ahsh");
    if (exists(fname)) {
        if (readFile(fname) != genHash(filename)) {
            writeFile(fname, genHash(filename));
            return true;
        } else {
            return false;
        }
    } else {
        writeFile(fname, genHash(filename));
        return true;
    }
}

int rawComp(string file, string com = "g++", vector<string> args = {}) {
    string comm = join({com, file, join(args)});

    const char * ccomm = comm.c_str();

    return system(ccomm);
}

tuple<string, int> compO(string cppfile, string com = "g++", vector<string> args = {}) {
    if (fileChanged(cppfile)) {
        cout << "compiling " + cppfile << endl;
        vector<string> nargs = {"-c"};
        for (const string& arg : args) {
            if (!startsWith(arg, "-o")) {
                nargs.push_back(arg);
            } else {
                nargs.push_back("-o" + replaceExt(cppfile, "o"));
            }
        }

        return { replaceExt(cppfile, ".o"), rawComp(cppfile, com, nargs) };
    } else {
        return { replaceExt(cppfile, ".o"), 0 };
    }
}

int oComp(string com = "g++", vector<string> args = {}) {
    string jargs = join(args);
    vector<string> wfiles = filterFiles(getFiles());

    for (const string& file : wfiles) {
        auto [ofile, scode] = compO(file, com, args);
        if (scode != 0) {
            exit(scode);
        }
    }

    string jofiles = join(wrap(replaceExts(wfiles, "o")));

    string comm = join({com, jofiles, jargs});

    cout << comm << endl;

    const char * ccomm = comm.c_str();

    return system(ccomm);
}

int comp(string com = "g++", vector<string> args = {}) {
    string jargs = join(args);
    vector<string> wfiles = wrap(filterFiles(getFiles()));
    string jfiles = join(wfiles);

    string comm = join({com, jfiles, jargs});

    const char * ccomm = comm.c_str();

    cout << comm << endl;

    return system(ccomm);
}

json loadConfig() {
    if ( exists(".acmp") ) {
        json cfg = json::parse(readFile(".acmp"));

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
        if (ctarg.count("obj")) {
            if (!ctarg.at("obj")) {
                if (ctarg.count("after")) {
                    if (comp(com, {out, oarg, link})) {
                        int rcode = system(string(ctarg.at("after")).c_str());
                    }
                } else {
                    comp(com, {out, oarg, link});
                }
            } else {
                if (ctarg.count("after")) {
                    if (oComp(com, {out, oarg, link})) {
                        int rcode = system(string(ctarg.at("after")).c_str());
                    }
                } else {
                    oComp(com, {out, oarg, link});
                }
            }
        } else {
            if (ctarg.count("after")) {
                if (comp(com, {out, oarg, link})) {
                    int rcode = system(string(ctarg.at("after")).c_str());
                }
            } else {
                comp(com, {out, oarg, link});
            }
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