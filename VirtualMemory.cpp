#pragma once

#include "MemoryConstants.h"
#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include <cstdint>
#include <algorithm>
#include <cmath>
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#define abs(a) ((a)<(0)?(-a):(a))
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
  uint64_t distance1 = NUM_PAGES - abs (static_cast<int64_t>
                                             (page_swapped_in - p));
  uint64_t distance2 = abs (static_cast<int64_t>(page_swapped_in - p));
  return min (distance1, distance2);
}

word_t get_address_of_parent (word_t root, word_t frameIndex, uint64_t level)
{
  // return if we got to the bottom of the tree
  if (level == TABLES_DEPTH)
  { return 0; }

  word_t ret_val = 0;
  word_t value;
  // check if it is the father of the desired frame
  for (uint64_t i = 0; i < PAGE_SIZE; i++)
  {

    PMread (root * PAGE_SIZE + i, &value);
    if (value == frameIndex)
    {
      return root* PAGE_SIZE + i;
    }
      // recursively call the next level of the tree for each of the sons of
      // the current root
    else if (value != 0)
    {
      word_t parent_address = get_address_of_parent (value, frameIndex, level +
      1);
      if (parent_address != 0)
      {
        return parent_address;
      }
//      ret_val = ret_val + parent;
    }
  }
  return 0;
}

word_t get_empty_frame (word_t root, word_t justCreatedFrame, uint64_t level)
{
  if (level == TABLES_DEPTH || root == justCreatedFrame)
  { return 0; }
  if (is_empty_frame (root))
  {
    return root;
  }

  word_t value;
  word_t ret_val = 0;

  for (uint64_t i = 0; i < PAGE_SIZE; i++)
  {
    PMread (root * PAGE_SIZE + i, &value);
    if (value != 0)
    {
      ret_val =
          ret_val + get_empty_frame (value, justCreatedFrame, level + 1);
      if (ret_val != 0)
      { break; }
    }
  }
  return ret_val;
}

word_t get_unused_frame (word_t root,
                         uint64_t level)
{
  // return if we got to the bottom of the tree
  if (level == TABLES_DEPTH)
  { return 0; }

  word_t ret_val = 0;
  word_t value;
  for (uint64_t i = 0; i < PAGE_SIZE; i++)
  {

    PMread (root * PAGE_SIZE + i, &value);

    // recursively call the next level of the tree for each of the sons of
    // the current root
    if (value != 0)
    {
      ret_val = max(ret_val, max(value, get_unused_frame (value,level + 1)));
    }
  }
  return ret_val;
}

// get_page_of_max_dist
uint64_t get_page_of_max_dist (uint64_t page_swapped_in)
{
  uint64_t addr[TABLES_DEPTH];
  uint64_t maxPage = 0;
  uint64_t maxDist = 0;

  for (uint64_t i = 0; i < NUM_PAGES; i++)
  {
    uint64_t j = TABLES_DEPTH;
    uint64_t left_address = i;

    for (; j > 0; j--)
    {
      addr[j - 1] = left_address & ((1 << OFFSET_WIDTH) - 1);
      left_address = left_address >> OFFSET_WIDTH;
    }
    word_t nextIndex = ROOT_TABLE_PAGE;
    word_t pageIndex = ROOT_TABLE_PAGE;
    //check if the page is used
    for (j = 0; j < TABLES_DEPTH; j++)
    {
      PMread (pageIndex * PAGE_SIZE + addr[j], &nextIndex);
      if (nextIndex == 0)
      {
        break;
      }
      pageIndex = nextIndex;
    }
    // if the page is used
    if (j == TABLES_DEPTH)
    {
      uint64_t dist = cyclicDistance (page_swapped_in, i);
      //update the max distance and the max page if relevant
      if (dist > maxDist)
      {
        maxDist = dist;
        maxPage = i;
      }
    }

  }
  return maxPage;
}

// get_frame_of_page
word_t get_frame_of_page (uint64_t page)
{
  uint64_t addr[TABLES_DEPTH];

  uint64_t left_address = page;
  uint64_t j = TABLES_DEPTH;

  for (; j > 0; j--)
  {
    addr[j - 1] = left_address & ((1 << OFFSET_WIDTH) - 1);
    left_address = left_address >> OFFSET_WIDTH;
  }
  word_t value = ROOT_TABLE_PAGE;
  word_t pageIndex = ROOT_TABLE_PAGE;
  for (j = 0; j < TABLES_DEPTH; j++)
  {
    PMread (pageIndex * PAGE_SIZE + addr[j], &value);
    pageIndex = value;
  }
  return value;
}

word_t translate_address (uint64_t virtualAddress)
{
  uint64_t addr[TABLES_DEPTH + 1];
  uint64_t left_virtual_address = virtualAddress;
  for (uint64_t i = TABLES_DEPTH + 1; i > 0; i--)
  {
    addr[i - 1] = left_virtual_address & ((1 << OFFSET_WIDTH) - 1);
    left_virtual_address = left_virtual_address >> OFFSET_WIDTH;
  }

  word_t nextIndex = ROOT_TABLE_PAGE;
  word_t pageIndex = ROOT_TABLE_PAGE;
  word_t preFrameIndex = ROOT_TABLE_PAGE;
  word_t frameIndex;
  for (uint64_t i = 0; i < TABLES_DEPTH; i++)
  {
    PMread (pageIndex * PAGE_SIZE + addr[i], &nextIndex);
    if (nextIndex == 0)
    {
      //STAGE 1
      frameIndex = get_empty_frame (ROOT_TABLE_PAGE, preFrameIndex, 0);
      //STAGE 2
      if (frameIndex == 0)
      {
        frameIndex = get_unused_frame (ROOT_TABLE_PAGE, 0) + 1;
      }
      //STAGE 3 -  all frames are used
      if (frameIndex == NUM_FRAMES)
      {
        // find the page with the max distance from the current page
        uint64_t maxPage = get_page_of_max_dist (
            virtualAddress >> OFFSET_WIDTH);
        frameIndex = get_frame_of_page (maxPage);
        // disconnect from previous parent
        PMwrite (get_address_of_parent (ROOT_TABLE_PAGE, frameIndex, 0)
                 , 0);
        PMevict (frameIndex, maxPage);
      }
      //ALWAYS REACH HERE! creates new table
      clear (frameIndex);
      PMwrite (pageIndex * PAGE_SIZE + addr[i], frameIndex);
      preFrameIndex = frameIndex;
      nextIndex = frameIndex;
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


