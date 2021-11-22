#include "conf.hpp"

using namespace util;

using json = nlohmann::json;
using namespace std::filesystem;

namespace conf {
    json loadConfig() {
        if ( exists(".mort") ) {
            json cfg = json::parse(readFile(".mort"));

            return cfg;
        } else if ( exists(".acmp") ) {
            json cfg = json::parse(readFile(".acmp"));

            return cfg;
        } else {
            json cfg = json::parse("{}");

            return cfg;
        }
    }
}