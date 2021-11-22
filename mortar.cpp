#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <map>
#include <any>
#include "SHA1/sha1.hpp" // https://github.com/stbrumme/hash-library
#include <stdio.h>

#include "util.hpp"
#include "conf.hpp"

using namespace std;
using namespace std::filesystem;
using namespace util;

using namespace conf; 

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
    string fname = replaceExt(filename, "mhsh");
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

void compTarget(string target) {
    auto config = loadConfig();
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
    std::vector<std::string> args(argv, argv + argc);
    if (argc == 1) {
        compTarget("_default");
    } else {
        if (args[1] == "clean") {
            for (const string& file : filterFiles(getFiles(), {"mhsh", "ahsh", "o"})) {
                remove( file );
            }
            return 0;
        } else {
            compTarget(argv[1]);
        }
    }
}
