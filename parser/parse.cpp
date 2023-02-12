#include "parse.hpp"
#include "../util.hpp"
#include <regex>
#include <sstream>
#include <iostream>
#include "../changed.hpp"
#include <set>

int NUMCHANGED;
extern bool DEBUG;

std::regex variable_regex(R"re(\$([a-zA-Z0-9_]+))re", std::regex_constants::optimize);
//std::regex indentation_regex(R"re(^\s*)re");
std::regex section_regex(R"re(^\s*(\w+):\s*((?:[^\s]+)?(?:\s+[^\s]+)*?)((?:\s+,\w+(?:=[\w\.\-\/]+)?)?(?:\s+,\w+(?:=[\w\.\-\/]+)?)*))re");
std::regex main_section_regex(R"re(^(\s*)(\w+):\s*((?:[\w\*\.\-/]+)?(?:\s+[\w\*\.\-/]+)*)((?:\s+,\w+(?:=[\w\.\-/]+)?)?(?:\s+,\w+(?:=[\w\.\-/]+)?)*))re");
std::regex assignment_regex(R"re(^\s*(\w+)\s*=\s*"(.*)"$)re");
std::regex comment_regex(R"re(\s*#.*$)re");
std::regex object_regex(R"re(.*objects$)re");

std::string escapeQuotes(std::string content) {
  std::string ocontent = content;

  std::regex re1("\\\"");

  ocontent = std::regex_replace(ocontent, re1, "\\\"");

  std::regex re2("\"");

  ocontent = std::regex_replace(ocontent, re2, "\"");

  return ocontent;
}

int System(std::string command) {
  std::string processedcommand;

  processedcommand = escapeQuotes(command);

  /*for (auto &var : envvars) {
    processedcommand =
    escapeQuotes("export " + var.first + "=\"" + var.second + "\"; ") +
    processedcommand;
    }*/

  processedcommand = "bash -c \"" + processedcommand + "\"";

  if (DEBUG) {
    //CANPRINT.lock();
    std::cout << processedcommand << std::endl;
    //CANPRINT.unlock();
  }

  //return system(processedcommand.c_str());
  return 0;
}

EnvVal::EnvVal() {};
EnvVal::EnvVal(std::string value,
               std::map<std::string, EnvVal> *env) {
  this->value = value;
  this->env = env;
}

std::string EnvVal::resolveValue() {
  std::string tmp = this->value;

  std::sregex_iterator rit ( tmp.begin(), tmp.end(), variable_regex );
  std::sregex_iterator rend;

  #warning This only replaces the first variable found.

  int found = 0;
  
  while (rit != rend) {
    std::string key = rit->str(1);
    std::cout << "Position: " << rit->position() << std::endl
              << "Length: " << rit->length() << std::endl
              << "before: \n" << tmp << std::endl;
    
    if (!this->env->count(key)) {
      throw std::runtime_error("\n    (In configuration file) Variable $"
                               + key + " is not defined.");
                               }
    
    tmp.replace(rit->position(),
                rit->length(),
                this->env->at(key).resolveValue());

    std::cout << "after: \n" << tmp << std::endl;

    ++rit;
    found++;
  }

  if (found) {
    std::cout << found << std::endl;
  }

  return tmp;
}

Step::Step() {};
Step::Step(std::string process,
           std::string type,
           std::vector<std::string> flags,
           std::vector<std::string> regexes,
           std::map<std::string, EnvVal> *env) {
  this->type = type;
  this->flags = flags;
  this->regexes = regexes;
  this->env = env;

  this->commands = {};

  for (auto& command : util::split(process, '\n')) {
    this->commands.push_back(EnvVal(command, this->env));
  }
}

int Step::runCommands() {
  int status = 0;

  for (auto& command : this->commands) {
    status = status || System(command.resolveValue());
  }

  return status;
};

bool Step::hasflag(std::string flag) {
  for (auto& sflag : this->flags) {
    if (flag == sflag) {
      return true;
    }
  }

  return false;
}

