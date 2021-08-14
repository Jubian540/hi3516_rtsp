/*ringbuf .c*/

#include<stdio.h>
#include<ctype.h>
#include <stdlib.h>
#include <string.h>
#include "ringfifo.h"
#include "rtputils.h"
#include "sample_comm.h"
#define NMAX 32

int iput = 0; /* ���λ������ĵ�ǰ����λ�� */
int iget = 0; /* �������ĵ�ǰȡ��λ�� */
int n = 0; /* ���λ������е�Ԫ�������� */

struct ringbuf ringfifo[NMAX];

extern int UpdateSpsOrPps(unsigned char *data,int frame_type,int len);

extern void UpdateSps(unsigned char *data,int len);
extern void UpdatePps(unsigned char *data,int len);

/* ���λ������ĵ�ַ��ż��㺯����������﻽�ѻ�������β�������ƻص�ͷ����
���λ���������Ч��ַ���Ϊ��0��(NMAX-1)
*/
void ringmalloc(int size)
{
    int i;
    for(i =0; i<NMAX; i++)
    {
        ringfifo[i].buffer = malloc(size);
        ringfifo[i].size = 0;
        ringfifo[i].frame_type = 0;
       // printf("FIFO INFO:idx:%d,len:%d,ptr:%x\n",i,ringfifo[i].size,(int)(ringfifo[i].buffer));
    }
    iput = 0; /* ���λ������ĵ�ǰ����λ�� */
    iget = 0; /* �������ĵ�ǰȡ��λ�� */
    n = 0; /* ���λ������е�Ԫ�������� */
}
/**************************************************************************************************
**
**
**
**************************************************************************************************/
void ringreset()
{
    iput = 0; /* ���λ������ĵ�ǰ����λ�� */
    iget = 0; /* �������ĵ�ǰȡ��λ�� */
    n = 0; /* ���λ������е�Ԫ�������� */
}
/**************************************************************************************************
**
**
**
**************************************************************************************************/
void ringfree(void)
{
    int i;
    printf("begin free mem\n");
    for(i =0; i<NMAX; i++)
    {
       // printf("FREE FIFO INFO:idx:%d,len:%d,ptr:%x\n",i,ringfifo[i].size,(int)(ringfifo[i].buffer));
        free(ringfifo[i].buffer);
        ringfifo[i].size = 0;
    }
}
/**************************************************************************************************
**
**
**
**************************************************************************************************/
int addring(int i)
{
    return (i+1) == NMAX ? 0 : i+1;
}

/**************************************************************************************************
**
**
**
**************************************************************************************************/
/* �ӻ��λ�������ȡһ��Ԫ�� */

int ringget(struct ringbuf *getinfo)
{
    int Pos;
    if(n>0)
    {
        Pos = iget;
        iget = addring(iget);
        n--;
        getinfo->buffer = (ringfifo[Pos].buffer);
        getinfo->frame_type = ringfifo[Pos].frame_type;
        getinfo->size = ringfifo[Pos].size;
        //printf("Get FIFO INFO:idx:%d,len:%d,ptr:%x,type:%d\n",Pos,getinfo->size,(int)(getinfo->buffer),getinfo->frame_type);
		//fflush(stdout);
        return ringfifo[Pos].size;
    }
    else
    {
        //printf("Buffer is empty\n");
        return 0;
    }
}
/**************************************************************************************************
**
**
**
**************************************************************************************************/
/* ���λ������з���һ��Ԫ��*/
void ringput(unsigned char *buffer,int size,int encode_type)
{

    if(n<NMAX)
    {
        memcpy(ringfifo[iput].buffer,buffer,size);
        ringfifo[iput].size= size;
        ringfifo[iput].frame_type = encode_type;
        //printf("Put FIFO INFO:idx:%d,len:%d,ptr:%x,type:%d\n",iput,ringfifo[iput].size,(int)(ringfifo[iput].buffer),ringfifo[iput].frame_type);
        iput = addring(iput);
        n++;
    }
    else
    {
        //  printf("Buffer is full\n");
    }
}

/**************************************************************************************************
**
**
**
**************************************************************************************************/
HI_S32 HisiPutH265DataToBuffer(VENC_STREAM_S *pstStream)
{
	HI_S32 i,j,x;
	HI_S32 len=0,off=0,len2=2,uplen=0;
	unsigned char *pstr;
	int iframe=0;
	for (i = 0; i < pstStream->u32PackCount; i++)
	{
		len+=pstStream->pstPack[i].u32Len;
	}
	if(len>=400*1024)
	{
		printf("drop data %d\n",len);
		return HI_SUCCESS;
	}

    if(n<NMAX)
    {
		for (i = 0; i < pstStream->u32PackCount; i++)
		{
			memcpy(ringfifo[iput].buffer+off,pstStream->pstPack[i].pu8Addr,pstStream->pstPack[i].u32Len);
			off+=pstStream->pstPack[i].u32Len;
			pstr=pstStream->pstPack[i].pu8Addr;

			#if 0
			//��H265��ȡvps��Ϣ  NAL_UNIT_VPS,   32 =>0x20
			if(pstr[4]==0x40)
			{
				UpdateVps(pstr+4,pstStream->pstPack[i].u32Len-4);
			}
			
			if(pstr[4]==0x42)//��ȡsps��Ϣ
			{
				UpdateSps(pstr+4,pstStream->pstPack[i].u32Len-4);// NAL_UNIT_SPS, 33  
				//iframe=1;
			}
			if(pstr[4]==0x44)//��ȡpps��Ϣ
			{
				UpdatePps(pstr+4,pstStream->pstPack[i].u32Len-4);//NAL_UNIT_PPS, 34   
			}
			if(pstr[4]==0x26)//�ҵ�I frame IDR  
			{
				iframe=1;
			}
			#endif

			switch(pstr[4])
			{
				case 0x40:
					UpdateVps(pstr+4,pstStream->pstPack[i].u32Len-4);
					break;
				case 0x42:
					UpdateSps(pstr+4,pstStream->pstPack[i].u32Len-4);
					break;
				case 0x44:
					UpdatePps(pstr+4,pstStream->pstPack[i].u32Len-4);//NAL_UNIT_PPS, 34
					break;
				case 0x4e:
					UpdateSei(pstr+4,pstStream->pstPack[i].u32Len-4);
					break;
				case 0x26:
					ringfifo[iput].frame_type=FRAME_TYPE_I;
					break;
				case 0x02:
					ringfifo[iput].frame_type = FRAME_TYPE_P;
					break;
			}

		}

        ringfifo[iput].size= len;
	   #if 0
		if(iframe)
		{
//			printf("I");
			ringfifo[iput].frame_type = FRAME_TYPE_I;
		}        	
		else
		{
			ringfifo[iput].frame_type = FRAME_TYPE_P;
//			printf("P");
		}
		#endif
        iput = addring(iput);
//		printf("(%d)",iput);
//		fflush(stdout);
        n++;
    }

	return HI_SUCCESS;
}
