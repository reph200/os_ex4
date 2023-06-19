#pragma once

#include "MemoryConstants.h"
#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include <cstdint>
#include <algorithm>
#include <cmath>

#define ROOT_TABLE_PAGE 0

uint64_t get_offset (uint64_t virtualAddress, uint64_t level)
{
  uint64_t shift = (VIRTUAL_ADDRESS_WIDTH - ((level + 1) * OFFSET_WIDTH));
  return (virtualAddress >> shift) & ((1 << OFFSET_WIDTH) - 1);
}

void clear (uint64_t frameIndex)
{
  for (uint64_t i = 0; i < PAGE_SIZE; i++)
  {
    PMwrite (frameIndex * PAGE_SIZE + i, 0);
  }
}
bool is_empty_frame (uint64_t frameIndex)
{
  word_t value;
  for (uint64_t i = 0; i < PAGE_SIZE; i++)
  {
    PMread (frameIndex * PAGE_SIZE + i, &value);
    if (value != 0)
    {
      return false;
    }
  }
  return true;
}

// get page by frame
uint64_t get_page_by_frame (uint64_t frameIndex)
{
  word_t value;
  PMread (frameIndex * PAGE_SIZE, &value);
  return value;
}

// get parent of frame
uint64_t get_parent (uint64_t frameIndex, uint64_t level)
{
  uint64_t page = get_page_by_frame (frameIndex);
  uint64_t offset = get_offset (page, level);
  return page - offset;
}

//

/*
 * min{NUM_PAGES - |page_swapped_in - p|, |page_swapped_in - p|}
 */
uint64_t cyclicDistance (uint64_t page_swapped_in, uint64_t p)
{
  uint64_t distance1 =
      NUM_PAGES - std::abs (static_cast<int64_t>(page_swapped_in - p));
  uint64_t distance2 = std::abs (static_cast<int64_t>(page_swapped_in - p));
  return std::min (distance1, distance2);
}

word_t translate_address (uint64_t virtualAddress)
{

  uint64_t addr[TABLES_DEPTH];
  uint64_t virtual_without_offset = virtualAddress >> OFFSET_WIDTH;
  for (uint64_t i = TABLES_DEPTH - 1; i <= 0; i--)
  {
      addr[i] = virtual_without_offset & ((1 << OFFSET_WIDTH) - 1);
      virtual_without_offset = virtual_without_offset >> OFFSET_WIDTH;
  }

  uint64_t offset = 0;
  uint64_t page_index = 0;
  for (uint64_t i = 0; i < TABLES_DEPTH; i++)
  {
    word_t value;
    offset = get_offset (virtualAddress, i);
    PMread (page_index * PAGE_SIZE + offset, &value);
    if (value == 0)
    {
      uint64_t frameIndex = 0;
      if (frameIndex == 0)
      {
        uint64_t min_distance = NUM_PAGES;
        uint64_t min_distance_frame = 0;
        for (uint64_t j = 0; j < NUM_FRAMES; j++)
        {
          uint64_t parent = get_parent (j, i);
          if (parent == page_index)
          {
            uint64_t page = get_page_by_frame (j);
            uint64_t distance = cyclicDistance (page, page_index);
            if (distance < min_distance)
            {
              min_distance = distance;
              min_distance_frame = j;
            }
          }
        }
        frameIndex = min_distance_frame;
        uint64_t page = get_page_by_frame (frameIndex);
        clear (frameIndex);
        PMwrite (frameIndex * PAGE_SIZE, page);
      }
      PMwrite (page_index * PAGE_SIZE + offset, frameIndex);
    }
    else
    {
      page_index = value;
    }

  }
  return page_index * PAGE_SIZE + offset;
}

/*
 * Initialize the virtual memory.
 */
void VMinitialize ()
{
  clear (ROOT_TABLE_PAGE);
}

/* Reads a word from the given virtual address
 * and puts its content in *value.
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
int VMread (uint64_t virtualAddress, word_t *value)
{
  uint64_t address = translate_address (virtualAddress);
  if (address == 0)
  {
    return 0;
  }
  PMread (address, value);
  return 1;
}

/* Writes a word to the given virtual address.
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
int VMwrite (uint64_t virtualAddress, word_t value)
{
  uint64_t address = translate_address (virtualAddress);
  if (address == 0)
  {
    return 0;
  }
  PMwrite (address, value);
  return 1;
}


