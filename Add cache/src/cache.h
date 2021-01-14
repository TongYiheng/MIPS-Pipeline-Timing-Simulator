#ifndef _CACHE_H_
#define _CACHE_H_

#include "shell.h"


/* 指令Cache */

typedef struct instructioncacheLine
{
    int valid;                      //有效位
    int LRU;                        //LRU位
    uint32_t tag;                   //主存标记
    uint32_t data[8];               //块数据32bytes
}instructioncacheLine;

typedef struct InstructionCache
{
    instructioncacheLine cacheLine[256];
}InstructionCache;

void initializeInstructionCache();                      //初始化
int compare_inst(uint32_t address);                     //比较CPU传入的地址的tag是否在cache行 
int delay_inst();
//char getData(uint32_t address);                       //从cache中取一个字节数据
uint32_t getInstruction_inst(uint32_t address);         //从cache中取一条指令
uint32_t moveFromMainMemory_inst(uint32_t address);     //从主存中读取一个块到cache中



/* 数据Cache */

typedef struct datacacheLine
{
    int valid;                  //有效位
    int LRU;                    //LRU位
    int dirty;                  //dirty位----用于写回
    uint32_t tag;               //主存标记
    uint32_t data[8];           //块数据32bytes
}datacacheLine;

typedef struct DataCache
{
    datacacheLine cacheLine[2048];
}DataCache;

void initializeDataCache();                             //初始化
int compare_data(uint32_t address);                     //比较CPU传入的地址的tag是否在cache行 
int delay_data();
uint32_t getData(uint32_t address);                     //从cache中取一条指令
uint32_t moveFromMainMemory_data(uint32_t address);     //从主存中读取一个块到cache中
void writeToCache(uint32_t address, uint32_t value);
void writeToMainMemory(uint32_t address, uint32_t value);


#endif