int Step::runObject(bool all, std::vector<std::string> filenames) {
  int status = 0;
  
  if (all) {
    std::vector<std::string> ofilenames = {};

    for (auto& file : filenames) {
      ofilenames.push_back(this->getObject(file));
    }
    
    this->env->insert_or_assign("OBJECT_FILES",
                                EnvVal(util::join(util::wrap(ofilenames)),
                                       this->env));
    status = this->runCommands();
    this->env->erase("OBJECT_FILES");
  } else {
    for (auto& file : filenames) {
      this->env->insert_or_assign("OBJECT_FILE",
                                  EnvVal(this->getObject(file), this->env));
      status = status || this->runCommands();
      this->env->erase("OBJECT_FILE");
    }
  }

  return status;
}

int Step::run(std::vector<std::string> unfiltered_filenames) {
  std::vector<std::string> exclude = {".*"};
  std::vector<std::string> filenames = util::includeExclude(this->regexes,
                                                            exclude,
                                                            unfiltered_filenames);
  
  int status = 0;
  bool all = false;
  
  if (this->type != "build") {
    all = true;
  }
  
  if (this->hasflag(",per")) {
    all = false;
  } else if (this->hasflag(",all")) {
    all = true;
  }

  std::smatch m;
  if (std::regex_match(this->type, m, object_regex)) {
    return this->runObject(all, filenames);
  }

  if (all) {
    this->env->insert_or_assign("FILES",
                                EnvVal(util::join(util::wrap(this->changedFiles(filenames))), this->env));
    status = this->runCommands();
    this->env->erase("FILES");
  } else {
    for (auto& file : filenames) {
      std::cout << file << " changed? " << changed::fileChanged(file) << std::endl;
      if (changed::fileChanged(file)) {
        this->env->insert_or_assign("FILE", EnvVal("\""+ file +"\"", this->env));
        this->env->insert_or_assign("OBJECT_FILE",
                                    EnvVal(this->getObject(file), this->env));
        status = status || this->runCommands();
        this->env->erase("FILE");
        this->env->erase("OBJECT_FILE");
      }
    }
  }

  return status;
}

std::regex object_split_regex(R"re(,object=(\S))re");
std::regex object_replacement_regex(R"re(\$FILE)re");

std::string Step::getObject(std::string filename) {
  std::smatch m;
  for (auto& flag : this->flags) {
    if (std::regex_match(flag, m, object_split_regex)) {
      std::regex_replace(m.str(1), object_replacement_regex, filename);
    }
  }

  return "./build/" + filename + ".o";
}

std::vector<std::string> Step::changedFiles(std::vector<std::string> files) {
  std::vector<std::string> exclude = {".*"};
  std::vector<std::string> filtered_files = util::includeExclude(this->regexes,
                                                         exclude,
                                                         files);

  std::vector<std::string> changed_files = {};

  for (auto& file : filtered_files) {
    if (changed::fileChanged(file)) {
      changed_files.push_back(file);
    }
  }

  return changed_files;
}

Target::Target() {};
Target::Target(std::string process,
       std::vector<std::string> inherits,
       std::map<std::string, Target> *targets,
       std::map<std::string, EnvVal> *env) {
  this->targets = targets;
  this->env = env;
  this->inherits = inherits;

  std::stringstream buffer;
  bool insection = false;
  std::string type;
  std::vector<std::string> regexes;
  std::vector<std::string> flags;
  
  for (auto& raw_line : util::split(process, '\n')) {
    std::string line = std::regex_replace(raw_line, comment_regex, "");
    std::smatch m;
    
    if (std::regex_match(line, m, section_regex)) {
      if (buffer.str().size()) {
        this->steps[type].push_back(Step(std::string(buffer.str()),
                                         type,
                                         flags,
                                         regexes,
                                         this->env));
      }
      
      type = m.str(1);
      regexes = util::split(m.str(2), ' ');
      flags = util::split(m.str(3), ' ');
      insection = true;

      buffer = std::stringstream("");
    } else if (std::regex_match(line, m, assignment_regex)) {
      std::string variable_name = m.str(1);
      std::string variable_value = m.str(2);
      this->delayed_env.insert_or_assign(variable_name,
                                  EnvVal(variable_value, this->env));
    } else if (insection) {
      buffer << line << std::endl;
    } else {
      if (util::strip(line) != "") {
        throw std::runtime_error("\n    Invalid syntax (Target): " + raw_line);
      }
    }
  }

  if (insection) {
    if (buffer.str().size()) {
      this->steps[type].push_back(Step(std::string(buffer.str()),
                                       type,
                                       flags,
                                       regexes,
                                       this->env));
    }
  }
}

