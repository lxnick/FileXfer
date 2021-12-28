#ifndef FILE_XFER_HEADER
#define FILE_XFER_HEADER

#include "stdint.h"
#include "stdbool.h"

#define RESERVED        (112)
#define FILEXFER_INFO   (128)

#define FILEXFER_PREFIX     (0xA5A55A01)
#define FILEXFER_OPTION     (0xABCDEF01)
#define FILEXFER_POSTFIX    (0xFFDDEECC)

#define FILEXFER_MAX_SIZE   (64*1024*1024)
#define	FILEXFER_MAX_BLOCK	(64*1024)	

#define FILEXFER_BLOCK_ACK		"ACK"
#define FILEXFER_COMPLETE_ACK	"BYE"

struct filexfer
{
    uint32_t    prefix;
    char        filename[64];
    uint32_t    filelength;
    uint32_t    option;
    uint8_t     reserved[RESERVED];
    uint32_t    postfix;
}   __attribute__((packed));


// #define MIN(X,Y) ((X) < (Y) ? : (X) : (Y))

void filexfer_setup(struct filexfer* xfer, char* filename, uint32_t length);
void filexfer_print(struct filexfer* xfer);
bool filexfer_extract(struct filexfer* xfer,char* filename, uint32_t length);

#endif
