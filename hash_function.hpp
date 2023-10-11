#ifndef CUCKOOFILTER_HASH_FUNCTION_H
#define CUCKOOFILTER_HASH_FUNCTION_H


#include <cstddef>
#include <cstdint>
#include <random>
#include <iostream>
#include <string>
#include <vector>
#include <tuple>

#define HASH_A 54059
#define HASH_B 76963
#define FIRSTH 37
static const uint32_t MURMUR_CONST = 0x5bd1e995;

// Martin Dietzfelbinger, "Universal hashing and k-wise independent random
// variables via integer arithmetic without primes".
class HashFunction {
private:
    //unsigned __int128 multiply_, add_;


public: 

    uint64_t hash(size_t key, size_t num_buckets);

//    uint64_t hash(uint32_t key) const;

};



/**
 * Hash function for string keys
 * @param key Key of string type
 * @return Hash function for string
 */
uint64_t HashFunction::hash(size_t key, size_t num_buckets){
//    return murmurHash3Function(&key, (size_t) key.size());
//    return std::hash<std::string>{}(key);
    unsigned long long key_val = std::hash<size_t>{}(key);
    uint32_t b = 1, j = 0;
    while (j < num_buckets) {
        b = j;
        key_val = key_val * 2862933555777941757ULL + 1; // Did you say magic number?
        j = (b + 1) * (double(1LL << 31) / double((key_val >> 33) + 1));
    }
    return b;
}

/**
 * Hash function for integer keys
 * @param key Key of integer type
 * @return Hash function for uint32
 */
//uint64_t HashFunction::hash(uint32_t key) const {
//    return (add_ + multiply_ * static_cast<decltype(multiply_)>(key)) >> 64;
//}


inline static uint32_t fingerprintComplement(const size_t index, const uint32_t fp) {
    return index ^ (fp * MURMUR_CONST);
}

#endif