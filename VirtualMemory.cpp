#pragma once

#include "MemoryConstants.h"
#include "VirtualMemory.h"
#include "PhysicalMemory.h"

void clear (uint64_t frame_index)
{
  for (uint64_t i = 0; i < PAGE_SIZE; i++)
  {
    PMwrite (frame_index * PAGE_SIZE + i, 0);
  }
}
/*
 * Initialize the virtual memory.
 */
void VMinitialize ()
{
  clear (0);
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
  uint64_t index = find (virtualAddress);
  if (index == -1)
  {
    return 0;
  }
  PMread (index, value);
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
  uint64_t index = find (virtualAddress);
  if (index == -1)
  {
    return 0;
  }
  PMwrite (index, value);
  return 1;
}
