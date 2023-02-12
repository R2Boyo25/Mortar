#include "changed.hpp"

using namespace std;
using namespace std::filesystem;
using namespace util;

extern bool compileheaders;
map<string, vector<string>> gincludes = {};

namespace changed {
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
      string fname = strip(line.substr(8, line.size() - 1));
      if (fname[0] != '<') {
        fname = strip(fname, "\"");

        out.push_back(getPath(dirName(filename), fname));
        for (string &f : includes(getPath(dirName(filename), fname))) {
          out.push_back(getPath(dirName(filename), f));
        }
      }
    }
  }

  return out;
}

vector<string> includes(string filename, string filevec) {
  vector<string> out = {};
  string content = readFile(filename);
  if (gincludes.find(filevec) != gincludes.end() && filename != "") {
    for (string &line : split(content, '\n')) {
      if (startsWith(line, "#include")) {
        line = stripComment(line);
        string fname = strip(line.substr(8, line.size() - 1));
        if (fname[0] != '<') {
          fname = strip(fname, "\"");
          if (!(std::find(gincludes[filevec].begin(), gincludes[filevec].end(),
                          getPath(dirName(filename), fname)) !=
                gincludes[filevec].end())) {
            gincludes[filevec].push_back(getPath(dirName(filename), fname));
            for (string &f :
                 includes(getPath(dirName(filename), fname), filevec)) {
              gincludes[filevec].push_back(getPath(dirName(filename), f));
            }
            out.push_back(getPath(dirName(filename), fname));
            for (string &f :
                 includes(getPath(dirName(filename), fname), filevec)) {
              out.push_back(getPath(dirName(filename), f));
            }
          }
        }
      }
    }
  }

  return out;
}

vector<string> includesChanged(string filename) {
  vector<string> changed = {};

  for (string &f : includes(filename, filename)) {
    if (fileChanged(f, filename)) {
      changed.push_back(f);
    }
  }

  return changed;
}

vector<string> includesChanged(string filename, string filevec) {
  vector<string> changed = {};

  for (string &f : includes(filename, filevec)) {
    if (fileChanged(f, filevec)) {
      changed.push_back(f);
    }
  }

  return changed;
}

file_time_type modTime(string fname) {
  return std::filesystem::last_write_time(fname);
}

  bool fileChanged(string filename) {
  gincludes[filename] = {};
  bool modtimegreater = false;
  if (exists(outname)) {
    if (exists("mortar.mort"))
      modtimegreater = modTime("mortar.mort") > modTime(outname) || modtimegreater;
    if (modtimegreater) {
      return true;
    } else if (includesChanged(filename, filename).size() > 0) {
      return true;
    } else {
      return (modTime(filename) > modTime(outname));
    }
  }
  
  return true;
}

bool fileChanged(string filename, string filevec) {
  bool modtimegreater = false;
  if (exists(outname)) {
    if (exists("mortar.mort"))
      modtimegreater = modTime("mortar.mort") > modTime(outname) || modtimegreater;
    if (modtimegreater) {
      return true;
    } else if (includesChanged(filename, filevec).size() > 0) {
      return true;
    } else {
      return (modTime(filename) > modTime(outname));
    }
  }

  return true;
}
} // namespace changed
