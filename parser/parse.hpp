#pragma once

//
// parse.cpp
//  Kazani 2023
//
// Parse .mort syntax
//

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "env.hpp"

class Step {
public:
  Step();
  Step(std::string process,
       std::string type,
       std::vector<std::string> flags,
       std::vector<std::string> regexes,
       std::shared_ptr<Env> env);

  int run(std::vector<std::string> filenames);
  std::vector<std::string> changedFiles(std::vector<std::string> filenames);
  std::vector<std::string> getRegexes();
  std::string getObject(std::string filename);
private:
  std::vector<std::string> regexes;
  std::string type;
  std::vector<std::string> flags;
  std::vector<EnvVal> commands;
  bool hasflag(std::string sflag);
  int runObject(bool all, std::vector<std::string> filenames);
  int runCommands();

  std::shared_ptr<Env> env;
};

class Target {
public:
  Target();
  Target(std::string process,
         std::vector<std::string> inherits,
         std::shared_ptr<std::map<std::string, Target>> targets,
         std::shared_ptr<Env> env);
  
  std::map<std::string, std::vector<Step>> getSteps();

  int runStep(std::string type,
                      std::vector<std::string> filenames);
  std::vector<std::string> changedFiles(std::vector<std::string> filenames);
  int processFiles();
private:
  std::map<std::string, std::vector<Step>> steps;
  std::shared_ptr<Env> env;
  Env delayed_env;
  std::shared_ptr<std::map<std::string, Target>> targets;
  std::vector<std::string> inherits;
};

class Config {
public:
  Config();
  Config(std::string filename);
  
  int processTarget(std::string target);
private:
  std::string file;
  std::shared_ptr<std::map<std::string, Target>> targets;
  std::shared_ptr<Env> env;

  std::vector<std::string> changedFiles(std::string target);
};
