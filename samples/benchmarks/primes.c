// primes.c
// Implements a very naive prime calculation algorithm

#include <stdio.h>
#include <stdbool.h>

bool is_prime(unsigned int candidate);

int main(int argc, char* argv[]) {
    // calculate the first 1000 primes
    unsigned int i = 1;
    unsigned int count = 0;
    while (i < 1000) {
        if (is_prime(i)) {
            printf("%d, ", i);
        }
        
        i += 1;
    }
    printf("\n");
    printf("Found %d primes\n", count);

    return 0;
}

bool is_prime(unsigned int candidate) {
    // is_prime
    // determines whether the candidate is prime

    bool prime = true;  // change to false once we prove it's not prime
    if (candidate < 2) {
        prime = false;
    }
    else if (candidate == 2) {
        prime = true;
    }
    else {
        unsigned int i = 2;
        bool found_factor = false;
        while (i < candidate && !found_factor) {    // todo: use 
            if (candidate % i == 0) {
                found_factor = true;
                prime = false;
            }
            else {
                i++;
            }
        }
    }

    return prime;
}
