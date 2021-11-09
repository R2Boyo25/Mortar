import os, sys, config

cfg = config.config()

def comp(com, args):
    ofiles = []
    for root, dirs, files in os.walk("./"):
        for file in files:
            if file.split(".")[-1].lower() in ["cpp", "c"]:
                ofiles.append(f"{root}/{file}".replace("//", "/"))

    filestr = " ".join( [ f"\"{i}\"" for i in ofiles ] )

    cppargs = " ".join(args)
    print(f'{com} {filestr} {cppargs}')
    os.system(f'{com} {filestr} {cppargs}')

def compilecfg(name):
    if name in cfg.keys():
        com = cfg[name]['com'] if 'com' in cfg[name].keys() else 'c++'
        args = []
        if 'out' in cfg[name].keys():
            args.append('-o ' + cfg[name]['out'])
        if 'l' in cfg[name].keys():
            for i in [f"-l{i}" for i in cfg[name]['l']]:
                args.append(i)
        if 'oarg' in cfg[name].keys():
            for i in [oarg for oarg in cfg[name]['oarg']]:
                args.append(i)
        
        comp(cfg['com'], args)
    else:
        print('Target "' + name + '" not found.')
        quit()

def main():
    if len(sys.argv) == 1:
        compilecfg('_default')
    elif len(sys.argv) == 2:
        if sys.argv[1] == 'config':
            cfg = config.getConfig()
            cfg[sys.argv[2]] = {}
            cfg[sys.argv[2]][sys.argv[3]] = sys.argv[4:] if sys.argv[3] in ["l", "oarg"] else sys.argv[4]
            config.saveConfig(cfg)
        else:
            compilecfg(sys.argv[1])

if __name__ == "__main__":
    main()