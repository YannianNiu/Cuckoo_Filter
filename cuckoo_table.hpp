#include <iostream>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <type_traits>
#include <exception>
#include <iomanip>

#include "bit_manager.hpp"


template<size_t entries_per_bucket, size_t bits_per_fp, typename fp_type>
class CuckooTable {

private:
    static const size_t bytes_per_bucket = (entries_per_bucket * bits_per_fp) / 8;
    // number of buckets
    size_t table_size;
    // mask for extracting lower bits
    uint32_t fp_mask;

    // manipulation with bits
    BitManager<fp_type> *bit_manager;

    struct Bucket {
        uint8_t data[bytes_per_bucket];
    };

    // element storage
    Bucket *buckets;

public:

    /**
     *
     * @tparam element_type Working element type
     * @tparam entries_per_bucket Number of entries in bucket
     * @tparam bits_per_fp  Number of bits in fingerprint
     * @tparam fp_type Fingerprint type
     * @param table_size Table size, total number of buckets
     * @param fp_mask Fingerprint mask from filter
     */
    CuckooTable(size_t table_size, uint32_t fp_mask);

    /**
     * Deleting all entries from cuckoo table and deleting bit manager.
     */
    ~CuckooTable();

    /**
     * Returning table size, number of buckets
     *
     * @return number of buckets
     */
    size_t getTableSize();

    /**
     * Returning maximum number of elements stored in table.
     *
     * @return Maximum possible number of elements
     */
    size_t maxNoOfElements();

    /**
     *  Gets fingerprint in bucket i with entry position j
     *
     * @param i Index of bucket which stores element
     * @param j Index of entry in bucket
     * @return Fingerprint on position (i,j)
     */
    uint32_t getFingerprint(size_t i, size_t j);

    /**
     * Count of stored fingerprints in bucket.
     *
     * @param i Bucket index
     * @return Number of fingerprints stored in one bucket
     */
    size_t fingerprintCount(size_t i);

    /**
     * Gets number of free entries from table, i.e entries which stores value 0.
     *
     * @return Number of enetries with value 0.
     */
    size_t getNumOfFreeEntries();

    /**
     * Prints table on standard output. Every bucket is presented with hexadecimal value
     */
    void printTable();

    /**
     * Inserting fingerprint in bucket with index i and entry index j
     *
     * @param i  Index of bucket
     * @param j  Entry index
     * @param fp Fingerprint for insertion
     */
    void insertFingerprint(size_t i, size_t j, uint32_t fp);

    /**
     * Method for inserting element in table. If another element is being replaced, his fingerprint is stored.
     *
     * @param i Bucket index
     * @param fp Fingerprint for storing
     * @param eject True if element is being ejected from table
     * @param prev_fp Fingerprint of element being replaced
     * @return True if element is inserted without replacing
     */
    bool replacingFingerprintInsertion(size_t i, uint32_t fp, bool eject, uint32_t &prev_fp);

    /**
     * Checking if bucket i contains fingerprint fp
     *
     * @param i Bucket index
     * @param fp Fingerprint for checking
     * @return  True if fingerprint is contained
     */
    bool containsFingerprint(size_t i, uint32_t fp);

    /**
     * Checking if fingerprint is contained in bucket with index i1 and i2.
     *
     * @param i1 First checking index
     * @param i2 Second checking index
     * @param fp Fingerprint to check
     * @return True if element is contained
     */
    bool containsFingerprint(size_t i1, size_t i2, uint32_t fp);

    /**
     * Deleting fingerprint from table. If fingerprint is not presented in certain bucket, returning false.
     *
     * @param fp  Fingerprint for deletion
     * @param i Index of bucket where fingerprint is stored.
     * @return True if element is deleted
     */
    bool deleteFingerprint(uint32_t fp, size_t i);
};


template<size_t entries_per_bucket, size_t bits_per_fp, typename fp_type>
CuckooTable<entries_per_bucket, bits_per_fp, fp_type>::CuckooTable(const size_t table_size, uint32_t fp_mask) {
    this->table_size = table_size;
    this->fp_mask = fp_mask;

    buckets = new Bucket[table_size];
    memset(buckets, 0, bytes_per_bucket * table_size);

    if (entries_per_bucket == 4 && bits_per_fp == 4 && std::is_same<fp_type, uint8_t>::value) {
        bit_manager = new BitManager4<fp_type>();
    } else if (entries_per_bucket == 4 && bits_per_fp == 8 && std::is_same<fp_type, uint8_t>::value) {
        bit_manager = new BitManager8<fp_type>();
    } else if (entries_per_bucket == 4 && bits_per_fp == 12 && std::is_same<fp_type, uint16_t>::value) {
        bit_manager = new BitManager12<fp_type>();
    } else if (entries_per_bucket == 4 && bits_per_fp == 16 && std::is_same<fp_type, uint16_t>::value) {
        bit_manager = new BitManager16<fp_type>();
    } else if (entries_per_bucket == 2 && bits_per_fp == 32 && std::is_same<fp_type, uint32_t>::value) {
        bit_manager = new BitManager32<fp_type>();
    } else {
        throw std::runtime_error("Invalid parameters.\n"
                                 "Supported parameter values for (entries_per_bucket, bits_per_fp, fp_type):\n"
                                 "{(4,  4, uint8_t),\n"
                                 " (4,  8, uint8_t),\n"
                                 " (4, 12, uint16_t),\n"
                                 " (4, 16, uint16_t),\n"
                                 " (2, 32, uint32_t)}\n");
    }

}


