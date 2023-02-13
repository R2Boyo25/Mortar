#include "doctest.h"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
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

#include <clipp/clipp.h>

#include "changed.hpp"
#include "configutils.hpp"
#include "util.hpp"

using namespace std;
using namespace std::filesystem;
using namespace util;
using namespace color;
using namespace changed;
using namespace clipp;

#ifndef COMMITHASH
#define COMMITHASH "UNKNOWN"
#endif

#ifndef VERSION
#define VERSION "UNKNOWN"
#endif

bool ERRORFOUND = false;
mutex CANPRINT;
mutex MODIFY_GLOBALS;
int NTHREADS = std::thread::hardware_concurrency();
int GLOBAL_COUNT = 0;
int GLOBAL_PROGRESS = 0;
string outname = "a.out";
bool compileheaders = false;
bool SHAREDOBJECT = false;
toml::table CONFIG;
string runcommand = "";
string TARGETNAME = "";
string COMPILER = "";
std::vector<std::string> EXCLUDE = {};
std::vector<std::string> INCLUDE = {};
bool DEBUG = false;
std::vector<std::string> deplinks = {};
std::map<std::string, std::string> standardvars = {};
std::vector<std::string> assumenewfiles = {};
std::vector<std::string> assumeoldfiles = {};
std::vector<std::string> relinkif = {};
bool RELINK = false;

std::string escapeQuotes(std::string content) {
  std::string ocontent = content;

  std::regex re1("\\\"");

  ocontent = std::regex_replace(ocontent, re1, "\\\"");

  std::regex re2("\"");

  ocontent = std::regex_replace(ocontent, re2, "\"");

  return ocontent;
}

