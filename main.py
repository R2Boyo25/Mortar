import os, sys

def main():
    files = []
    for root, files, dir in os.walk("."):
        for file in files:
            if file.split(".")[-1].lower() in [".cpp", ".c"]:
                files.append(f"{root}/{file}".replace("//", "/"))

    filestr = " ".join( [ f"\"{i}\"" for i in files ] )

    cppargs = " ".join(sys.argv[1:])
    print(f'c++ {filestr} {cppargs}')
    os.system(f'c++ {filestr} {cppargs}')

if __name__ == "__main__":
    main()