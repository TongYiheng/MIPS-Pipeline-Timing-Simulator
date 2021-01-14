#include "cache.h"
#include "pipe.h"
#include "shell.h"
#include "mips.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

InstructionCache IC;
DataCache DC;

uint32_t mem_tag_inst;        //主存标记
uint32_t set_num_inst;        //cache组号
uint32_t block_offset_inst;   //块内偏移
uint32_t hit_line_inst;       //命中的cache行号

uint32_t mem_tag_data;        //主存标记
uint32_t set_num_data;        //cache组号
uint32_t block_offset_data;   //块内偏移
uint32_t hit_line_data;       //命中的cache行号

int counter_inst;           //指令cache的计数器
int counter_data;           //数据cache的计数器



/* 指令Cache相关实现 */

void initializeInstructionCache()
{
    for(int i=0;i<256;i++)
        memset(&IC.cacheLine[i],0,sizeof(instructioncacheLine));
}

int compare_inst(uint32_t address)
{
    mem_tag_inst=address>>11;                       //主存标记

    #ifdef DEBUG
        printf("cache组号：%x\n",set_num_inst);
        printf("主存标记：%x\n",mem_tag_inst);
    #endif

    for(int i=set_num_inst*4;i<set_num_inst*4+4;i++)
    {
        #ifdef DEBUG
            printf("当前的cache行:%d\n",i);
            printf("当前的cache行的有效位:%d\n",IC.cacheLine[i].valid);
            printf("当前的cache行的标记:%x\n",IC.cacheLine[i].tag);
        #endif
        if(IC.cacheLine[i].valid==1 && mem_tag_inst==IC.cacheLine[i].tag)   //命中
        {
            hit_line_inst=i;
            #ifdef DEBUG
                printf("命中的cache行：%d\n",hit_line_inst);
            #endif

            //LRU位的处理
            IC.cacheLine[i].LRU=0;
            for(int k=set_num_inst*4;k<set_num_inst*4+4;k++)
            {
                if(k!=i)
                    IC.cacheLine[k].LRU++;
            }

            return 1;
        }
    }
    return 0;
}

int delay_inst()
{
    if(counter_inst<=50)
    {
        counter_inst++;
        return 0;
    }
    else
    {
        counter_inst=0;
        return 1;
    }
}

uint32_t getInstruction_inst(uint32_t address)  //从cache中取一条指令
{
    block_offset_inst=((address-(mem_tag_inst << 11))%32)/4;
    return IC.cacheLine[hit_line_inst].data[block_offset_inst];
}

uint32_t moveFromMainMemory_inst(uint32_t address)   //从主存中读取一个块到cache中
{
    int row=set_num_inst*4;
    int remains=0;

    //先看有没有空余的行即valid=0
    for(int i=set_num_inst*4;i<set_num_inst*4+4;i++)
    {
        if(IC.cacheLine[i].valid==0)
        {
            row=i;
            remains=1;
            #ifdef DEBUG
                printf("空余的行:%d\n",row);
            #endif
            break;
        }
    }

    
    //如果不存在valid=0的行，找出LRU位最大的cache行
    if(!remains)
    {
        for(int i=set_num_inst*4+1;i<set_num_inst*4+4;i++)
        {
            if(IC.cacheLine[i].LRU>IC.cacheLine[row].LRU)
                row=i;
        }

        #ifdef DEBUG
            printf("LRU最大的行：%d\n",row);
        #endif
    }
    

    //将主存的一个块装入cache
    uint32_t begin=(address/32)<<5;
    uint32_t end=(address/32+1)<<5;
    #ifdef DEBUG
        printf("begin=%x\n",begin);
        printf("end=%x\n",end);
    #endif

    int k=0;
    for(uint32_t i=begin;i<end;i=i+4)
    {
        IC.cacheLine[row].data[k++]=mem_read_32(i);
    }

    //设置valid、tag、LRU位
    IC.cacheLine[row].valid=1;
    IC.cacheLine[row].tag=mem_tag_inst;
    IC.cacheLine[row].LRU=0;
    for(int i=set_num_inst*4;i<set_num_inst*4+4;i++)
    {
        if(i!=row)
            IC.cacheLine[i].LRU++;
    }

    return mem_read_32(address);
}




/* 数据Cache相关实现 */

void initializeDataCache()
{
    for(int i=0;i<2048;i++)
        memset(&DC.cacheLine[i],0,sizeof(datacacheLine));
}