template<size_t entries_per_bucket, size_t bits_per_fp, typename fp_type>
CuckooTable<entries_per_bucket, bits_per_fp, fp_type>::~CuckooTable() {
    delete[] buckets;
    delete bit_manager;
}


template<size_t entries_per_bucket, size_t bits_per_fp, typename fp_type>
size_t CuckooTable<entries_per_bucket, bits_per_fp, fp_type>::getTableSize() {
    return table_size;
}


template<size_t entries_per_bucket, size_t bits_per_fp, typename fp_type>
size_t CuckooTable<entries_per_bucket, bits_per_fp, fp_type>::maxNoOfElements() {
    return entries_per_bucket * table_size;
}


template<size_t entries_per_bucket, size_t bits_per_fp, typename fp_type>
inline uint32_t CuckooTable<entries_per_bucket, bits_per_fp, fp_type>::
getFingerprint(const size_t i, const size_t j) {
    const uint8_t *bucket = buckets[i].data;
    uint32_t fp = bit_manager->read(j, bucket);
    return fp & fp_mask;
}


template<size_t entries_per_bucket, size_t bits_per_fp, typename fp_type>
size_t CuckooTable<entries_per_bucket, bits_per_fp, fp_type>::
fingerprintCount(const size_t i) {
    size_t count = 0;
    for (size_t j = 0; j < entries_per_bucket; j++) {
        if (getFingerprint(i, j) != 0) {
            count++;
        }
    }
    return count;
}


template<size_t entries_per_bucket, size_t bits_per_fp, typename fp_type>
void CuckooTable<entries_per_bucket, bits_per_fp, fp_type>::
insertFingerprint(const size_t i, const size_t j, const uint32_t fp) {
    const uint8_t *bucket = buckets[i].data;
    uint32_t efp = fp & fp_mask;
    bit_manager->write(j, bucket, efp);
}


template<size_t entries_per_bucket, size_t bits_per_fp, typename fp_type>
inline bool
CuckooTable<entries_per_bucket, bits_per_fp, fp_type>::replacingFingerprintInsertion(const size_t i, const uint32_t fp,
                                                                                     const bool eject,
                                                                                     uint32_t &prev_fp) {
    for (size_t j = 0; j < entries_per_bucket; j++) {
        if (getFingerprint(i, j) == 0) {
            insertFingerprint(i, j, fp);
            return true;
        }
    }

    if (eject) {
        size_t next = rand() % entries_per_bucket;
        prev_fp = getFingerprint(i, next);
        insertFingerprint(i, next, fp);
    }
    return false;
}


template<size_t entries_per_bucket, size_t bits_per_fp, typename fp_type>
bool CuckooTable<entries_per_bucket, bits_per_fp, fp_type>::containsFingerprint(const size_t i, const uint32_t fp) {
    const uint8_t *bucket = buckets[i].data;
    uint64_t val = *((uint64_t *) bucket);

    return bit_manager->hasvalue(val, fp);
}


template<size_t entries_per_bucket, size_t bits_per_fp, typename fp_type>
bool CuckooTable<entries_per_bucket, bits_per_fp, fp_type>::containsFingerprint(const size_t i1, const size_t i2,
                                                                                const uint32_t fp) {
    const uint8_t *b1 = buckets[i1].data;
    const uint8_t *b2 = buckets[i2].data;

    uint64_t val1 = *((uint64_t *) b1);
    uint64_t val2 = *((uint64_t *) b2);

    return bit_manager->hasvalue(val1, fp) || bit_manager->hasvalue(val2, fp);
}


template<size_t entries_per_bucket, size_t bits_per_fp, typename fp_type>
bool CuckooTable<entries_per_bucket, bits_per_fp, fp_type>::deleteFingerprint(const uint32_t fp, const size_t i) {
    for (size_t j = 0; j < entries_per_bucket; j++) {
        if (getFingerprint(i, j) == fp) {
            insertFingerprint(i, j, 0);
            return true;
        }
    }
    return false;
}


template<size_t entries_per_bucket, size_t bits_per_fp, typename fp_type>
size_t CuckooTable<entries_per_bucket, bits_per_fp, fp_type>::
getNumOfFreeEntries() {
    size_t free = 0;
    for (size_t i = 0; i < table_size; ++i) {
        for (size_t j = 0; j < entries_per_bucket; ++j) {
            if (getFingerprint(i, j) == 0) {
                free++;
            }
        }
    }
    return free;
}


template<size_t entries_per_bucket, size_t bits_per_fp, typename fp_type>
void CuckooTable<entries_per_bucket, bits_per_fp, fp_type>::printTable() {
    for (int i = 0; i < table_size; ++i) {
        std::cout << i << " | ";
        for (int j = 0; j < entries_per_bucket; ++j) {
            auto bucket = buckets[i].data;
            uint32_t fp = bit_manager->read(j, bucket);
            std::cout << std::setfill('0') << std::setw(8) << std::hex << fp << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::dec;
}