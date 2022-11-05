#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <chrono>
#include <color.h>
#include <mutex>
#include <stdio.h>
#include <thread>
#include <toml/toml.hpp>
#include <tuple>

#include "changed.hpp"
#include "util.hpp"

using namespace std;
using namespace std::filesystem;
using namespace util;
using namespace color;
using namespace changed;

bool ERRORFOUND = false;
mutex CANPRINT;
mutex MODIFY_GLOBALS;
int NTHREADS = std::thread::hardware_concurrency();
int GLOBAL_COUNT = 0;
int GLOBAL_PROGRESS = 0;
string outname = "";
bool compileheaders = false;
bool SHAREDOBJECT = false;
toml::table CONFIG;

toml::table loadConfig() {
  if (exists(".mort")) {
    try {
      toml::table tmltab = toml::parse(".mort");
      return tmltab;
    } catch (...) {
      std::cout << "Failed to parse config file, not valid TOML" << std::endl;
      std::exit(1);
    }
  }

  toml::table cfg = toml::table();
  return cfg;
}

void downloadDependency(map<string, toml::value> repo) {
  char BACKSLASH = '/';
  int r;
  if (!(repo.count("url")) or !(repo.count("cpath")) or
      !(repo.count("ipath"))) {
    CANPRINT.lock();
    std::cout << "Dependency missing git url, copy path, or include path"
              << std::endl;
    CANPRINT.unlock();
    r = system("rm -rf tmp");
    exit(1);
  } else {
    // I have no idea what I just wrote here,
    // it is a mess because I didn't feel like messing with folder copying in
    // C++ r is to get the compiler to stop complaining
    if (!exists("include/" + get<string>(repo["ipath"]))) {
      string user = split(
          get<string>(repo["url"]),
          BACKSLASH)[split(get<string>(repo["url"]), BACKSLASH).size() - 2];
      string gitrepo = split(
          get<string>(repo["url"]),
          BACKSLASH)[split(get<string>(repo["url"]), BACKSLASH).size() - 1];
      string folder = "tmp/" + user + "/" + gitrepo + "/";

      CANPRINT.lock();
      cout << "Downloading dependency \"" << user << "/" << gitrepo << "\"..."
           << endl;
      CANPRINT.unlock();

      r = system(("mkdir tmp/" + user).c_str());
      r = system(
          ("git clone -q --depth=1 " + get<string>(repo["url"]) + " " + folder)
              .c_str());
      if (repo.count("exclude")) {
        for (const string &file : get<vector<string>>(repo["exclude"])) {
          r = system(("rm -rf " + folder + file).c_str());
        }
      }
      r = system(("mkdir -p $(dirname \"./include/" +
                  get<string>(repo["ipath"]) + "\")")
                     .c_str());
      r = system(("cp -r " + folder + "/" + get<string>(repo["cpath"]) +
                  " include/" + get<string>(repo["ipath"]))
                     .c_str());
      if (repo.count("command")) {
        r = system((("cd include/" + get<string>(repo["ipath"])) + "; " +
                    get<string>(repo["command"]))
                       .c_str());
      }
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

    for (map<string, toml::value> &repo : deps) {
      thread thrd(downloadDependency, repo);
      threads.push_back(move(thrd));
    }

    for (auto &thrd : threads) {
      if (thrd.joinable()) {
        thrd.join();
      }
    }

    r = system("rm -rf tmp");
  }
}

int rawComp(string file, string com = "g++", vector<string> args = {}) {
  string comm = join({com, file, join(args)});

  const char *ccomm = comm.c_str();

  return system(ccomm);
}

tuple<string, int> compO(string cppfile, string com = "g++",
                         vector<string> args = {}, int THREADID = 1,
                         string PROGRESS = "") {
  if (fileChanged(cppfile)) {
    CANPRINT.lock();
    cout << CYN << "[" << ORN << THREADID << ", " << PROGRESS << CYN
         << "]: " << GRN << cppfile.substr(2, cppfile.size() - 1) << RES
         << endl;
    CANPRINT.unlock();
    vector<string> nargs;
    if (getExt(cppfile) == "cpp" or getExt(cppfile) == "c") {
      nargs = {"-c", "-Ibuild"};
      if (SHAREDOBJECT) {
        nargs.push_back("-fPIC");
      }
    } else {
      nargs = {"-Ibuild"};
    }
    for (const string &arg : args) {
      if (!startsWith(arg, "-o") and !startsWith(arg, "-l")) {
        nargs.push_back(arg);
      } else if (startsWith(arg, "-l")) {
        if (getExt(cppfile) == "cpp" or getExt(cppfile) == "c") {
          nargs.push_back(arg);
        }
      } else if (startsWith(arg, "-o")) {
        if (getExt(cppfile) == "cpp" or getExt(cppfile) == "c") {
          nargs.push_back("-o" + string("./build/") +
                          removeDotSlash(cppfile + ".o"));
        } else {
          nargs.push_back("-o" + string("./build/") +
                          removeDotSlash(cppfile + ".gch"));
        }
      }
    }

    int code = rawComp(cppfile, com, nargs);

    return {"./build/" + replaceExt(cppfile, ".o"), code};
  } else {
    return {"./build/" + replaceExt(cppfile, ".o"), 0};
  }
}

void threadComp(vector<string> files, string com = "g++",
                vector<string> args = {}, int THREADID = 1) {
  if (files.size() == 0) {
    return;
  }
  int THREAD_PROGRESS = 0;
  int THREAD_COUNT = files.size();
  auto TIMESTART = chrono::system_clock::now();

  for (const string &file : files) {
    CANPRINT.lock();
    THREAD_PROGRESS++;
    GLOBAL_PROGRESS++;
    string prog = "G" + to_string(GLOBAL_PROGRESS) + "/" +
                  to_string(GLOBAL_COUNT) + " T" + to_string(THREAD_PROGRESS) +
                  "/" + to_string(THREAD_COUNT);
    CANPRINT.unlock();

    auto [ofile, scode] = compO(file, com, args, THREADID, prog);
    if (scode != 0) {
      cout << RED << "[" << ORN << "MORTAR" << RED << "]: ERROR" << RES << endl;
      exit(scode);
    }
  }

  auto TIMENOW = chrono::system_clock::now();
  CANPRINT.lock();
  cout << CYN << "[" << ORN << THREADID << CYN << "]: "
       << "Thread completed in "
       << chrono::duration_cast<chrono::seconds>(TIMENOW - TIMESTART).count()
       << " seconds\n"
       << RES;
  CANPRINT.unlock();
}

int oComp(string com = "g++", vector<string> args = {}) {
  auto MAINSTART = chrono::system_clock::now();
  string jargs = join(args);
  vector<string> wfiles;
  if (compileheaders) {
    wfiles = orderExts(filterFiles(getFiles(), {"c", "cpp", "h", "hpp"}));
  } else {
    wfiles = orderExts(filterFiles(getFiles(), {"c", "cpp"}));
  }

  vector<string> pfiles = {};

  for (const string &file : wfiles) {
    makedirs(file);
    if (fileChanged(file)) {
      pfiles.push_back(file);
    }
  }

  vector<thread> threads = {};

  GLOBAL_COUNT = pfiles.size();

  vector<vector<string>> sfiles = splitvs(pfiles, NTHREADS);

  int USED = 0;

  for (const vector<string> &chunk : sfiles) {
    if (chunk.size() > 0) {
      USED++;
    }
  }

  if (USED == 0) {
    cout << RED << "[" << ORN << "MORTAR" << RED << "]: No files to compile"
         << RES << endl;
    exit(0);
  }

  cout << CYN << "[" << ORN << "MORTAR" << CYN << "]: Found " << NTHREADS
       << " threads, using " << USED << RES << endl;

  for (const vector<string> &chunk : sfiles) {
    thread thrd(threadComp, chunk, com, args, threads.size() + 1);
    threads.push_back(move(thrd));
  }

  for (auto &thrd : threads) {
    if (thrd.joinable()) {
      thrd.join();
    }
  }

  vector<string> efiles = {};

  for (string &file : wfiles) {
    if (getExt(file) == "c" or getExt(file) == "cpp") {
      efiles.push_back(file + ".o");
    }
  }

  // define command to run to combile object files

  string jofiles = join(wrap(toBuild(efiles)));
  string comm;

  if (SHAREDOBJECT) {
    comm = join({com, jofiles, jargs, "-shared"});
  } else {
    comm = join({com, jofiles, jargs});
  }

  cout << CYN << "[" << ORN << "MORTAR" << CYN << "]: "
       << "Combining object files" << RES << endl;

  const char *ccomm = comm.c_str();

  auto MAINNOW = chrono::system_clock::now();

  int res = system(ccomm);
  cout << CYN << "[" << ORN << "MORTAR" << CYN << "]: Compilation completed in "
       << chrono::duration_cast<chrono::seconds>(MAINNOW - MAINSTART).count()
       << " seconds\n"
       << RES;
  return res;
}

bool configValueExists(std::map<toml::key, toml::value> table,
                       std::string key) {
  return table.count(key);
}

template <class T>
T getConfigValue(std::map<toml::key, toml::value> table, std::string key) {
  return get<T>(table.at(key));
}

std::map<toml::key, toml::value> tableToMap(toml::value table) {
  return toml::get<std::map<toml::key, toml::value>>(table);
}

bool configInherits(std::map<toml::key, toml::value> table) {
  if (configValueExists(table, "inherits")) {
    if (!configValueExists(tableToMap(CONFIG),
                           getConfigValue<std::string>(table, "inherits"))) {
      std::cout << "Inherited table is not defined" << std::endl;
      std::exit(1);
    }

    return true;
  }

  return false;
}

toml::value getConfigInheritance(std::map<toml::key, toml::value> table) {
  return getConfigValue<toml::value>(
      tableToMap(CONFIG), getConfigValue<std::string>(table, "inherits"));
}

template <class T>
T getInheritedValue(std::map<toml::key, toml::value> table, std::string key) {
  return getConfigValue<T>(tableToMap(getConfigInheritance(table)), key);
}

void compTarget(string target) {
  toml::table config;

  try {
    config = loadConfig();
  } catch (std::runtime_error) {
    cout << "No .mort file found, or it is improperly formatted" << endl;
    exit(1);
  }

  CONFIG = config;

  if (!exists("build")) {
    int r = system("mkdir build");
  }

  if (!config.count(target)) {

    cout << "Target " << target << " not found!" << endl;
    return;

  } else {

    if (config.count("deps")) {
      downloadDependencies(
          get<vector<map<string, toml::value>>>(config["deps"]));
    }

    string com = "g++";
    string oarg = "";
    string link = "";
    string out = "-oa.out";

    std::map<toml::key, toml::value> ctarg =
        toml::get<std::map<toml::key, toml::value>>(config.at(target));

    if (ctarg.count("com")) {
      com = get<string>(ctarg.at("com"));
    }

    if (ctarg.count("oarg")) {
      vector<string> coargs = get<vector<string>>(ctarg["oarg"]);
      oarg = join(coargs);
    }

    if (configInherits(ctarg)) {
      if (configValueExists(tableToMap(getConfigInheritance(ctarg)), "oarg")) {
        for (string const &li :
             getInheritedValue<vector<string>>(ctarg, "oarg")) {
          oarg += " " + li;
        }
      }
    }

    if (ctarg.count("l")) {
      for (string const &li : get<vector<string>>(ctarg["l"])) {
        link += "-l" + li + " ";
      }
    }

    if (configInherits(ctarg)) {
      if (configValueExists(tableToMap(getConfigInheritance(ctarg)), "l")) {
        for (string const &li : getInheritedValue<vector<string>>(ctarg, "l")) {
          link += "-l" + li + " ";
        }
      }
    }

    if (configInherits(ctarg)) {
      if (configValueExists(tableToMap(getConfigInheritance(ctarg)), "out")) {
        outname = getInheritedValue<string>(ctarg, "out");
        out = "-o" + outname;
      }
    }

    if (ctarg.count("out")) {
      outname = get<string>(ctarg.at("out"));
      out = "-o" + outname;
    }

    if (configInherits(ctarg)) {
      if (configValueExists(tableToMap(getConfigInheritance(ctarg)),
                            "threads")) {
        NTHREADS = getInheritedValue<int>(ctarg, "threads");
      }
    }

    if (ctarg.count("threads")) {
      NTHREADS = get<int>(ctarg.at("threads"));
    }

    if (configInherits(ctarg)) {
      if (configValueExists(tableToMap(getConfigInheritance(ctarg)),
                            "compileHeaders")) {
        compileheaders = getInheritedValue<bool>(ctarg, "compileHeaders");
      }
    }

    if (ctarg.count("compileHeaders")) {
      compileheaders = get<bool>(ctarg.at("compileHeaders"));
    }

    if (exists("include")) {
      oarg += " -Iinclude";
    }

    if (ctarg.count("before")) {
      int r = system(get<string>(ctarg["before"]).c_str());
      if (r != 0) {
        if (ctarg.count("beforeoptional")) {
          if (!get<bool>(ctarg["beforeoptional"])) {
            cout << RED << "[" << ORN << "MORTAR" << RED
                 << "]: Before command failed - set beforeoptional to true to "
                    "make optional"
                 << RES << endl;
            exit(1);
          }
        } else {
          cout << RED << "[" << ORN << "MORTAR" << RED
               << "]: Before command failed - set beforeoptional to true to "
                  "make optional"
               << RES << endl;
          exit(1);
        }
      }
    }

    if (ctarg.count("after")) {
      if (!oComp(com, {out, oarg, link})) {
        int rcode = system(get<string>(ctarg.at("after")).c_str());
      }
    } else {
      oComp(com, {out, oarg, link});
    }
  }
}

int main(int argc, char *argv[]) {
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
      for (const string &file :
           filterFiles(getFiles("./build"), {"mhsh", "ahsh", "o", "gch"})) {
        remove(file);
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