int System(std::string command,
           std::map<std::string, std::string> envvars = {}) {
  std::string processedcommand;

  processedcommand = escapeQuotes(command);

  for (auto &var : envvars) {
    processedcommand =
        escapeQuotes("export " + var.first + "=\"" + var.second + "\"; ") +
        processedcommand;
  }

  std::regex re1("\\$");

  processedcommand = std::regex_replace(processedcommand, re1, "\\$");

  processedcommand = "bash -c \"" + processedcommand + "\"";

  if (DEBUG) {
    CANPRINT.lock();
    std::cout << processedcommand << std::endl;
    CANPRINT.unlock();
  }

  return system(processedcommand.c_str());
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
  }
  // I have no idea what I just wrote here,
  // it is a mess because I didn't feel like messing with folder copying in
  // C++ r is to get the compiler to stop complaining
  if (!exists("include/" + get<string>(repo["ipath"]))) {
    string user =
        split(get<string>(repo["url"]),
              BACKSLASH)[split(get<string>(repo["url"]), BACKSLASH).size() - 2];
    string gitrepo =
        split(get<string>(repo["url"]),
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

    std::vector<std::string> exclude = {};
    if (configValueExists(repo, "exclude")) {
      exclude = getConfigValue<std::vector<std::string>>(repo, "exclude");
    }

    std::vector<std::string> include = {};
    if (configValueExists(repo, "include")) {
      include = getConfigValue<std::vector<std::string>>(repo, "include");
    }

    for (auto &file : getExcluded(include, exclude, getFiles("./"))) {
      r = system(("rm -rf " + folder + file).c_str());
    }

    r = system(
        ("mkdir -p $(dirname \"./include/" + get<string>(repo["ipath"]) + "\")")
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

  if (configValueExists(repo, "link_paths")) {
    for (auto &rgx :
         getConfigValue<std::vector<std::string>>(repo, "link_paths")) {
      for (auto &so : filterFilesRegex(
               "include/" + rgx,
               getFiles("include/" + get<std::string>(repo["ipath"])))) {
        deplinks.push_back("include/" + get<std::string>(repo["ipath"]) + "/" +
                           so);
      }
    }
  }

  if (configValueExists(repo, "exclude_from_compilation")) {
    if (getConfigValue<bool>(repo, "exclude_from_compilation")) {
      EXCLUDE.push_back("include/" + get<std::string>(repo["ipath"]) + "/.*");
    }
  }

  (void)r;
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

  (void)r;
}

int rawComp(string file, string com = "g++", vector<string> args = {}) {
  auto cargs = args;

  for (auto &so : deplinks) {
    cargs.push_back(so);
  }

  string comm = join({com, file, join(cargs)});

  const char *ccomm = comm.c_str();

  return System(ccomm, standardvars);
}

tuple<string, int> compO(string cppfile, string com = "g++",
                         vector<string> args = {}, int THREADID = 1,
                         string PROGRESS = "") {
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

int linkObjects(std::string com, std::vector<std::string> cfiles, std::vector<std::string> args) {
  std::string j_args  = join(args);

  vector<string> ofiles = {};

  for (string &file : cfiles) {
    if (getExt(file) == "c" or getExt(file) == "cpp") {
      ofiles.push_back(file + ".o");
    }
  }
  
  std::string j_files = join(wrap(toBuild(ofiles)));

  std::string linkcommand;

  if (SHAREDOBJECT) {
    linkcommand = join({com, j_files, j_args, "-shared"});
  } else {
    linkcommand = join({com, j_files, j_args});
  }

  cout << CYN << "[" << ORN << "MORTAR" << CYN << "]: "
       << "Linking..." << RES << endl;
  
  return System(linkcommand, standardvars);;
}

int oComp(string com = "g++", vector<string> args = {}) {
  auto MAINSTART = chrono::system_clock::now();
  string jargs = join(args);
  vector<string> wfiles;

  if (compileheaders) {
    wfiles = orderExts(includeExclude(
        INCLUDE, EXCLUDE, filterFiles(getFiles(), {"c", "cpp", "h", "hpp"})));
  } else {
    wfiles = orderExts(includeExclude(INCLUDE, EXCLUDE,
                                      filterFiles(getFiles(), {"c", "cpp"})));
  }

  vector<string> pfiles = {};

  std::vector<std::string> excludeall = {".*"};

  for (auto& relinkfile : includeExclude(relinkif, excludeall, getFiles())) {
    if (fileChanged(relinkfile)) {
      RELINK = true;
      break;
    }
  }

  for (const string &file : wfiles) {
    makedirs(file);
    std::vector<std::string> vfile = {file};
    
    bool assumed = includeExclude(assumenewfiles, excludeall, vfile).size() > 0;
    bool assumed_old = includeExclude(assumeoldfiles, excludeall, vfile).size() > 0;
    if (assumed) {
      if (DEBUG) {
        CANPRINT.lock();
        std::cout << "Assuming " << file << " is new." << std::endl;
        CANPRINT.unlock();
      }
    }

    if (assumed_old) {
      if (DEBUG) {
        CANPRINT.lock();
        std::cout << "Assuming " << file << " is old." << std::endl;
        CANPRINT.unlock();
      }
    }
    
    if ((fileChanged(file) || assumed) && !assumed_old) {
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
    if (RELINK) {
      int res = linkObjects(com, wfiles, args);
      (void)res;

      auto LINKDONE = chrono::system_clock::now();
  
      cout << CYN << "[" << ORN << "MORTAR" << CYN << "]: Done. Linking took "
           << chrono::duration_cast<chrono::seconds>(LINKDONE - MAINSTART).count() << " seconds\n"
           << RES;
    }
    
    if (runcommand != "") {
      int rcode = System(runcommand.c_str(), standardvars);
      exit(rcode);
    }

    cout << RED << "[" << ORN << "MORTAR" << RED << "]: No files to compile"
         << RES << endl;
    exit(0);
  }

  cout << CYN << "[" << ORN << "MORTAR" << CYN << "]: " << NTHREADS
       << " threads available, using " << USED << RES << endl;

  for (const vector<string> &chunk : sfiles) {
    thread thrd(threadComp, chunk, com, args, threads.size() + 1);
    threads.push_back(move(thrd));
  }

  for (auto &thrd : threads) {
    if (thrd.joinable()) {
      thrd.join();
    }
  }

  auto COMPILATIONDONE = chrono::system_clock::now();

  int res = linkObjects(com, wfiles, args);

  auto LINKDONE = chrono::system_clock::now();
  
  cout << CYN << "[" << ORN << "MORTAR" << CYN << "]: Done. Compilation took "
       << chrono::duration_cast<chrono::seconds>(COMPILATIONDONE - MAINSTART).count()
       << " seconds and linking took " << chrono::duration_cast<chrono::seconds>(LINKDONE - COMPILATIONDONE).count() << " seconds\n"
       << RES;
  return res;
}

bool envvar(char *name) {
  auto v = getenv(name);
  if (v != NULL) {
    return std::stoi(v);
  }

  return false;
}

void compTarget(string target) {
  toml::table config;

  try {
    config = loadConfig();
  } catch (std::runtime_error const&) {
    cout << "No .mort / mortar.toml file found, or it is improperly formatted"
         << endl;
    exit(1);
  }

  CONFIG = config;

  if (!exists("build")) {
    int r = system("mkdir build");
    (void)r;
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

    if (configValueExists(ctarg, "type")) {
      if (getConfigValue<string>(ctarg, "type") == "command") {
        if (!configValueExists(ctarg, "target")) {
          std::cout
              << "Target type is set to command but is missing `target` key"
              << std::endl;
          std::exit(1);
        }

        if (!configValueExists(ctarg, "target")) {
          std::cout
              << "Target type is set to command but is missing `command` key"
              << std::endl;
          std::exit(1);
        }

        runcommand = getConfigValue<string>(ctarg, "command");

        target = getConfigValue<string>(ctarg, "target");

        ctarg = toml::get<std::map<toml::key, toml::value>>(config.at(target));
      }
    }

    if (configInherits(ctarg)) {
      if (configValueExists(tableToMap(getConfigInheritance(ctarg)), "com")) {
        com = getInheritedValue<std::string>(ctarg, "com");
      }
    }

    if (ctarg.count("com")) {
      com = get<string>(ctarg.at("com"));
    }

    if (getenv((char *)"DISTCC_HOSTS") != NULL) {
      com = "distcc";
    }

    if (COMPILER != "") {
      com = COMPILER;
    }

    if (ctarg.count("exclude")) {
      for (string const &li :
           getConfigValue<vector<string>>(ctarg, "exclude")) {
        EXCLUDE.push_back(li);
      }
    }

    if (configInherits(ctarg)) {
      if (configValueExists(tableToMap(getConfigInheritance(ctarg)),
                            "exclude")) {
        for (string const &li :
             getInheritedValue<vector<string>>(ctarg, "exclude")) {
          EXCLUDE.push_back(li);
        }
      }
    }

    if (ctarg.count("include")) {
      INCLUDE = get<vector<string>>(ctarg["include"]);
    }

    if (configInherits(ctarg)) {
      if (configValueExists(tableToMap(getConfigInheritance(ctarg)),
                            "include")) {
        for (string const &li :
             getInheritedValue<vector<string>>(ctarg, "include")) {
          INCLUDE.push_back(li);
        }
      }
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

    if (ctarg.count("relink")) {
      for (auto& reg : get<vector<string>>(ctarg["relink"])) {
        relinkif.push_back(reg);
      }
    }

    if (configInherits(ctarg)) {
      if (configValueExists(tableToMap(getConfigInheritance(ctarg)), "relink")) {
        for (auto& reg : getInheritedValue<vector<string>>(ctarg, "relink")) {
          relinkif.push_back(reg);
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

    if (outname == "a.out") {

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

    } else {
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

    if (configInherits(ctarg)) {
      if (configValueExists(tableToMap(getConfigInheritance(ctarg)), "env")) {
        for (auto &item : getInheritedValue<std::map<std::string, toml::value>>(
                 ctarg, "env")) {
          standardvars[item.first] = get<std::string>(item.second);
        }
      }
    }

    if (configValueExists(ctarg, "env")) {
      for (auto &item :
           getConfigValue<std::map<std::string, toml::value>>(ctarg, "env")) {
        standardvars[item.first] = get<std::string>(item.second);
      }
    }

    standardvars["EXECUTABLE"] = "./" + outname;

    bool hasbefore = false;
    std::string before = "";

    if (configInherits(ctarg)) {
      if (configValueExists(tableToMap(getConfigInheritance(ctarg)), "before")) {
        before = getInheritedValue<std::string>(ctarg, "before");
        hasbefore = true;
      }
    }

    if (configValueExists(ctarg, "before")) {
      before = getConfigValue<std::string>(ctarg, "before");
      hasbefore = true;
    }

    bool beforeoptional = false;
    
    if (configInherits(ctarg)) {
      if (configValueExists(tableToMap(getConfigInheritance(ctarg)), "beforeoptional")) {
        beforeoptional = getInheritedValue<bool>(ctarg, "beforeoptional");
      }
    }

    if (configValueExists(ctarg, "beforeoptional")) {
      beforeoptional = getConfigValue<bool>(ctarg, "beforeoptional");
    }
    
    
    if (hasbefore) {
      int r = System(before.c_str(), standardvars);
      if (r != 0) {
        if (!beforeoptional) {
          cout << RED << "[" << ORN << "MORTAR" << RED
               << "]: Before command failed - set beforeoptional to true to "
            "make optional"
               << RES << endl;
          exit(1);
        }
      }
    }

    bool hasafter = false;
    std::string after = "";

    if (configInherits(ctarg)) {
      if (configValueExists(tableToMap(getConfigInheritance(ctarg)), "after")) {
        after = getInheritedValue<std::string>(ctarg, "after");
        hasafter = true;
      }
    }

    if (configValueExists(ctarg, "after")) {
      after = getConfigValue<std::string>(ctarg, "after");
      hasafter = true;
    }
    

    if (hasafter) {
      if (!oComp(com, {out, oarg, link})) {
        int rcode =
            System(after.c_str(), standardvars);
        (void)rcode;
      }
    } else {
      oComp(com, {out, oarg, link});
    }

    if (runcommand != "") {
      int rcode = System(runcommand.c_str(), standardvars);
      (void)rcode;
    }
  }
}

int mortar_main() {
  if (!NTHREADS) {
    NTHREADS = 1;
  }

  if (TARGETNAME == "") {
    compTarget("_default");
    return 0;
  }

  compTarget(TARGETNAME);

  return 0;
}

int mortar_clean() {
  for (const string &file :
       filterFiles(getFiles("./build"), {"mhsh", "ahsh", "o", "gch"})) {
    remove(file);
  }
  return 0;
}

int mortar_version() {
  std::cout << "Mortar v" << VERSION << " (Commit " << COMMITHASH << ")"
            << std::endl;
  return 0;
}

auto manpage(group cli) {
  auto fmt = doc_formatting().split_alternatives(true);
  return make_man_page(cli, "mortar", fmt)
      .prepend_section("DESCRIPTION", "\tCopyright (C) Kazani 2022\n\tMortar - "
                                      "Easy to use C++ build system.")
      .append_section("LICENSE", "\tLGPLv3");
  ;
}

int makeCLI(int argc, char **argv) {
  enum class mode { help, clean, version, none };

  mode selected = mode::none;

  auto opts =
       ((option("-c", "--compiler") & value("COMPILER", COMPILER))
           .doc("Use COMPILER to compile files."),
       option("-d", "--debug").set(DEBUG).doc("Print debugging information."),
       option("-h", "--help")
           .set(selected, mode::help)
           .doc("Print this message and exit."),
       (option("-j", "--jobs") & number("N", NTHREADS))
           .doc("Allow N jobs at once."),
       repeatable(option("-o", "--old-file", "--assume-old")
                   & value("FILE", assumeoldfiles) )
           .doc("Consider FILE to be very old and don't rebuild it."),
       (option("-O", "--out") & value("FILE", outname))
           .doc("Linker output to FILE."),
       option("-so", "--sharedobject")
           .set(SHAREDOBJECT)
           .doc("Generate a shared object instead of a binary."),
       option("-v", "--version")
           .set(selected, mode::version)
           .doc("Print Mortar version and exit."),
       repeatable(option("-W", "--what-if", "--new-file", "--assume-new")
                   & value("FILE", assumenewfiles))
           .doc("Consider FILE to be infinitely new."));

  auto cli = ((opt_value("target", TARGETNAME),
               opts) |
              command("clean").set(selected, mode::clean));

  if (parse(argc, argv, cli)) {
    switch (selected) {
    case mode::clean:
      return mortar_clean();
      break;
    case mode::help:
      std::cout << manpage(cli);
      return 0;
      break;
    case mode::version:
      return mortar_version();
      break;
    case mode::none:
      return mortar_main();
      break;
    }
  }

  return mortar_main();
}

int main(int argc, char *argv[]) {
  navigateDirectories();
#ifndef DOCTEST_CONFIG_DISABLE
  doctest::Context context;

  context.setOption("abort-after", 5);

  context.applyCommandLine(argc, argv);

  int res = context.run();
  if (context.shouldExit() || res || envvar((char *)"DOCTEST_EXIT"))
    return res;
#else
  int res = 0;
#endif

  return res || makeCLI(argc, argv);
}
