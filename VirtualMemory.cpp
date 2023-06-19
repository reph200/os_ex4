#pragma once

#include "MemoryConstants.h"
#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include <cstdint>
#include <algorithm>
#include <cmath>

#define ROOT_TABLE_PAGE 0


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

uint64_t cyclicDistance (uint64_t page_swapped_in, uint64_t p)
{
  uint64_t distance1 = NUM_PAGES - std::abs (static_cast<int64_t>
      (page_swapped_in - p));
  uint64_t distance2 = std::abs (static_cast<int64_t>(page_swapped_in - p));
  return std::min (distance1, distance2);
}

word_t get_parent_of_frame (word_t root ,word_t frameIndex)
{
 // dfs on the tree and find the parent of the frame
  word_t value;
  for(uint64_t i = 0; i < PAGE_SIZE; i++)
  {

    PMread (root * PAGE_SIZE+i, &value);
    if (value == frameIndex)
    {
      return root;
    }
    else
    {
      return get_parent_of_frame (value, frameIndex);
    }
  }
}

word_t get_unused_or_empty_frame (word_t justCreatedFrame)
{
  word_t frameIndex = 0;

  ...
  if (frameIndex != justCreatedFrame && is_empty_frame (frameIndex))
  {
    return frameIndex;
  }
  ...


  if (frameIndex == NUM_FRAMES - 1)
  {
    return 0;
  }
  else
  {
    return frameIndex + 1;
  }
}

// get_page_of_max_dist
word_t get_page_of_max_dist (uint64_t virtualAddress)
{
  word_t maxPage = 0;
  word_t maxDist = 0;
  for (word_t i = 0; i < NUM_PAGES; i++)
  {
    word_t dist = cyclicDistance (i, virtualAddress);
    if (dist > maxDist)
    {
      maxDist = dist;
      maxPage = i;
    }
  }
  return maxPage;
}

// get_frame_of_page
word_t get_frame_of_page (word_t page)
{
  word_t frameIndex;
  for (frameIndex = 0; frameIndex < NUM_FRAMES; frameIndex++)
  {
    word_t value;
    PMread (frameIndex * PAGE_SIZE, &value);
    if (value == page)
    {
      break;
    }
  }
  return frameIndex;
}

word_t translate_address (uint64_t virtualAddress)
{
  uint64_t addr[TABLES_DEPTH + 1];
  uint64_t left_virtual_address = virtualAddress;
  for (uint64_t i = TABLES_DEPTH; i <= 0; i--)
  {
    addr[i] = left_virtual_address & ((1 << OFFSET_WIDTH) - 1);
    left_virtual_address = left_virtual_address >> OFFSET_WIDTH;
  }

  word_t nextIndex = ROOT_TABLE_PAGE;
  word_t pageIndex = ROOT_TABLE_PAGE;
  word_t preFrameIndex = ROOT_TABLE_PAGE;
  for (uint64_t i = 0; i < TABLES_DEPTH; i++)
  {
    PMread (pageIndex * PAGE_SIZE + addr[i], &nextIndex);
    if (nextIndex == 0)
    {
      word_t frameIndex = get_unused_or_empty_frame (preFrameIndex);
      preFrameIndex = frameIndex;
      if (frameIndex == 0) // all frames are used
      {
        // find the page with the max distance from the current page
        word_t maxPage = get_page_of_max_dist (virtualAddress >> OFFSET_WIDTH);
        frameIndex = get_frame_of_page (maxPage);
        // disconnect from previous parent
        PMevict (frameIndex, maxPage);
        PMwrite (get_parent_of_frame (frameIndex), 0);
      }
      clear (frameIndex);
      PMwrite (pageIndex * PAGE_SIZE + addr[i], frameIndex);
    }

    pageIndex = nextIndex;
  }
  PMrestore (pageIndex, virtualAddress >> OFFSET_WIDTH);
  return pageIndex * PAGE_SIZE + addr[TABLES_DEPTH];
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


