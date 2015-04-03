/* datastore.h */

#ifndef __DATASTORE_H
#define __DATASTORE_H

#include <stdint.h>
#include <FreeRTOS.h>
#include <semphr.h>

typedef struct
{
  uint8_t * bufferStart;
  uint8_t * bufferEnd;
  uint16_t maxSize;
  uint16_t size;
  uint8_t * head;
  uint8_t * tail;
  uint8_t packetSize;
  SemaphoreHandle_t mutex;
} DATASTORE_RamRingbuffer;

#define SUCCESS 0
#define ERR_BUFFER_FULL 1
#define ERR_BUFFER_EMPTY 2
#define ERR_TIMEOUT 3

uint8_t DATASTORE_RamRingbuffer_Initialize(
                    DATASTORE_RamRingBuffer * ramringbuffer, 
                    uint8_t * buffer, 
                    uint16_t size, 
                    uint8_t packet_size);
                    
uint8_t DATASTORE_RamRingbuffer_Push(
                    DATASTORE_RamRingBuffer * ramringbuffer, 
                    uint8_t * packet);
                            
uint8_t DATASTORE_RamRingbuffer_Pop(
                    DATASTORE_RamRingbuffer * ramringbuffer, 
                    uint8_t * dest);
                    
uint8_t DATASTORE_InitializeStandardBuffers();

DATASTORE_RamRingbuffer * DATASTORE_RamRingbuffer_Test;
                    
#endif