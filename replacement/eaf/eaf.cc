#include <functional>
#include <vector>
#include <cmath>
#include <iostream>
#include <random>

#include "cache.h"
#include "util.h"

const double BLOOM_FILTER_ERROR = 0.008;
const double DENOM = 0.480453013918201;

// 64-bit hash for 64-bit platforms
uint64_t MurmurHash64A ( const void * key, int len, unsigned int seed )
{
  const uint64_t m = 0xc6a4a7935bd1e995;
  const int r = 47;

  uint64_t h = seed ^ (len * m);

  const uint64_t * data = (const uint64_t *)key;
  const uint64_t * end = data + (len/8);

  while(data != end)
  {
    uint64_t k = *data++;

    k *= m;
    k ^= k >> r;
    k *= m;

    h ^= k;
    h *= m;
  }

  const unsigned char * data2 = (const unsigned char*)data;

  switch(len & 7)
  {
  case 7: h ^= uint64_t(data2[6]) << 48;
  case 6: h ^= uint64_t(data2[5]) << 40;
  case 5: h ^= uint64_t(data2[4]) << 32;
  case 4: h ^= uint64_t(data2[3]) << 24;
  case 3: h ^= uint64_t(data2[2]) << 16;
  case 2: h ^= uint64_t(data2[1]) << 8;
  case 1: h ^= uint64_t(data2[0]);
    h *= m;
  };

  h ^= h >> r;
  h *= m;
  h ^= h >> r;

  return h;
}

class BloomFilter {
public:
  explicit BloomFilter(uint64_t _size = 50000) : BLOOM_FILTER_ENTRIES(_size), _counter(0) {
    BPE = -(log(BLOOM_FILTER_ERROR) / DENOM);
    BLOOM_FILTER_SIZE = _size;
    BLOOM_HASH_SIZE = static_cast<int>(ceil(0.693147180559945 * BPE));
    _container = std::vector<uint64_t>(BLOOM_FILTER_SIZE, 0);
  }

  void insert(uint64_t address) {
    const void* key = &address;

    uint64_t a = MurmurHash64A(key, sizeof(uint64_t), 0x9747b28c) % BLOOM_FILTER_SIZE;
    uint64_t b = MurmurHash64A(key, sizeof(uint64_t), a) % BLOOM_FILTER_SIZE;

    for (auto i = 0; i < BLOOM_HASH_SIZE; ++i) {
      auto x = (a + ((i % BLOOM_FILTER_SIZE) * b) % BLOOM_FILTER_SIZE) % BLOOM_FILTER_SIZE;
      if (_container[x] == 0) {
        _container[x] = 1;
        _counter++;
      }
    }
  }

  bool check(uint64_t address) {
    const void* key = &address;

    uint64_t a = MurmurHash64A(key, sizeof(uint64_t), 0x9747b28c) % BLOOM_FILTER_SIZE;
    uint64_t b = MurmurHash64A(key, sizeof(uint64_t), a) % BLOOM_FILTER_SIZE;

    uint64_t hit = 0;
    for (auto i = 0; i < BLOOM_HASH_SIZE; ++i) {
      auto x = (a + ((i % BLOOM_FILTER_SIZE) * b) % BLOOM_FILTER_SIZE) % BLOOM_FILTER_SIZE;
      if (_container[x] == 1) {
        hit++;
      }
    }

    return hit == BLOOM_HASH_SIZE;
  }

  void debug() const {
    std::cout << "BLOOM FILTER DEBUG:" << std::endl;
    std::cout << BLOOM_FILTER_SIZE << " " << BLOOM_HASH_SIZE << std::endl;
  }

  void clear() {
    _counter = 0;
    std::for_each(begin(_container), end(_container), [](uint64_t& x){
      x = 0;
    });
  }

  bool full() const {
    return _counter == BLOOM_FILTER_SIZE;
  }

private:
  double_t BPE;
  uint64_t BLOOM_FILTER_ENTRIES;
  uint64_t BLOOM_FILTER_SIZE;
  uint64_t BLOOM_HASH_SIZE;

  uint64_t _counter;
  std::vector<uint64_t> _container;
};

BloomFilter* blooms;
uint32_t lru_c = 0;
uint32_t mru_c = 0;
uint32_t in_c = 0;
uint32_t clear_c = 0;

void CACHE::initialize_replacement() {
  blooms = new BloomFilter(NUM_SET * NUM_WAY);
}

// find replacement victim
uint32_t CACHE::find_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK* current_set, uint64_t ip, uint64_t full_addr, uint32_t type) {
//  auto end = std::next(current_set, NUM_WAY);
//  uint32_t way = 0;
//  uint32_t victim_way = 0;
//  uint32_t max_lru = -1;
//  uint64_t victim_addr = 0;
//  auto b = current_set;
//  for (; way < NUM_WAY; ++way) {
//    if (b->valid && b->lru > max_lru && !blooms->check(b->address)) {
//      victim_way = way;
//      victim_addr = b->address;
//    }
//    b = std::next(b, 1);
//  }
//  blooms->insert(victim_addr);
//  return victim_way;
  auto victim_block = std::max_element(current_set, std::next(current_set, NUM_WAY), lru_comparator<BLOCK, BLOCK>());
  blooms->insert(victim_block->address);
  return std::distance(current_set, victim_block);
}

void insert_into_mru(std::vector<BLOCK>::iterator& begin, std::vector<BLOCK>::iterator& end, uint32_t way) {
  uint32_t hit_lru = std::next(begin, way)->lru;
  std::for_each(begin, end, [hit_lru](BLOCK& x) {
    if (x.lru <= hit_lru)
      x.lru++;
  });
  std::next(begin, way)->lru = 0; // promote to the MRU position
}

void insert_into_lru(std::vector<BLOCK>::iterator& begin, std::vector<BLOCK>::iterator& end, uint32_t way) {
  auto lru = std::max_element(begin, end, lru_comparator<BLOCK, BLOCK>())->lru;
  auto current_block = std::next(begin, way);
  current_block->lru = lru + 1;
}

// called on every cache hit and cache fill
void CACHE::update_replacement_state(uint32_t cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type,
                                     uint8_t hit) {
  if (hit && type == WRITEBACK)
    return;

  auto bf = blooms;
  auto begin = std::next(block.begin(), set * NUM_WAY);
  auto end = std::next(begin, NUM_WAY);

  if (hit) {
    insert_into_mru(begin, end, way);
  } else {
    if (!bf->check(full_addr)) {
      // Insert with bimodal policy, MRU with probability 1/64.
      std::random_device rd;
      std::mt19937 gen(rd());
      std::uniform_int_distribution<> uid(1, 64);
      auto pn = uid(gen);
      if (pn == 1) {
        // Present in EAF, insert with high priority. (MRU position).
        ++mru_c;
        insert_into_mru(begin, end, way);
      } else {
        ++lru_c;
        insert_into_lru(begin, end, way);
      }
    } else {
      // Present in EAF, insert with high priority. (MRU position).
      ++in_c;
      insert_into_mru(begin, end, way);
    }
  }

  if (bf->full()) {
    ++clear_c;
    bf->clear();
  }
}

void CACHE::replacement_final_stats() {
  std::cout << "STATISTICS FOR CURRENT CACHE:" << std::endl;
  blooms->debug();
  std::cout << NUM_SET << " " << NUM_WAY << std::endl;
  printf("LRU = %u, MRU = %u, IN = %u, CL = %u\n", lru_c, mru_c, in_c, clear_c);
  std::cout << "END OF STATISTICS" << std::endl;
}
