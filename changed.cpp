#include "changed.hpp"

using namespace std;
using namespace std::filesystem;
using namespace util;

namespace changed {
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
        string fname = "./build/" + removeDotSlash(filename + ".mhsh");//replaceExt(filename, "mhsh");
        writeFile(fname, genHash(filename));
    }

    bool fileChanged(string filename) {
        string fname = "./build/" + removeDotSlash(filename + ".mhsh");//replaceExt(filename, "mhsh");
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
}