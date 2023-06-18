#pragma once

#include "MemoryConstants.h"
#include "VirtualMemory.h"
#include "PhysicalMemory.h"

#define ROOT_TABLE_FRAME 0

uint64_t get_offset(uint64_t virtualAddress, uint64_t level)
{
  uint64_t shift = (VIRTUAL_ADDRESS_WIDTH - OFFSET_WIDTH) - (level * bitWidth(NUM_FRAMES));
  return (virtualAddress >> shift) & (NUM_FRAMES - 1);
}

int get_address(uint64_t virtualAddress)
{
  uint64_t page = virtualAddress;
  uint64_t frame = 0;
  for (uint64_t i = 0; i < TABLES_DEPTH; i++)
  {
    word_t value;
    PMread (frame * PAGE_SIZE + get_offset(virtualAddress,i), &value);
      if (value == 0)
      {

      }
      else
      {
        frame = value;
      }

  }
  return frame * PAGE_SIZE + table;
}

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
  clear (ROOT_TABLE_FRAME);
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
  uint64_t address =  get_address(virtualAddress);
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
  uint64_t address = get_address (virtualAddress);
  if (address == 0)
  {
    return 0;
  }
  PMwrite (address, value);
  return 1;
}


