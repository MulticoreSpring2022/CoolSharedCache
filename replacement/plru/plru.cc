#include <algorithm>
#include <iterator>

#include "cache.h"
#include "util.h"

/*  Pseudo-LRU
 *  Each bit represents one branch point in a binary decision tree; let 1
    represent that the left side has been referenced more recently than the
    right side, and 0 vice-versa.

    Ex. Hit the Block#2.

                   bit0 = 0
                 /          \\
               /             \\
              /               \\
          bit1 = 0           bit2 = 1
          /     \            //     \
         /       \          //       \
    Block0     Block1    Block2   Block3

 *  If there are n blocks in a set, then the number of bits for the record is n-1.
 *  In this program, we considered the element->lru as a branch point.
 *  The root bit is the lru of the (NUM_WAY/2 -1)th block.
 */

// keep using lru to trace the status in PLRU

void CACHE::initialize_replacement()
{

  // set all blocks status -1
  // lru has only two status in PLRU: 0 1
  for (auto& blk : block)
    blk.lru = -1;
}

// find replacement victim
uint32_t CACHE::find_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK* current_set, uint64_t ip, uint64_t full_addr, uint32_t type)
{
  // When cache fill, the last element's lru is -1. Except that, others are either 0 or 1.
  // n-way set have (n-1) bits to record the status

  auto base = std::next(current_set, 0);
  auto find = base;

  double z = (double)NUM_WAY / 2;
  int d = (int)z; // Distance

  // Search from the root bit which is the lru of (NUM_WAY/2 -1)th block.
  // From the root, do the opposite way and follow the direction to find the next branch until the leaf.
  while (z < 1) {
    z = z / 2;
    find = std::next(current_set, d - 1);

    if (find->lru == 0) {
      d = (int)(d - z);
    } else if (find->lru == 1)
      d = (int)(d + z);
  }

  return d;
}

// called on every cache hit and cache fill
void CACHE::update_replacement_state(uint32_t cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type,
                                     uint8_t hit)
{
  /* stay the part of LRU below */
  if (hit && type == WRITEBACK)
    return;

  auto begin = std::next(block.begin(), set * NUM_WAY); // line --> offset
  auto target = std::next(begin, 0);

  double y = (double)NUM_WAY / 2;
  int x = (int)y; // Index of the block in one set

  // While hit a block, update the bits tree corresponding to the direction from the root to the leaf.
  // The root bit is the lru of the (NUM_WAY/2 -1)th block.
  // 1 represent that the left side has been referenced more recently than the right side, and 0 vice-versa.
  while (y < 1) {
    y = y / 2;
    target = std::next(begin, x - 1);

    if (way > (x - 1)) {
      target->lru = 0;
      x = (int)(x + y);
    } else {
      target->lru = 1;
      x = (int)(x - y);
    }
  }
}

void CACHE::replacement_final_stats() {}