int compare_data(uint32_t address)
{
    set_num_data=(address>>5)%256;                  //cache组号
    mem_tag_data=address>>13;                       //主存标记

    #ifdef DEBUG
        printf("cache组号：%x\n",set_num_data);
        printf("主存标记：%x\n",mem_tag_data);
    #endif

    for(int i=set_num_data*8;i<set_num_data*8+8;i++)
    {
        #ifdef DEBUG
            printf("当前的cache行:%d\n",i);
            printf("当前的cache行的有效位:%d\n",DC.cacheLine[i].valid);
            printf("当前的cache行的标记:%x\n",DC.cacheLine[i].tag);
        #endif
        if(DC.cacheLine[i].valid==1 && mem_tag_data==DC.cacheLine[i].tag)   //命中
        {
            hit_line_data=i;
            #ifdef DEBUG
                printf("命中的cache行：%d\n",hit_line_data);
            #endif

            //LRU位的处理
            DC.cacheLine[i].LRU=0;
            for(int k=set_num_data*8;k<set_num_data*8+8;k++)
            {
                if(k!=i)
                    DC.cacheLine[k].LRU++;
            }

            return 1;
        }
    }
    return 0;
}

int delay_data()
{
    if(counter_data<=50)
    {
        counter_data++;
        return 0;
    }
    else
    {
        counter_data=0;
        return 1;
    }
}

uint32_t getData(uint32_t address)  //从cache中取一条指令
{
    block_offset_data=((address-(mem_tag_data << 13))%32)/4;
    return DC.cacheLine[hit_line_data].data[block_offset_data];
}

uint32_t moveFromMainMemory_data(uint32_t address)   //从主存中读取一个块到cache中
{
    int row=set_num_data*8;
    int remains=0;

    //先看有没有空余的行即valid=0
    for(int i=set_num_data*8;i<set_num_data*8+8;i++)
    {
        if(DC.cacheLine[i].valid==0)
        {
            row=i;
            remains=1;
            #ifdef DEBUG
                printf("空余的行:%d\n",row);
            #endif
            break;
        }
    }

    
    //如果不存在valid=0的行，找出LRU位最大的cache行
    if(!remains)
    {
        for(int i=set_num_data*8+1;i<set_num_data*8+8;i++)
        {
            if(DC.cacheLine[i].LRU>DC.cacheLine[row].LRU)
                row=i;
        }

        #ifdef DEBUG
            printf("LRU最大的行：%d\n",row);
        #endif
    }
    
    if(DC.cacheLine[row].dirty==1)
    {
        for(int i=0;i<8;i++)
        {
            mem_write_32(DC.cacheLine[row].tag<<13 | (row/8)<<5 | (i*4), DC.cacheLine[row].data[i]);    //写回内存
        }
    }

    //将主存的一个块装入cache
    uint32_t begin=(address/32)<<5;
    uint32_t end=(address/32+1)<<5;
    #ifdef DEBUG
        printf("begin=%x\n",begin);
        printf("end=%x\n",end);
    #endif

    int k=0;
    for(uint32_t i=begin;i<end;i=i+4)
    {
        DC.cacheLine[row].data[k++]=mem_read_32(i);
    }

    //设置valid、tag、LRU位
    DC.cacheLine[row].valid=1;
    DC.cacheLine[row].tag=mem_tag_data;
    DC.cacheLine[row].LRU=0;
    for(int i=set_num_data*8;i<set_num_data*8+8;i++)
    {
        if(i!=row)
            DC.cacheLine[i].LRU++;
    }

    return mem_read_32(address);
}

void writeToCache(uint32_t address, uint32_t value)
{
    if(compare_data(address))
    {
        uint32_t offset=((address-(mem_tag_data << 13))%32)/4;
        DC.cacheLine[hit_line_data].dirty=1;
        DC.cacheLine[hit_line_data].data[offset]=value;
    }
    else
    {
        moveFromMainMemory_data(address);   //写回的内存块不在cache，先读进来
        compare_data(address);
        uint32_t offset=((address-(mem_tag_data << 13))%32)/4;
        DC.cacheLine[hit_line_data].dirty=1;
        DC.cacheLine[hit_line_data].data[offset]=value;
    }
    
}

void writeToMainMemory(uint32_t address, uint32_t value)
{
    mem_write_32(address,value);    //写回内存
}