std::map<std::string, std::vector<Step>> Target::getSteps() {
  std::map<std::string, std::vector<Step>> all_steps = this->steps;

  for (auto& parent : this->inherits) {
    for (auto& keypair : this->targets->at(parent).getSteps()) {
      for (auto& step : keypair.second) {
        if (!all_steps.count(keypair.first)) {
          all_steps[keypair.first] = {};
        }
        
        all_steps[keypair.first].push_back(step);
      }
    }
  }

  return all_steps;
}

int Target::runStep(std::string type, std::vector<std::string> filenames) {
  int status = 0;
  std::map<std::string, std::vector<Step>> all_steps = this->getSteps();
  
  if (!all_steps.count(type)) {
    return 0;
  }

  for (auto& step : all_steps[type]) {
    status = status || step.run(filenames);
  }

  return status;
}

int Target::processFiles(std::vector<std::string> filenames) {
  int status = 0;

  for (auto& var : this->delayed_env) {
    this->env->insert_or_assign(var.first, var.second);
  }

  if (this->env->count("EXECUTABLE")) {
    outname = this->env->at("EXECUTABLE").resolveValue(); 
  }
  
  status = status || this->runStep(std::string("before"), filenames);
  
  status = status || this->runStep(std::string("before_build"), filenames);
  status = status || this->runStep(std::string("build"), filenames);
  status = status || this->runStep(std::string("after_build"), filenames);

  status = status || this->runStep(std::string("before_objects"), filenames);
  status = status || this->runStep(std::string("objects"), filenames);
  status = status || this->runStep(std::string("after_objects"), filenames);

  status = status || this->runStep(std::string("after"), filenames);

  return status;
}

std::vector<std::string> Target::changedFiles(std::vector<std::string> filenames) {
  if (this->delayed_env.count("EXECUTABLE")) {
    outname = this->delayed_env.at("EXECUTABLE").resolveValue(); 
  }

  std::set<std::string> files = {};

  if (!this->steps.count("build")) {
    return {};
  }
  
  for (auto& step : this->steps["build"]) {
    for (auto& file : step.changedFiles(filenames)) {
      files.insert(file);
    }
  }

  return std::vector<std::string>(files.begin(), files.end());
}

Config::Config() {};
Config::Config(std::string filename) {
  std::stringstream buffer;
  bool insection = false;
  std::string name;
  std::vector<std::string> inherits;
  for (auto& raw_line : util::split(util::readFile(filename), '\n')) {
    std::string line = std::regex_replace(raw_line, comment_regex, "");
    std::smatch m;
    
    if (std::regex_match(line, m, main_section_regex)) {
      if (!m.str(1).size()) {
        if (buffer.str().size()) {
          this->targets[name] = Target(std::string(buffer.str()),
                                       inherits,
                                       &this->targets,
                                       &this->env);
        }
      
        name = m.str(2);
        inherits = util::split(m.str(3), ' ');
        insection = true;

        buffer = std::stringstream("");
        continue;
      }

      buffer << line << std::endl;
    } else if (insection) {
      buffer << line << std::endl;
    } else if (std::regex_match(line, m, assignment_regex)) {
      std::string variable_name = m.str(1);
      std::string variable_value = m.str(2);
      this->env[variable_name] = EnvVal(variable_value, &this->env);
    } else {
      if (util::strip(line) != "") {
        throw std::runtime_error("\n    Invalid syntax (Config): " + raw_line);
      }
    }
  }

  if (insection) {
    if (buffer.str().size()) {
      this->targets[name] = Target(std::string(buffer.str()),
                                   inherits,
                                   &this->targets,
                                   &this->env);
    }
  }
}

int Config::processTarget(std::string target) {
  if (!this->targets.count(target)) {
    throw std::runtime_error("Target: " + target + " does not exist.");
  }

  if (this->env.count("EXECUTABLE")) {
    outname = this->env["EXECUTABLE"].resolveValue(); 
  }

  NUMCHANGED = this->changedFiles(target).size();

  return this->targets[target].processFiles(util::getFiles());
}

std::vector<std::string> Config::changedFiles(std::string target) {
  return this->targets[target].changedFiles(util::getFiles());
}
