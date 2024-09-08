import platform
import subprocess
import sys


def get_symbols(binary):
    symbols = set()
    args = ["nm"]
    if platform.system() == "Darwin":
        args += ["-g", "-U", binary]
    elif platform.system() == "Linux":
        args += ["-g", "--defined-only", binary]
    else:
        raise Exception("Unsupported platform %s" % platform.system())
    res = subprocess.run(args, stdout=subprocess.PIPE)
    for line in res.stdout.decode("utf-8").split("\n"):
        # skip over empty lines and lines that are not symbols
        if line.strip() and "for architecture" not in line:
            symbols.add(line.split(" ")[2])
    return symbols



if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: %s <binary1> <binary2>" % sys.argv[0])
        sys.exit(1)
    binary1, binary2 = sys.argv[1:]
    symbols1 = get_symbols(binary1)
    symbols2 = get_symbols(binary2)
    intersection = symbols1 & symbols2
    if symbols1 & symbols2:
        sys.stderr.write("Conflicting symbols with `postgres`:\n  ")
        sys.stderr.write("\n  ".join(intersection))
        sys.stderr.write("\n")
        sys.exit(1)
