#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define FRAME_SIZE 256 // 帧大小
#define ADDRESSES_SIZE 1000 //address.txt虚拟地址条目数
#define BinaryNum 100
#define TLB_Size 16 //TLB大小

FILE *OutFile;//输出文件
FILE *VirtualAddrFile;//虚拟地址文件
FILE *AddrStoreFile;//存储地址文件

int addresses[ADDRESSES_SIZE];//虚拟地址文件
int FrameUse[FRAME_SIZE / 2]; //判断页帧是否被使用
char PhyMemory[FRAME_SIZE][FRAME_SIZE];

//读取文件
int ReadFile()
{
    int i=0;
    if((VirtualAddrFile = fopen("addresses.txt","r"))== NULL) return 0;
    while(fscanf(VirtualAddrFile, "%d", &addresses[i]) != EOF) i++;
    fclose(VirtualAddrFile);//关闭文件
}

struct node{
    int key;
    int value;
    struct node* prev;
    struct node* next;
};//双向链表

typedef struct Node node;
typedef struct Node{
    int pageNum;
    int frameNum;
    int inMemory;
    node *next;
}Node;

typedef struct  {
    node* front;
    node* rear;
    int num;
}TLBlist;

void initTLB(TLBlist *TLB) {
    TLB->front = NULL;
    TLB->rear = NULL;
    TLB->num = 0;
}

void delFullTLB(TLBlist* TLB){//仅队列满时调用
    node* temp = TLB->front;
    //删除node
    TLB->front = TLB->front->next;
    free(temp);
    TLB->num--;
}

void insertTLB(TLBlist *TLB, int pageNum, int frameNum)
{
    if (TLB->num <= TLB_Size) {//TLB未满
        //node初始化
        node* temp = (node*)malloc(sizeof(node));
        temp->pageNum = pageNum;
        temp->frameNum = frameNum;
        temp->inMemory = 1;
        temp->next = NULL;
        //加入TLB
        if (TLB->rear == NULL) {//第一个加入node
                TLB->front = temp;
                TLB->rear = temp;
                TLB->num++;
        }
        else {
                TLB->rear->next = temp;
                TLB->rear = temp;
                TLB->num++;
        }
    }
    else {//TLB满，FIFO策略
            //清除最后的node
            delFullTLB(TLB);
            //加入node
            insertTLB(TLB, pageNum, frameNum);
    }
}

void ChangePage(int pageNum,int rearFrameNum,char PhyMemory[][256],int pageC[][2]) {
    FILE* diskFile;
    diskFile = fopen("BACKING_STORE.bin", "rb");
    fseek(diskFile, 256 * pageNum, 0);
    char* buffer = (char*)malloc(FRAME_SIZE+1);
    fread(buffer, 1, 256, diskFile);
    for (int i = 0; i < 256; i++) {
            PhyMemory[rearFrameNum][i] = buffer[i];
    }
    //更新page
    pageC[pageNum][1] = 1;
    pageC[pageNum][0] = rearFrameNum;
}


struct node TLB[FRAME_SIZE];
int TLBHit = 0;
int page_default = 0;
int firstFram = 0;
int point = 0;
int point_TLB = 0;
int rearFrameNum = 0;//指向内存最后插入的frame number后的空frame
//创建page
int main(void) {
    
    ReadFile();
    int pageC[256][2];
    for(int i=0;i<256;i++)pageC[i][1]=0;
    //创建TLB
    TLBlist *TLBC = (TLBlist*)malloc(sizeof(TLBlist));
    initTLB(TLBC);

    
    for(int i = 0; i <ADDRESSES_SIZE; i++){
        int pageNum = (addresses[i] >> 8) & 255;
        int offset = addresses[i] & 255;
        int frameNum;
        //遍历TLB，看是否在其中
        int inTLBC = 0;
        node *tempC = TLBC->front;
        int test = 0;
        if (TLBC->num)
        { //TLB不为空
            while (tempC != NULL)
            {
                if (tempC->pageNum == pageNum)
                {
                    test = 1;
                    //printf("  TLB  ");
                    frameNum = tempC->frameNum;
                    TLBHit++;
                    // printf("出现一次tlb命中   ");
                    inTLBC = 1;
                    break;
                }
                tempC = tempC->next;
            }
        }
        if (inTLBC == 0) {//不在TLB中
            if (pageC[pageNum][1])
            { //在页表但不在TLB中
                test = 2;
                //printf("  page 在内存中  ");
                frameNum = pageC[pageNum][0];
                //加入TLB
                insertTLB(TLBC, pageNum, frameNum);
            }
            else {//缺页错误
                test = 3;
                //printf(" page 缺页错误  ");
                // printf("出现了一次缺页！   ");
                page_default++;
                ChangePage(pageNum, rearFrameNum,PhyMemory,pageC);
                frameNum = pageC[pageNum][0];
                rearFrameNum++;
                insertTLB(TLBC, pageNum, frameNum);
            }
        }

        printf("%d", PhyMemory[frameNum][offset]);
        if (i != ADDRESSES_SIZE - 1)
            printf("\n");
    }
    
    double TLB_p = (double)TLBHit/1000;
    double page = (double)page_default/1000;
    printf("TLBHIT :  %lf   ,pageFalut: %lf", TLB_p, page);
    return 0;
}
