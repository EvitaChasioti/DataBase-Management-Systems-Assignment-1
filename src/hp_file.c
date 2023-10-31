#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "hp_file.h"
#include "record.h"

#define CALL_BF(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {         \
    BF_PrintError(code);    \
    exit(code);                \
  }                         \
}

int HP_CreateFile(char *fileName){
  int fileDescriptor;
  void* data;
  int block_num;
  BF_Block* block;
  BF_Block_Init(&block);

  CALL_BF(BF_CreateFile(fileName));
  CALL_BF(BF_OpenFile(fileName, &fileDescriptor));

  CALL_BF(BF_AllocateBlock(fileDescriptor, block));
  CALL_BF(BF_GetBlockCounter(fileDescriptor, &block_num));
  data = BF_Block_GetData(block);

  HP_info* hpinfo=data;
  hpinfo->bytes_size=BF_BLOCK_SIZE-sizeof(HP_block_info);
  hpinfo->capacity=(hpinfo->bytes_size/sizeof(Record));
  
  //printf("%d\n",hpinfo->capacity);
  //printf("%d\n",hpinfo->bytes_size);

  hpinfo->last_block_id=block_num-1;

  BF_Block_SetDirty(block);
  CALL_BF(BF_UnpinBlock(block));
  CALL_BF(BF_CloseFile(fileDescriptor));
  return 0;
}

HP_info* HP_OpenFile(char *fileName, int *file_desc){
  CALL_BF(BF_OpenFile(fileName, file_desc));
  BF_Block* block;
  BF_Block_Init(&block);
  CALL_BF(BF_GetBlock(*file_desc, 0, block));
  void * data=BF_Block_GetData(block);
  HP_info* hpinfo=data;
  return hpinfo;
}


int HP_CloseFile(int file_desc,HP_info* hp_info ){
}

int HP_InsertEntry(int file_desc,HP_info* hp_info, Record record){
  if (hp_info->last_block_id == 0) {
    BF_Block* block;
    BF_Block_Init(&block);

    void* records_data;
    void* block_info_data;

    CALL_BF(BF_AllocateBlock(file_desc, block));
    records_data = BF_Block_GetData(block);
    block_info_data = records_data + hp_info->bytes_size;

    HP_block_info* block_info = block_info_data;
    block_info->block_id = hp_info->last_block_id + 1;
    block_info->num_of_records = 0;
    block_info->next = NULL;
    hp_info->last_block_id++;

    Record* rec = records_data;
    rec[block_info->num_of_records++] = record;

    printf("%s ", rec[block_info->num_of_records-1].name);
    printf("%d ", rec[block_info->num_of_records-1].id);
    printf("%d ", block_info->block_id);
    printf("%d\n", block_info->num_of_records);

    BF_Block_SetDirty(block);
    CALL_BF(BF_UnpinBlock(block));
  }
  else {
    BF_Block* block;
    BF_Block_Init(&block);
    CALL_BF(BF_GetBlock(file_desc, hp_info->last_block_id, block));

    void* records_data;
    void* block_info_data;
    
    records_data = BF_Block_GetData(block);
    block_info_data = records_data + hp_info->bytes_size;
    HP_block_info* block_info = block_info_data;

    if (block_info->num_of_records < hp_info->capacity) {
      printf("Adding a record... ");
      Record* rec = records_data;
      rec[block_info->num_of_records] = record;
      

      printf("%s ", rec[block_info->num_of_records].name);
      printf("%d ", rec[block_info->num_of_records].id);
      printf("%d ", block_info->block_id);

      if (block_info->num_of_records < hp_info->capacity)
        block_info->num_of_records++;
      printf("%d\n", block_info->num_of_records);

      BF_Block_SetDirty(block);
      CALL_BF(BF_UnpinBlock(block));
    }
    else {
      printf("Creating a new Block with id = %d... ", hp_info->last_block_id);
      BF_Block* new_block;
      BF_Block_Init(&new_block);
      printf("%p\n", new_block);
      CALL_BF(BF_AllocateBlock(file_desc, new_block));
      records_data = BF_Block_GetData(new_block);
      block_info_data = records_data + hp_info->bytes_size;

      HP_block_info* block_info2 = block_info_data;
      block_info2->block_id = hp_info->last_block_id + 1;
      block_info2->num_of_records = 0;
      block_info2->next = NULL;
      hp_info->last_block_id++;

      Record* rec = records_data;
      rec[block_info2->num_of_records++] = record;
      block_info->next = new_block;
      
      printf("%p =?=", block_info->next);
      printf("%p\n", new_block);
      printf("%s ", rec[block_info2->num_of_records-1].name);
      printf("%d ", rec[block_info2->num_of_records-1].id);
      printf("%d ", block_info2->block_id);
      printf("%d\n", block_info2->num_of_records);

      BF_Block_SetDirty(new_block);
      CALL_BF(BF_UnpinBlock(new_block));
    }
    BF_Block_SetDirty(block);
    CALL_BF(BF_UnpinBlock(block));
  }
  return 0;
}

int HP_GetAllEntries(int file_desc,HP_info* hp_info, int value){ 
  BF_Block* block;
  BF_Block_Init(&block);
  void* records_data;
  void* block_info_data;
  int count=0;
  int count2=0;
  for (int id = 1; id <= hp_info->last_block_id; id++) {
    CALL_BF(BF_GetBlock(file_desc, id, block));
    records_data = BF_Block_GetData(block);
    block_info_data = records_data + hp_info->bytes_size;
    HP_block_info* block_info = block_info_data;
    Record* rec = records_data;
    for (int i = 0; i < block_info->num_of_records; i++) {
      if(rec[i].id==value){
        printf("%s %s %s\n",rec[i].name, rec[i].surname, rec[i].city  );
        count2+=count;
      }
    }
    printf("Changing block...\n");
    count++;
  }

  printf("%d  %d\n", count, count2);
  return 0;
}
