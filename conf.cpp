#include "conf.hpp"

using namespace util;

//using json = nlohmann::json;
using namespace std::filesystem;

namespace conf {
    toml::table loadConfig() {
        if ( exists(".mort") ) {
            try {
                toml::table tmltab = toml::parse(".mort");//readFile(".mort"));
                //std::string strconf = toml::json_formatter(tmltab);
                return tmltab;//json::parse(strconf);
            } catch (...) {
                std::cout << "Failed to parse config file, not valid TOML" << std::endl;
            }
        } else if ( exists(".acmp") ) {
            try {
                toml::table tmltab = toml::parse(".acmp");//readFile(".acmp"));
                //std::string strconf = toml::json_formatter(tmltab);
                return tmltab;//json::parse(strconf);
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
}