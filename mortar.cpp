#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <map>
#include <any>

#include <stdio.h>
#include <thread>
#include <mutex>
#include <variant>
#include "toml/toml.hpp"
#include <chrono>
#include <color.h>

#include "util.hpp"
#include "changed.hpp"

using namespace std;
using namespace std::filesystem;
using namespace util;
using namespace color;
using namespace changed;

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

void downloadDependency(map<string, toml::value> repo) {
    char BACKSLASH = '/';
    int r;
    if (!(repo.count("url")) or !(repo.count("cpath")) or !(repo.count("ipath"))) {
        CANPRINT.lock();
        std::cout << "Dependency missing git url, copy path, or include path" << std::endl;
        CANPRINT.unlock();
        r = system("rm -rf tmp");
        exit(1);
    } else {
        // I have no idea what I just wrote here, 
        // it is a mess because I didn't feel like messing with folder copying in C++
        // r is to get the compiler to stop complaining
        if (!exists("include/" + get<string>(repo["ipath"]))) {
            string user = split(get<string>(repo["url"]), BACKSLASH)[split(get<string>(repo["url"]), BACKSLASH).size()-2];
            string gitrepo = split(get<string>(repo["url"]), BACKSLASH)[split(get<string>(repo["url"]), BACKSLASH).size()-1];
            string folder = "tmp/" + user + "/" + gitrepo + "/";
            
            CANPRINT.lock();
            cout << "Downloading dependency \"" << user << "/" << gitrepo << "\"..." << endl;
            CANPRINT.unlock();

            r = system(("mkdir tmp/" + user).c_str());
            r = system(("git clone -q --depth=1 " + get<string>(repo["url"]) + " " + folder).c_str());
            if (repo.count("exclude")) {
                for (const string& file : get<vector<string>>(repo["exclude"])) {
                    r = system(("rm -rf " + folder + file).c_str());
                }
            }
            r = system(("mkdir -p $(dirname \"./include/" + get<string>(repo["ipath"]) + "\")").c_str());
            r = system(("cp -r " + folder + "/" + get<string>(repo["cpath"]) + " include/" + get<string>(repo["ipath"])).c_str());
        }
    }
}

void downloadDependencies(vector<map<string, toml::value>> deps) {
    int r;
    if (deps.size()) {

        if (!exists("include")) {
            r = system("mkdir include");
        }

        r = system("mkdir tmp");

        vector<thread> threads = {};

        for (map<string, toml::value>& repo : deps) {
            thread thrd(downloadDependency, repo);
            threads.push_back(move(thrd));
        }

        for (auto& thrd : threads) {
            if (thrd.joinable()) {
                thrd.join();
            }
        }

        r = system("rm -rf tmp");
    }
}

int rawComp(string file, string com = "g++", vector<string> args = {}) {
    string comm = join({com, file, join(args)});

    //{ { { { someprog; echo $? >&3; } | filter >&4; } 3>&1; } | { read xs; exit $xs; } } 4>&1
    
    /*
    if (!system("which gppfilt > /dev/null 2>&1")) {
        //comm = join({"{ { { {", com, file, join(args), "; echo $? >&3; } | gppfilt >&4; } 3>&1; } | { read xs; exit $xs; } } 4>&1"});
        comm = join({"mispipe \"", com, file, join(args), "\"", "\"gppfilt\""});
    } else {
        comm = join({com, file, join(args)});
    }
    */

    const char * ccomm = comm.c_str();

    return system(ccomm);
}

