import random
import math
import sys
import os

N = int(sys.argv[1].replace("_", "") if len(sys.argv) >= 2 else 100_000_000)
m = min(int(math.log10(N) // 3), 3)
mm = (1000 ** m)
filename = f"data/sample{N // mm}{['', 'K', 'M', 'G'][m]}.txt"

if not os.path.exists("data"):
    os.mkdir("data")

with open(filename, "w") as f:
    for i in range(N):
        f.write(f"{random.randint(-100000, 100000)}\n")
        if i % (mm * 10)  == 0:
            print(".", end="", flush=True);

print("\nSaved in", filename)
