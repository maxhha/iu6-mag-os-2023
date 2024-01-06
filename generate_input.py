import random
import math

N = 100_000_000
m = min(int(math.log10(N) // 3), 3)

with open(f"sample{N // (1000 ** m)}{['', 'K', 'M', 'G'][m]}.txt", "w") as f:
    for _ in range(N):
        f.write(f"{random.randint(-100000, 100000)}\n")
