#include <algorithm>
#include <cstring>
#include <stdexcept>

template <class Tdata, class Tkey, unsigned entries_in_bucket>
class transposition_table_c
{
    static_assert(entries_in_bucket <= 256, "Too much entries in one bucket");

protected:

    Tdata *table = nullptr;
    size_t buckets = 0;
    Tkey key_mask = 0;
    size_t _size = 0;
    unsigned char *occupancy = nullptr;

public:

    transposition_table_c() = delete;

    transposition_table_c(const transposition_table_c<Tdata, Tkey,
                          entries_in_bucket> &t) = delete;

    transposition_table_c<Tdata, Tkey,
        entries_in_bucket> operator = (const transposition_table_c<Tdata, Tkey,
                entries_in_bucket> &x) = delete;

    transposition_table_c(const size_t entries)
    {
        set_size(entries);
    }

    ~transposition_table_c()
    {
        delete[] table;
        delete[] occupancy;
    }

    Tdata* find(const Tdata &entry_data)
    {
        return FindEntryAndRotate(entry_data, entry_data.hash());
    }

    Tdata* find(const Tdata &entry_data, const Tkey key)
    {
        return FindEntryAndRotate(entry_data, key);
    }

    void add(const Tdata &entry_data)
    {
        add(entry_data, entry_data.hash());
    }

    void add(const Tdata &entry_data, const Tkey key)
    {
        Tdata* entry_ptr = FindEntryAndRotate(entry_data, key);
        if(entry_ptr != nullptr)
        {
            *entry_ptr = entry_data;
            return;
        }
        const Tkey bucket_num = (key & key_mask);
        const auto bucket_ptr = &table[bucket_num*entries_in_bucket];
        if(occupancy[bucket_num] < entries_in_bucket)
        {
            _size++;
            occupancy[bucket_num]++;
        }
        std::rotate(&bucket_ptr[0], &bucket_ptr[entries_in_bucket - 1],
                    &bucket_ptr[entries_in_bucket]);
        bucket_ptr[0] = entry_data;
    }

    void set_size(const size_t entries)
    {
        _size = 0;
        buckets = 0;
        key_mask = 0;
        unsigned sz = entries/entries_in_bucket;
        unsigned MSB_count = 0;
        while(sz >>= 1)
            MSB_count++;
        sz = static_cast<unsigned>(1 << MSB_count);

        if((table = new Tdata[entries_in_bucket*sz]) == nullptr ||
                (occupancy = new unsigned char[sz]) == nullptr)
            throw std::invalid_argument("Out of memory");

        buckets = sz;
        key_mask = static_cast<Tkey>(sz - 1);
        clear();
    }

    void clear()
    {
        _size = 0;
        std::fill_n(occupancy, buckets, 0);
    }

    size_t size() const
    {
        return _size;
    }

    size_t max_size() const
    {
        return buckets*entries_in_bucket;
    }

    void resize(size_t entries)
    {
        delete[] table;
        delete[] occupancy;
        set_size(entries);
    }

    Tdata* FindEntryAndRotate(const Tdata &entry, Tkey hash)
    {
        const unsigned bucket_num = hash & key_mask;
        auto * const ptr = &table[entries_in_bucket*bucket_num];
        for(size_t i = 0; i < occupancy[bucket_num]; ++i)
            if(ptr[i] == entry)
            {
                std::rotate(&ptr[0], &ptr[i], &ptr[i + 1]);
                return ptr;
            }
        return nullptr;
    }
};
