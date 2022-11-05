#include "util.hpp"
using namespace std;
using namespace std::filesystem;

namespace util {
vector<string> removeDotSlash(vector<string> dotted) {
  vector<string> ndotted = {};
  for (const string &filename : dotted) {
    if (filename[0] == '.' && filename[1] == '/') {
      ndotted.push_back(filename.substr(2));
      continue;
    }
    ndotted.push_back(filename);
  }
  return ndotted;
}

TEST_CASE("[vector] removeDotSlash") {
  std::vector<std::string> output;
  std::vector<std::string> testve;

  testve = {"./test.a"};
  output = {"test.a"};
  CHECK(removeDotSlash(testve) == output);
  testve = {"test.a"};
  output = {"test.a"};
  CHECK(removeDotSlash(testve) == output);
  testve = {"test.a", "./test2.b"};
  output = {"test.a", "test2.b"};
  CHECK(removeDotSlash(testve) == output);
}

int Popen(string command, string STDIN) {
  FILE *pFile = popen(command.c_str(), "w");

  if (pFile == NULL)
    return 1;
  const char *psData = STDIN.c_str();
  size_t nNumWritten = fwrite(psData, 1, STDIN.size(), pFile);

  return pclose(pFile);
  pFile = NULL;
}

string readFile(string fname) { // https://stackoverflow.com/a/2602258/14639101
  std::ifstream t(fname);
  std::stringstream buffer;
  buffer << t.rdbuf();
  return buffer.str();
}

vector<string> split(string splitting,
                     char delimiter) { // https://stackoverflow.com/a/10058725

  stringstream tosplit(splitting);
  string segment;
  vector<string> seglist;

  while (getline(tosplit, segment, delimiter)) {
    seglist.push_back(segment);
  }

  return seglist;
}

TEST_CASE("split") {
  std::vector<std::string> output = {};

  output = {"yes", "no", "true", "false"};
  CHECK(split("yes no true false", ' ') == output);

  output = {"path", "to", "file"};
  CHECK(split("path/to/file", '/') == output);
}

string join(vector<string> v,
            string delimiter) { // https://stackoverflow.com/a/20986194
  stringstream ss;
  const int v_size = v.size();
  for (size_t i = 0; i < v_size; ++i) {
    if (i != 0)
      ss << delimiter;
    ss << v[i];
  }
  string s = ss.str();

  return s;
}

TEST_CASE("join") {
  CHECK(join({"yes", "no", "true", "false"}, " ") == "yes no true false");

  CHECK(join({"path", "to", "file"}, "/") == "path/to/file");
}

vector<string> wrap(vector<string> towrap) {
  vector<string> wrappedv = {};

  for (string const &arg : towrap) {
    string wrapped = "\"" + arg + "\"";
    wrappedv.push_back(wrapped);
  }

  return wrappedv;
}

TEST_CASE("wrap") {
  std::vector<std::string> output;

  output = {"\"hallo\"", "\"auf wiedersehen\""};
  CHECK(wrap({"hallo", "auf wiedersehen"}) == output);
}

vector<string> getFiles(string dir) {
  vector<string> files = {};

  for (auto const &dir_entry : recursive_directory_iterator{dir}) {
    files.push_back(dir_entry.path());
  }

  return files;
}

vector<string> orderExts(vector<string> files) {
  vector<string> headers = {};
  vector<string> source = {};

  for (string &file : files) {
    if (getExt(file) == "h" or getExt(file) == "hpp") {
      headers.push_back(file);
    } else if (getExt(file) == "c" or getExt(file) == "cpp") {
      source.push_back(file);
    }
  }

  vector<string> out = {};

  for (string &file : headers) {
    out.push_back(file);
  }

  for (string &file : source) {
    out.push_back(file);
  }

  return out;
}

TEST_CASE("orderExts") {
  std::vector<std::string> output;

  output = {"a.h", "d.hpp", "b.c", "c.cpp"};
  CHECK(orderExts({"a.h", "b.c", "c.cpp", "d.hpp"}) == output);
}

vector<string> filterFiles(vector<string> files, vector<string> exts) {
  vector<string> ffiles = {};

  for (string const &file : files) {
    string sfile = split(file, '/').back();
    if (sfile.find(".") == string::npos) {
      continue;
    }

    string fext = split(sfile, '.').back();
    bool inexts = false;
    for (string const &ext : exts) {
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

TEST_CASE("filterFiles") {
  std::vector<std::string> output;

  output = {"a.c", "c.cpp"};
  CHECK(filterFiles({"a.c", "b.txt", "c.cpp", "d.toml"}, {"c", "cpp"}) ==
        output);
}

string replaceExt(string filename, string newext) {
  vector<string> splt = split(filename, '.');
  splt[splt.size() - 1] = newext;
  return join(splt, ".");
}

TEST_CASE("[string] replaceExt") {
  CHECK(replaceExt("abc.cpp", "o") == "abc.o");
  CHECK(replaceExt("abc.h", "gch") == "abc.gch");
}

vector<string> replaceExts(vector<string> files, string newext) {
  vector<string> newfiles = {};
  for (string const &file : files) {
    newfiles.push_back(replaceExt(file, newext));
  }
  return newfiles;
}

TEST_CASE("[vector] replaceExts") {
  std::vector<std::string> output;

  output = {"abc.o", "def.o"};
  CHECK(replaceExts({"abc.cpp", "def.c"}, "o") == output);
}

bool startsWith(string str, string strwth) { return str.rfind(strwth, 0) == 0; }

TEST_CASE("startsWith") {
  CHECK(startsWith("abcd", "ab"));
  CHECK_FALSE(startsWith("a_bcd", "ab"));
}

std::vector<std::vector<string>> splitvs(std::vector<string> vec, int n) {
  std::vector<std::vector<string>> vec_of_vecs(
      n); // https://stackoverflow.com/a/48739993

  int quotient = vec.size() / n;
  int remainder = vec.size() % n;
  int first = 0;
  int last;
  for (int i = 0; i < n; ++i) {
    if (i < remainder) {
      last = first + quotient + 1;
      vec_of_vecs[i] =
          std::vector<string>(vec.begin() + first, vec.begin() + last);
      first = last;
    } else if (i != n - 1) {
      last = first + quotient;
      vec_of_vecs[i] =
          std::vector<string>(vec.begin() + first, vec.begin() + last);
      first = last;
    } else {
      vec_of_vecs[i] = std::vector<string>(vec.begin() + first, vec.end());
    }
  }

  return vec_of_vecs;
}

TEST_CASE("splitvs") {
  std::vector<std::vector<std::string>> output;

  output = {{"a", "b", "c"}, {"d", "e"}};
  CHECK(splitvs({"a", "b", "c", "d", "e"}, 2) == output);
}

std::string getExt(std::string filename) {
  if (filename == "") {
    return "";
  }

  string sfile = split(filename, '/').back();
  if (sfile.find(".") == string::npos) {
    return "";
  }

  string fext = split(sfile, '.').back();

  return fext;
}

TEST_CASE("getExt") {
  CHECK(getExt("abc.txt") == "txt");
  CHECK(getExt("defg.cpp") == "cpp");
  CHECK(getExt("hi.toml") == "toml");
}

void makedirs(std::string filename) {
  int r = system(("mkdir -p $(dirname \"./build/" + filename + "\")").c_str());
}

std::vector<std::string> toBuild(std::vector<std::string> files) {
  std::vector<std::string> ofiles = {};

  for (string &file : files) {
    ofiles.push_back("./build/" + removeDotSlash(file));
  }

  return ofiles;
}

TEST_CASE("[vector] toBuild") {
  std::vector<std::string> inputv;
  std::vector<std::string> output;

  inputv = {"a.cpp", "dir/b.cpp"};
  output = {"./build/a.cpp", "./build/dir/b.cpp"};
  CHECK(toBuild(inputv) == output);

  inputv = {"./a.cpp", "./dir/b.cpp"};
  output = {"./build/a.cpp", "./build/dir/b.cpp"};
  CHECK(toBuild(inputv) == output);
}

std::string toBuild(std::string file) {
  return "./build/" + removeDotSlash(file);
}

TEST_CASE("[string] toBuild") {
  CHECK(toBuild("a.cpp") == "./build/a.cpp");
  CHECK(toBuild("./a.cpp") == "./build/a.cpp");
  CHECK(toBuild("dir/b.cpp") == "./build/dir/b.cpp");
  CHECK(toBuild("./dir/b.cpp") == "./build/dir/b.cpp");
}

std::string removeDotSlash(std::string filename) {
  if (filename[0] == '.' && filename[1] == '/') {
    return filename.substr(2, filename.size() - 1);
  }
  return filename;
}

TEST_CASE("[string] removeDotSlash") {
  CHECK(removeDotSlash("./a.cpp") == "a.cpp");
  CHECK(removeDotSlash("a.cpp") == "a.cpp");
}

std::string lstrip(std::string text, std::string toremove) {
  while (string(1, text[0]) == toremove) {
    text = text.erase(0, 1);
  }

  return text;
}

TEST_CASE("lstrip") {
  CHECK(lstrip("aaab", "a") == "b");
  CHECK(lstrip("b", "a") == "b");
  CHECK(lstrip("aaaaa", "a") == "");
}

std::string rstrip(std::string text, std::string toremove) {
  while (string(1, text[text.size() - 1]) == toremove) {
    text = text.erase(text.size() - 1, 1);
  }

  return text;
}

TEST_CASE("rstrip") {
  CHECK(rstrip("baaa", "a") == "b");
  CHECK(rstrip("b", "a") == "b");
  CHECK(rstrip("aaaaa", "a") == "");
}

std::string strip(std::string text, std::string toremove) {
  text = lstrip(text, toremove);
  text = rstrip(text, toremove);

  return text;
}

TEST_CASE("strip") {
  CHECK(strip("baaa", "a") == "b");
  CHECK(strip("b", "a") == "b");
  CHECK(strip("aaaaa", "a") == "");
  CHECK(strip("aaaabaaa", "a") == "b");
}

std::string stripComment(std::string text) {
  if (text.find("//") != string::npos) {
    auto pos = text.find("//");
    int offset = 0;

    if (pos > 0) {
      if (text[pos - 1] == ' ') {
        offset = 1;
      }
    }

    text = text.erase(pos - offset);
  }

  return text;
}

TEST_CASE("stripComment") {
  CHECK(stripComment("// yes") == "");
  CHECK(stripComment("a // yes") == "a");
  CHECK(stripComment("b// yes") == "b");
  CHECK(stripComment("ccc") == "ccc");
  CHECK(stripComment("") == "");
}

std::string dirName(std::string filename) {
  string directory;
  const size_t last_slash_idx = filename.rfind('/');
  if (std::string::npos != last_slash_idx) {
    return filename.substr(0, last_slash_idx);
  } else {
    return "./";
  }
}

TEST_CASE("dirName") {
  CHECK(dirName("aa/bb.cc") == "aa");
  CHECK(dirName("aa/bb/cc.dd") == "aa/bb");
  CHECK(dirName("bb.cc") == "./");
}

} // namespace util