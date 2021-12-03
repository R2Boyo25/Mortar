#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <map>
#include <any>
#include <SHA1/sha1.hpp> // https://github.com/stbrumme/hash-library
#include <stdio.h>
#include <thread>
#include <mutex>
#include <variant>
#include "toml/toml.hpp"

#include "util.hpp"

using namespace std;
using namespace std::filesystem;
using namespace util;

bool ERRORFOUND = false;
mutex CANPRINT;
mutex MODIFY_GLOBALS;
int NTHREADS = std::thread::hardware_concurrency();
bool TREEVIEW = false;
int GLOBAL_COUNT = 0;
int GLOBAL_PROGRESS = 0;

toml::table loadConfig() {
    if ( exists(".mort") ) {
        try {
            toml::table tmltab = toml::parse(".mort");
            return tmltab;
        } catch (...) {
            std::cout << "Failed to parse config file, not valid TOML" << std::endl;
        }
    } else if ( exists(".acmp") ) {
        try {
            toml::table tmltab = toml::parse(".acmp");
            return tmltab;
        } catch (...) {
            std::cout << "Failed to parse config file, not valid TOML" << std::endl;
        }
    } else {
        toml::table cfg = toml::parse("");

        return cfg;
    }
    toml::table cfg = toml::parse("");
    return cfg;
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

void saveHash(string filename) {
    string fname = replaceExt(filename, "mhsh");
    writeFile(fname, genHash(filename));
}

bool fileChanged(string filename) {
    string fname = replaceExt(filename, "mhsh");
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

int rawComp(string file, string com = "g++", vector<string> args = {}) {
    string comm = join({com, file, join(args)});

    const char * ccomm = comm.c_str();

    return system(ccomm);
}

tuple<string, int> compO(string cppfile, string com = "g++", vector<string> args = {}, int THREADID = 1, string PROGRESS = "") {
    if (fileChanged(cppfile)) {
        CANPRINT.lock();
        cout << "[" << THREADID << ", " << PROGRESS << "]: " << cppfile.substr(2, cppfile.size() - 1) << endl;
        CANPRINT.unlock();
        vector<string> nargs = {"-c"}; 
        for (const string& arg : args) {
            if (!startsWith(arg, "-o")) {
                nargs.push_back(arg);
            } else {
                nargs.push_back("-o" + replaceExt(cppfile, "o"));
            }
        }

        int code = rawComp(cppfile, com, nargs);
        
        if (!code) {
            saveHash(cppfile);
        }

        return { replaceExt(cppfile, ".o"), code };
    } else {
        return { replaceExt(cppfile, ".o"), 0 };
    }
}

void threadComp(vector<string> files, string com = "g++", vector<string> args = {}, int THREADID = 1) {
    if (files.size() == 0) {
        return;
    }
    int THREAD_PROGRESS = 0;
    int THREAD_COUNT = files.size();
    if (TREEVIEW) {
        CANPRINT.lock();
        cout << "[" << THREADID << "]: " << "Files assigned" << endl;
        Popen("tree --noreport --fromfile", join(removeDotSlash(files), "\n")); 
        CANPRINT.unlock();
    } 
    for (const string& file : files) {
        CANPRINT.lock();
        THREAD_PROGRESS++;
        GLOBAL_PROGRESS++;
        string prog = "G" + to_string(GLOBAL_PROGRESS) + "/" + to_string(GLOBAL_COUNT) + " T" + to_string(THREAD_PROGRESS) + "/" + to_string(THREAD_COUNT);
        CANPRINT.unlock();

        auto [ofile, scode] = compO(file, com, args, THREADID, prog);
        if (scode != 0) {
            cout << "[MORTAR]: ERROR" << endl;
            exit(scode);
        }
    }
}

int oComp(string com = "g++", vector<string> args = {}) {
    string jargs = join(args);
    vector<string> wfiles = filterFiles(getFiles());

    vector<string> pfiles = {};
    
    for (const string& file : wfiles) {
        if (fileChanged(file)) {
            pfiles.push_back(file);
        }
    }

    vector<thread> threads = {};

    GLOBAL_COUNT = pfiles.size();

    vector<vector<string>> sfiles = splitvs(pfiles, NTHREADS);

    for (const vector<string>& chunk : sfiles) {
        //for (int i = 0; i < sfiles.size(); i++) {
        //vector<string> chunk = sfiles[i];
        thread thrd(threadComp, chunk, com, args, threads.size()+1);
        threads.push_back(move(thrd));
    }

    for (auto& thrd : threads) {
        if (thrd.joinable()) {
            thrd.join();
        }
    }

    string jofiles = join(wrap(replaceExts(wfiles, "o")));

    string comm = join({com, jofiles, jargs});

    cout << "[MORTAR]: " << "Combining object files" << endl;

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
    toml::table config = loadConfig();
    if (!config.count(target)) {

        cout << "Target " << target << " not found!" << endl;
        return;

    } else {

        string com = "g++";
        string oarg = "";
        string link = "";
        string out = "-oa.out";

        std::map<toml::key, toml::value> ctarg = toml::get<std::map<toml::key, toml::value>>(config.at(target));

        if (ctarg.count("com")) {
            com = get<string>(ctarg.at("com"));
        }

        if (ctarg.count("oarg")) {
            vector<string> coargs = get<vector<string>>(ctarg["oarg"]);
            if (exists("include")) {
                coargs.push_back("-I include");
            }
            oarg = join(coargs);
        }

        if (ctarg.count("l")) {
            for (string const& li : get<vector<string>>(ctarg["l"])) {
                link += "-l" + li + " ";
            }
        }

        if (ctarg.count("out")) {
            out = "-o" + get<string>(ctarg.at("out"));
        }

        if (ctarg.count("threads")) {
            NTHREADS = get<int>(ctarg.at("threads"));
        }

        if (ctarg.count("tree")) {
            TREEVIEW = get<bool>(ctarg.at("tree"));
        }

        if (ctarg.count("obj")) {
            if (!get<bool>(ctarg["obj"])) {
                if (ctarg.count("after")) {
                    if (!comp(com, {out, oarg, link})) {
                        int rcode = system(get<string>(ctarg.at("after")).c_str());
                    }
                } else {
                    comp(com, {out, oarg, link});
                }
            } else {
                if (ctarg.count("after")) {
                    if (!oComp(com, {out, oarg, link})) {
                        int rcode = system(get<string>(ctarg.at("after")).c_str());
                    }
                } else {
                    oComp(com, {out, oarg, link});
                }
            }
        } else {
            if (ctarg.count("after")) {
                if (!oComp(com, {out, oarg, link})) {
                    int rcode = system(get<string>(ctarg.at("after")).c_str());
                }
            } else {
                oComp(com, {out, oarg, link});
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (!NTHREADS) {
        NTHREADS = 1;
    }

    std::vector<std::string> args(argv, argv + argc);
    if (argc == 1) {
        compTarget("_default");
    } else {
        if (args[1] == "-j") {
            NTHREADS = stoi(args[2]);
            compTarget("_default");
        } else if (args[1] == "clean") {
            for (const string& file : filterFiles(getFiles(), {"mhsh", "ahsh", "o"})) {
                remove( file );
            }
            return 0;
        } else {
            if (args[2] == "-j") {
                NTHREADS = stoi(args[3]);
            }
            compTarget(argv[1]);
        }
    }
}
