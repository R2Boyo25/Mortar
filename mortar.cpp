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
//#include <toml/toml.hpp>
#include <tuple>

#include <clipp/clipp.h>

#include "changed.hpp"
//#include "configutils.hpp"
#include "util.hpp"
#include "parser/parse.hpp"

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
bool DEBUG = false;
bool SILENT = false;
mutex CANPRINT;
mutex MODIFY_GLOBALS;
int NTHREADS = std::thread::hardware_concurrency();
int GLOBAL_COUNT = 0;
int GLOBAL_PROGRESS = 0;
//toml::table CONFIG;
Config CONFIG;
string TARGETNAME = "";
std::string outname;
string COMPILER = "";
std::vector<std::string> EXCLUDE = {};
std::vector<std::string> INCLUDE = {};
std::vector<std::string> deplinks = {};
std::map<std::string, std::string> standardvars = {};
std::vector<std::string> assumenewfiles = {};
std::vector<std::string> assumeoldfiles = {};
std::vector<std::string> relinkif = {};

/*int rawComp(string file, string com = "g++", vector<string> args = {}) {
  auto cargs = args;

  for (auto &so : deplinks) {
    cargs.push_back(so);
  }

  string comm = join({com, file, join(cargs)});

  const char *ccomm = comm.c_str();

  return System(ccomm, standardvars);
}*/

/*tuple<string, int> compO(string cppfile, string com = "g++",
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
  }*/

/*int linkObjects(std::string com, std::vector<std::string> cfiles, std::vector<std::string> args) {
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
}*/

/*int oComp(string com = "g++", vector<string> args = {}) {
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
}*/

bool envvar(char *name) {
  auto v = getenv(name);
  if (v != NULL) {
    return std::stoi(v);
  }

  return false;
}

int mortar_main() {
  if (!NTHREADS) {
    NTHREADS = 1;
  }

  if (TARGETNAME == "") {
    return CONFIG.processTarget("_default");
  }

  return CONFIG.processTarget(TARGETNAME);
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
       option("-d", "--debug")
           .set(DEBUG)
           .doc("Print debugging information."),
       option("-h", "--help")
           .set(selected, mode::help)
           .doc("Print this message and exit."),
       (option("-j", "--jobs") & number("N", NTHREADS))
           .doc("Allow N jobs at once."),
       option("--no-silent")
           .set(SILENT, false)
           .doc("Echo recipes (disable --silent mode)"),
       (option("-o", "--out") & value("FILE", outname))
           .doc("Linker output to FILE."),
       repeatable( option("-O", "--old-file", "--assume-old")
                   & value("FILE", assumeoldfiles) )
           .doc("Consider FILE to be very old and don't rebuild it."),
       option("-s", "--silent", "--quiet")
           .set(SILENT, true)
           .doc("Don't echo recipes."),
       option("-v", "--version")
           .set(selected, mode::version)
           .doc("Print Mortar version and exit."),
       /*option("-so", "--sharedobject")
           .set(SHAREDOBJECT)
           .doc("Generate a shared object instead of a binary."),*/
       repeatable( option("-W", "--what-if", "--new-file", "--assume-new")
                   & value("FILE", assumenewfiles) )
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
  //navigateDirectories();
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

  CONFIG = Config("mortar.mort");
  
  return res || makeCLI(argc, argv);
}
