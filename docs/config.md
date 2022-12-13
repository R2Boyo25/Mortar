# mortar.toml
The [TOML](https://toml.io/en/) file containing the configuration should be named `mortar.toml`. (If you see any of my projects using `.mort`, that was what I used to call it, and it still works) (I switched to `mortar.toml` so syntax highlighting would work out-of-the-box without custom associations)

# Commands
Anything that is defined as a command is run using bash.
YOU CAN USE [BASH FUNCTIONS](https://linuxize.com/post/bash-functions/).

# Targets
Targets are [TOML](https://toml.io/en/) tables.
Targets contain the information to compile a program.
Any target may be selected and run, except for `deps`. (which isn't a target)
```toml
key = "value"
keybool = true
keylist = ["value", "value"]
```

## _default
Mortar will run the target named `_default` if no target is specified at the command line.

### com
`com` is the compilation command. (`g++` by default, can be overidden with the `-c` flag))

### exclude
`exclude` is a list of regexes for files to exclude from compilation. (default is empty)

### include
`include` is a list of regexes for files to manually include in compilation that have been excluded by `exclude`. (default is empty)

You may exclude everything except the include files by setting `exclude` to `[".*"]`.

### relink
`relink` is a list of regexes for files that trigger a relinking of the project. (ex: if you're linking Rust staticlibs or something - `.*\\.rs`)

### oarg
`oarg` is a list of arguments to be passed to `com`.

### l
`l` is a list of library names to be passed to `com`. (with `-l` prepended)

### out
`out` is the output binary's path relative to the current working directory of Mortar. (`a.out` by default)

### threads
`threads` is the number of threads to use during compilation. (max CPU threads by default, can be overidden with the `-j` flag)

### compileHeaders
`compileHeaders` is whether or not to compile headers to gch files. (I haven't noticed any performance boosts in small programs.)

### before
`before` is the command to run before compilation.

### beforeoptional
`beforeoptional` is whether or not the `before` command is required to succeed to continue compilation. (`false` by default)

### after
`after` is the command to run after compilation.

# Inheritance
`inherits` may be set to the name of a target to inherit values from that target.

Lists are combined with the inherited valued. (uses inherited value if inheriting target's value is undefined)

Non-lists are replaced by the value of the inheriting target. (uses inherited value if inheriting target's value is undefined)

# Env
`env` is a [subtable]() of any target than containes environment variables.
```toml
[target.env]
varname="value"
varname2 = "value2"
```

# Runners
A target may set the `type` key to `command` to become a runner.
The key `target` must be defined and set to a valid target.
The key `command` must be set to a command to run.
NO OTHER KEYS ARE USED FROM THE RUNNER - DEFINE SETTINGS IN THE TARGET.

# Deps
`deps` is a list of tables containing git dependencies.
- git is the only supported download method.

## Adding a dependency
You can add a dependency with the following [TOML](https://toml.io/en/):
```toml
[[deps]]
url = "https://example.com/path/to/repository.git"
ipath = "pathtofolder"
cpath = "pathtofolder"
exclude = ["path1", "file1", "path/tofile"]
```

### url
`url` is the link to the git repository of the dependency.

### cpath
`cpath` is the path to copy the files from under the repository. (if you only want part of the repository) (use `.` for the entire repository) (`.` by default)

### ipath
`ipath` is the path to put `cpath` under `include`. (so you can put a dependency in its own folder) (`include/` is prepended to this value)

### exclude
`exclude` is a list of regexes for paths to exclude from being moved into `ipath`. (if you don't want certain files) (useful for excluding a library's test cpp files, etc.) 

### include
`include` is a list of regexes for path to include that have been excluded by `exclude`. (basically the same as a target's `include` & `exclude`, functionally)

### exclude_from_compilation
`exclude_from_compilation` tells mortar whether to exclude the files in this dependency from compilation. (`false` by default) (should set to `true` if using `cmake` + `make`) [TODO]

### link_paths
`link_path` is a regex for files to link (statically) into the output executable. (empty by default)

### configure
`configure` is a command to run to configure the dependency. (empty by default) (internally `cd $cpath; $configure $ipath`) (need to copy files manually if you use this.) [TODO]

### setup
`setup` is the command to run to setup the dependency. (empty by default) (internally `cd $ipath; $setup`) [TODO]

### cmake
`cmake` tells mortar if the repository uses cmake. (`false` by default) (`configure` & `setup` default to `cmake` & `make`, respectively.) [TODO]

# Example
[Mortar's mortar.toml file](/mortar.toml)