tuple<string, int> compO(string cppfile, string com = "g++", vector<string> args = {}, int THREADID = 1, string PROGRESS = "") {
    if (fileChanged(cppfile)) { 
        CANPRINT.lock();
        cout << CYN << "[" << ORN << THREADID << ", " << PROGRESS << CYN << "]: " << GRN << cppfile.substr(2, cppfile.size() - 1) << RES << endl;
        CANPRINT.unlock();
        vector<string> nargs = {"-c"}; 
        for (const string& arg : args) {
            if (!startsWith(arg, "-o")) {
                nargs.push_back(arg);
            } else {
                nargs.push_back(("-o" + string("./build/")) + removeDotSlash(replaceExt(cppfile, "o")));
            }
        }

        int code = rawComp(cppfile, com, nargs);
        
        if (!code) {
            saveHash(cppfile);
        }

        return { "./build/" + replaceExt(cppfile, ".o"), code };
    } else {
        return { "./build/" + replaceExt(cppfile, ".o"), 0 };
    }
}

void threadComp(vector<string> files, string com = "g++", vector<string> args = {}, int THREADID = 1) {
    if (files.size() == 0) {
        return;
    }
    int THREAD_PROGRESS = 0;
    int THREAD_COUNT = files.size();
    auto TIMESTART = chrono::system_clock::now();
    if (TREEVIEW) {
        CANPRINT.lock();
        cout << CYN << "[" << ORN << THREADID << CYN << "]: " << "Files assigned" << RES << endl;
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
            cout << RED << "[" <<ORN << "MORTAR" << RED << "]: ERROR" << RES << endl;
            exit(scode);
        }
    }
    auto TIMENOW = chrono::system_clock::now();
    CANPRINT.lock();
    cout << CYN << "[" << ORN << THREADID << CYN << "]: " << "Thread completed in " <<  chrono::duration_cast<chrono::seconds>(TIMENOW - TIMESTART).count() << " seconds\n" << RES;
    CANPRINT.unlock();
}

int oComp(string com = "g++", vector<string> args = {}) {
    auto MAINSTART = chrono::system_clock::now();
    string jargs = join(args);
    vector<string> wfiles = filterFiles(getFiles());

    vector<string> pfiles = {};
    
    for (const string& file : wfiles) {
        makedirs(file);
        if (fileChanged(file)) {
            pfiles.push_back(file);
        }
    }

    vector<thread> threads = {};

    GLOBAL_COUNT = pfiles.size();

    vector<vector<string>> sfiles = splitvs(pfiles, NTHREADS);

    int USED = 0;

    for (const vector<string>& chunk : sfiles) {
        if (chunk.size() > 0) {
            USED++;
        }
    }
    
    if (USED == 0) {
        cout << RED << "[" <<ORN << "MORTAR" << RED << "]: No files to compile" << RES << endl;
        exit(0);
    }

    cout << CYN << "[" <<ORN << "MORTAR" << CYN << "]: Found " << NTHREADS << " threads, using " << USED << RES << endl;

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

    string jofiles = join(wrap(toBuild(replaceExts(wfiles, "o"))));

    string comm = join({com, jofiles, jargs});

    cout << CYN << "[" << ORN << "MORTAR" << CYN << "]: " << "Combining object files" << RES << endl;

    const char * ccomm = comm.c_str();

    auto MAINNOW = chrono::system_clock::now();

    int res = system(ccomm);
    cout << CYN << "[" << ORN << "MORTAR" << CYN << "]: Compilation completed in " << chrono::duration_cast<chrono::seconds>(MAINNOW - MAINSTART).count() << " seconds\n" << RES;
    return res;
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
    toml::table config;

    try {
        config = loadConfig();
    } catch (std::runtime_error) {
        cout << "No .mort or .acmp file found, or it is improperly formatted" << endl;
        exit(1);
    }

    if (!exists("build")) {
        int r = system("mkdir build");
    }
    
    if (!config.count(target)) {

        cout << "Target " << target << " not found!" << endl;
        return;

    } else {

        if (config.count("deps")) {
            downloadDependencies(get<vector<map<string, toml::value>>>(config["deps"]));
        }

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
            for (const string& file : filterFiles(getFiles("./build"), {"mhsh", "ahsh", "o"})) {
                remove( file );
            }
            return 0;
        } else {
            if (argc == 4) {
                if (args[2] == "-j") {
                    NTHREADS = stoi(args[3]);
                }
            }
            compTarget(argv[1]);
        }
    }
}
