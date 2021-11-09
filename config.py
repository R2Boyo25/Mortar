import os, json

rootpath = f"/{'/'.join(os.path.abspath(__file__).split('/')[:-1])}/aconfig/"

def toDots(path):
    return ".".join(path.split("/"))

def genConfig():
    if not (os.path.exists(rootpath + toDots(os.getcwd()))) or not (os.path.exists('.acmp')):
        with open(rootpath + toDots(os.getcwd()), "w") as f:
            f.write("{\n\n}")

def loadConfig(path):
    with open(path, "r") as f:
        return json.loads(f.read())

def getConfig():
    genConfig()
    if os.path.exists(".acmp"):
        return loadConfig('.acmp')
    else:
        return loadConfig(rootpath + toDots(os.getcwd()))
    
def saveConfig(data):
    genConfig()
    if os.path.exists(".acmp"):
        with open('.acmp', 'w') as f:
            f.write(json.dumps(data, indent = 4))
    else:
        with open(rootpath + toDots(os.getcwd()), 'w') as f:
            f.write(json.dumps(data, indent = 4))

class config:
    def __init__(self):
        self.conf = getConfig()

    def __getitem__(self, item):
        return self.conf[item]
    
    def __setitem__(self, item, value):
        self.conf[item] = value
        _save()

    def _save(self):
        saveConfig(self.conf)
    
    def keys(self):
        return self.conf.keys()
    
