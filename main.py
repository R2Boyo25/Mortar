import os, sys

def main():
    ofiles = []
    for root, dirs, files in os.walk("./"):
        for file in files:
            if file.split(".")[-1].lower() in ["cpp", "c"]:
                ofiles.append(f"{root}/{file}".replace("//", "/"))

    filestr = " ".join( [ f"\"{i}\"" for i in ofiles ] )

    cppargs = " ".join(sys.argv[1:])
    print(f'c++ {filestr} {cppargs}')
    os.system(f'c++ {filestr} {cppargs}')

if __name__ == "__main__":
    main()