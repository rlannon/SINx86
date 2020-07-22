# primes.py

def is_prime(candidate: int) -> bool:
    prime = True
    if candidate < 2:
        prime = False
    elif candidate > 2:
        i = 2
        while (i < candidate and prime):
            if candidate % i == 0:
                prime = False
            else:
                i += 1
    
    return prime

# main program
i = 1
count = 0

while i < 1000:
    if is_prime(i):
        count += 1
        print(i, ", ", sep="")
    i += 1

print()
print(f"Found {count} primes")
print("Done.")
