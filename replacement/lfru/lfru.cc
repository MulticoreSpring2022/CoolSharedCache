#include <algorithm>
#include <iterator>

#include "cache.h"
#include "util.h"

// initialize replacement state
void CACHE::initialize_replacement()
{
  // set the value to 0
  for (auto& blk : block)
    blk.lru = 0;
}

// find replacement victim
uint32_t CACHE::find_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK* current_set, uint64_t ip, uint64_t full_addr, uint32_t type)
{
  // find the least frequently used block
  return std::distance(current_set, std::min_element(current_set, std::next(current_set, NUM_WAY), lru_comparator<BLOCK, BLOCK>()));
}

// called on every cache hit and cache fill
void CACHE::update_replacement_state(uint32_t cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type,
                                     uint8_t hit)
{
  if (hit && type == WRITEBACK)
    return;

  auto begin = std::next(block.begin(), set * NUM_WAY);
  auto end = std::next(begin, NUM_WAY);
  uint32_t hit_lru = std::next(begin, way)->lru;
  uint32_t num = NUM_WAY;
  // a block whose lru reaches low+1 can be moved from unprivileged partition to privileged partition
  uint32_t low = 5;
  // a block which is moved from unprivileged partition to privileged partition will be set high+1
  // make sure high is larger than low
  uint32_t high = low+100;

  // a block whose lru reaches low+1 can be moved from unprivileged partition to privileged partition
  if (hit_lru>low && hit_lru<high){
    for (auto x = begin; x != end; ++x){
      if (x->lru > high){
        x->lru++;
        // if the ratio of privileged partition reaches 0.5, move the least recently used block to unprivileged partition
        if(x->lru>high+num*0.5){
          x->lru = 0;
        }
      }
    }
    std::next(begin, way)->lru = high+1;
  }
  // a block belonging to unprivileged partition add by 1
  else if(hit_lru<=low){
    std::next(begin, way)->lru = hit_lru+1;
  }
  // a block belonging to privileged partition update to high+1
  else{
    for (auto x = begin; x != end; ++x){
      if (x->lru <= hit_lru && x->lru>high){
        x->lru++;
        // if the ratio of privileged partition reaches 0.5, move the least recently used block to unprivileged partition
        if(x->lru>high+num*0.5){
          x->lru = 0;
        }
      }
    }
    std::next(begin, way)->lru = high+1;
  }
  
}

void CACHE::replacement_final_stats() {}
