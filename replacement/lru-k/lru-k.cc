#include <algorithm>
#include <iterator>
#include <vector>
#include <unordered_map>

#include "cache.h"
#include "util.h"

const int K = 5;
std::vector<std::vector<std::pair<BLOCK, uint32_t>>> lfu_k_queues;
std::vector<std::vector<std::pair<BLOCK, uint32_t>>> lru_queues;

void CACHE::initialize_replacement() {
  lfu_k_queues = std::vector<std::vector<std::pair<BLOCK, uint32_t>>>(NUM_SET);
  lru_queues = std::vector<std::vector<std::pair<BLOCK, uint32_t>>>(NUM_SET);
}

// find replacement victim
uint32_t CACHE::find_victim(
    uint32_t cpu, uint64_t instr_id,
    uint32_t set, const BLOCK* current_set,
    uint64_t ip, uint64_t full_addr, uint32_t type) {

  auto& lfu_k_queue = lfu_k_queues[set];
  if (!lfu_k_queue.empty()) {
    // std::cout << "V1-BEGIN1" << std::endl;
    uint32_t lru_index = 0;
    uint32_t lru_max = 0;
    for (auto i = 0; i < lfu_k_queue.size(); ++i) {
      const auto& [lfu_item, _] = lfu_k_queue[i];
      if (lfu_item.lru > lru_max) {
        lru_max = lfu_item.lru;
        lru_index = i;
      }
    }
    auto victim_block = lfu_k_queue[lru_index];
    std::swap(lfu_k_queue[lru_index], lfu_k_queue[lfu_k_queue.size()-1]);
    lfu_k_queue.pop_back();
    return victim_block.second;
  } else {
    auto& lru_queue = lru_queues[set];
    uint32_t lru_index = 0;
    uint32_t lru_max = 0;
    for (auto i = 0; i < lru_queue.size(); ++i) {
      auto& [lru_item, _] = lru_queue[i];
      if (lru_item.lru > lru_max) {
        lru_max = lru_item.lru;
        lru_index = i;
      }
    }
    // std::cout << lru_max << " " << lru_index << std::endl;
    auto victim_block = lru_queue[lru_index];
    std::swap(lru_queue[lru_index], lru_queue[lru_queue.size()-1]);
    lru_queue.pop_back();
    return victim_block.second;
  }
}

// called on every cache hit and cache fill
void CACHE::update_replacement_state(uint32_t cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type,
                                     uint8_t hit)
{
  if (hit && type == WRITEBACK){
    return;
  }

  auto& lfu_queue = lfu_k_queues[set];
  auto& lru_queue = lru_queues[set];

  auto curr = std::next(std::next(block.begin(), set * NUM_WAY), way);
  if (hit) {
    ++curr->lfu;
    if (curr->lfu == K) {
      lfu_queue.emplace_back(*curr, way);
      for (auto it = lru_queue.begin(); it != lru_queue.end(); ++it) {
        if (it->second == way) {
          lru_queue.erase(it);
          break;
        }
      }
    }
  } else {
    lru_queue.emplace_back(*curr, way);
    curr->lfu = 1;
  }
}

void CACHE::replacement_final_stats() {}
