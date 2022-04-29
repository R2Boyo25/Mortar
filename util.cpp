#include "util.hpp"
using namespace std;
using namespace boost::filesystem;

namespace util {
    vector<string> removeDotSlash(vector<string> dotted) {
        vector<string> ndotted = {};
        for (const string& filename : dotted) {
            ndotted.push_back(filename.substr(2, filename.size() - 1));
        }
        return ndotted;
    }

    int Popen(string command, string STDIN) {
        FILE * pFile = popen(command.c_str(), "w");

        if(pFile == NULL)
            return 1;
        const char * psData = STDIN.c_str();
        size_t nNumWritten = fwrite(psData, 1, STDIN.size(), pFile);

        return pclose(pFile);
        pFile = NULL;
    }

    string readFile(string fname) {
        // https://stackoverflow.com/a/2602258/14639101
        std::ifstream t(fname);
        std::stringstream buffer;
        buffer << t.rdbuf();
        return buffer.str();
    }
 
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

    string join(vector<string> v, string delimiter) { // https://stackoverflow.com/a/20986194
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

    vector<string> getFiles(string dir) {
        vector<string> files = {};

        for(auto const& dir_entry: recursive_directory_iterator{dir}) {
            files.push_back(dir_entry.path().string());
        }

        return files;
    }

    vector<string> orderExts(vector<string> files) {
        vector<string> headers = {};
        vector<string> source  = {};

        for (string& file : files) {
            if (getExt(file) == "h" or getExt(file) == "hpp") {
                headers.push_back(file);
            } else if (getExt(file) == "c" or getExt(file) == "cpp") {
                source.push_back(file);
            }
        }

        vector<string> out = {};

        for (string& file : headers) {
            out.push_back(file);
        }

        for (string& file : source) {
            out.push_back(file);
        }

        return out;
    }

    vector<string> filterFiles(vector<string> files, vector<string> exts) {
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

    std::vector< std::vector<string> > splitvs(std::vector<string> vec, int n) {
        std::vector< std::vector<string> > vec_of_vecs(n); // https://stackoverflow.com/a/48739993

        int quotient = vec.size() / n;
        int remainder = vec.size() % n;
        int first = 0;
        int last;
        for (int i = 0; i < n; ++i) {
            if (i < remainder) {
                last = first + quotient + 1;
                vec_of_vecs[i] = std::vector<string>(vec.begin() + first, vec.begin() + last);
                first = last;
            } else if (i != n - 1) {
                last = first +  quotient;
                vec_of_vecs[i] = std::vector<string>(vec.begin() + first, vec.begin() + last);
                first = last;
            } else {
                vec_of_vecs[i] = std::vector<string>(vec.begin() + first, vec.end());
            }
        }

        return vec_of_vecs;
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

    void makedirs(std::string filename) {
        int r = system(("mkdir -p $(dirname \"./build/" + filename + "\")").c_str());
    }

    std::vector<std::string> toBuild(std::vector<std::string> files) {
        std::vector<std::string> ofiles = {};

        for (string& file : files) {
            ofiles.push_back("./build/" + removeDotSlash(file));
        }

        return ofiles;
    }

    std::string toBuild(std::string file) {
        return "./build/" + removeDotSlash(file);
    }

    std::string removeDotSlash(std::string filename) {
        return filename.substr(2, filename.size() - 1);
    }

    std::string lstrip(std::string text, std::string toremove) {
        while (string(1, text[0]) == toremove) {
            text = text.erase(0, 1);
        }

        return text;
    }

    std::string rstrip(std::string text, std::string toremove) {
        while (string(1, text[text.size()-1]) == toremove) {
            text = text.erase(text.size()-1, 1);
        }

        return text;
    }

    std::string strip(std::string text, std::string toremove) {
        text = lstrip(text, toremove);
        text = rstrip(text, toremove);

        return text;
    }

    std::string stripComment(std::string text) {
        if (text.find("//") != string::npos) {
            text = text.erase(text.find("//") - 1);
        }

        return text;
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
}