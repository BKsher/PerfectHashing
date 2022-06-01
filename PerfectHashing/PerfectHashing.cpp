#include <bits/stdc++.h>
using namespace std;

unsigned int MurmurHash2(const void* key, int len, unsigned int seed);

template <typename VALUE>
class CPerfectHashTable {
public:

    typedef std::pair<std::string, VALUE> TKeyValuePair;
    typedef std::vector<TKeyValuePair> TKeyValueVector;
    struct SKeyValueVectorBucket {
        TKeyValueVector m_vector;
        size_t          m_bucketIndex;
    };
    typedef std::vector<struct SKeyValueVectorBucket> TKeyValueVectorBuckets;

    //building perfect hashing table
    void Calculate(const TKeyValueVector& data) {

        m_numItems = data.size();
        m_numBuckets = NumBucketsForSize(m_numItems);
        m_bucketMask = m_numBuckets - 1;
        m_salts.resize(m_numBuckets);
        m_data.resize(m_numItems);
        TKeyValueVectorBuckets buckets;
        buckets.resize(m_numBuckets);

        for (size_t i = 0; i < m_numBuckets; ++i)
            buckets[i].m_bucketIndex = i;

        for (const TKeyValuePair& pair : data) {
            size_t bucket = FirstHash(pair.first.c_str());
            buckets[bucket].m_vector.push_back(pair);
        }

        std::sort(
            buckets.begin(),
            buckets.end(),
            [](const SKeyValueVectorBucket& a, const SKeyValueVectorBucket& b) {
                return a.m_vector.size() > b.m_vector.size();
            }
        );

        std::vector<bool> slotsClaimed;
        slotsClaimed.resize(m_numItems);
        for (size_t bucketIndex = 0, bucketCount = buckets.size(); bucketIndex < bucketCount; ++bucketIndex) {
            if (buckets[bucketIndex].m_vector.size() == 0)
                break;
            FindSaltForBucket(buckets[bucketIndex], slotsClaimed);
        }
    }

    //get value by key
    VALUE* GetValue(const char* key) {
        size_t bucket = FirstHash(key);
        int salt = m_salts[bucket];
        size_t dataIndex;
        if (salt < 0)
            dataIndex = (size_t)((salt * -1) - 1);
        else
            dataIndex = MurmurHash2(key, strlen(key), (unsigned int)salt) % m_data.size();

        if (m_data[dataIndex].first.compare(key) == 0)
            return &m_data[dataIndex].second;
        return nullptr;
    }

private:

    //first hashing
    unsigned int FirstHash(const char* key) {
        return MurmurHash2(key, strlen(key), 435) & m_bucketMask;
    }

    //second hashing
    void FindSaltForBucket(const SKeyValueVectorBucket& bucket, std::vector<bool>& slotsClaimed) {
        if (bucket.m_vector.size() == 1) {
            for (size_t i = 0, c = slotsClaimed.size(); i < c; ++i)
            {
                if (!slotsClaimed[i])
                {
                    slotsClaimed[i] = true;
                    m_salts[bucket.m_bucketIndex] = (i + 1) * -1;
                    m_data[i] = bucket.m_vector[0];
                    return;
                }
            }
        }
        for (int salt = 0; ; ++salt) {
            assert(salt >= 0);
            std::vector<size_t> slotsThisBucket;
            bool success = std::all_of(
                bucket.m_vector.begin(),
                bucket.m_vector.end(),
                [this, &slotsThisBucket, salt, &slotsClaimed](const TKeyValuePair& keyValuePair) -> bool {
                    const char* key = keyValuePair.first.c_str();
                    unsigned int slotWanted = MurmurHash2(key, strlen(key), (unsigned int)salt) % m_numItems;
                    if (slotsClaimed[slotWanted])
                        return false;
                    if (std::find(slotsThisBucket.begin(), slotsThisBucket.end(), slotWanted) != slotsThisBucket.end())
                        return false;
                    slotsThisBucket.push_back(slotWanted);
                    return true;
                }
            );
            if (success)
            {
                m_salts[bucket.m_bucketIndex] = salt;
                for (size_t i = 0, c = bucket.m_vector.size(); i < c; ++i)
                {
                    m_data[slotsThisBucket[i]] = bucket.m_vector[i];
                    slotsClaimed[slotsThisBucket[i]] = true;
                }
                return;
            }
        }
    }

    static size_t NumBucketsForSize(size_t size) {
        if (!size)
            return 1;
        size_t ret = 1;
        size = size >> 1;
        while (size) {
            ret = ret << 1;
            size = size >> 1;
        }
        return ret;
    }
    std::vector<int> m_salts;
    std::vector<TKeyValuePair> m_data;
    size_t m_numItems;
    size_t m_numBuckets;
    size_t m_bucketMask;
};

//hashing function
unsigned int MurmurHash2(const void* key, int len, unsigned int seed)
{
    const unsigned int m = 0x5bd1e995;
    const int r = 24;
    unsigned int h = seed ^ len;
    const unsigned char* data = (const unsigned char*)key;
    while (len >= 4)
    {
        unsigned int k = *(unsigned int*)data;
        k *= m;
        k ^= k >> r;
        k *= m;
        h *= m;
        h ^= k;
        data += 4;
        len -= 4;
    }
    switch (len)
    {
    case 3: h ^= data[2] << 16;
    case 2: h ^= data[1] << 8;
    case 1: h ^= data[0];
        h *= m;
    };
    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
}

int main(int argc, char** argv)
{
    CPerfectHashTable<int> table;
    decltype(table)::TKeyValueVector data;
    ifstream file("words.txt");
    string str;
    int i = 0;
    while (std::getline(file, str) && i < 100000)
    {
        data.push_back(std::make_pair(str, i));
        ++i;
    }
    table.Calculate(data);

    for (decltype(table)::TKeyValuePair& keyValuePair : data) {
        int* value = table.GetValue(keyValuePair.first.c_str());
        assert(value != nullptr);
        if (value == nullptr)
        {
            printf("Error, could not find data for key \" % s\"n", keyValuePair.first.c_str());
            return 0;
        }
        assert(*value == keyValuePair.second);
        if (*value != keyValuePair.second)
        {
            printf("  table[\" % s\"] = %in", keyValuePair.first.c_str(), *value);
            printf("Incorrect value detected, should have gotten %i!n\n", keyValuePair.second);
            return 0;
        }
    }
    printf("Perfect hashing table corractly created!\n");
    return 0;
}
