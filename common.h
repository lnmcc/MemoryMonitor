/* ************************************************************************
	> File Name: common.h 
	> Author: lnmcc
	> Mail: lnmcc@hotmail.com 
	> Blog: lnmcc.net 
	> Created Time: 2014年04月11日 星期五 15时08分46秒
 *********************************************************************** */

#ifndef _MEMCHECKERHEADER_H 
#define _MEMCHECKERHEADER_H 

#define SINGLE_NEW           0x00
#define ARRAY_NEW            0x01 
#define SINGLE_DELETE        0x02
#define ARRAY_DELETE         0x03
#define SYS_SINGLE_NEW       0x04
#define SYS_ARRAY_NEW        0x05

#define FILENAME_LEN         128
#define MSG_TYPE             0X12345678

using namespace std;

typedef struct {
    char		    fileName[FILENAME_LEN];	
    unsigned long	lineNum;
    size_t 	     	size;	
    int	        	type;
    void*	    	address;
} Operation;

typedef struct  {
    int	            type;			
    Operation       OP;		
} MsgEntity;

typedef struct {
    char            fileName[FILENAME_LEN];
    unsigned long   lineNum;
    void*           address;
    int             type;
    unsigned long   totalSize;
} MemStatus;

#endif
