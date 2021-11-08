import os, sys

def main():
    files = []
    for root, file, dir in os.walk("."):
        if file.split(".")[-1].lower() in [".cpp", ".c"]:
            files.append(f"{root}/{file}".replace("//", "/"))

    filestr = " ".join( [ f"\"{i}\"" for i in files ] )

    cppargs = " ".join(sys.argv[1:])
    os.system(f'c++ {filestr} {cppargs}')

if __name__ == "__main__":
    main()