/* filesys.c
 * 
 * provides interface to virtual disk
 * 
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "filesys.h"


diskblock_t  virtualDisk [MAXBLOCKS] ;           // define our in-memory virtual, with MAXBLOCKS blocks
fatentry_t   FAT         [MAXBLOCKS] ;           // define a file allocation table with MAXBLOCKS 16-bit entries
fatentry_t   rootDirIndex            = 0 ;       // rootDir will be set by format
direntry_t * currentDir              = NULL ;
fatentry_t   currentDirIndex         = 0 ;

/* writedisk : writes virtual disk out to physical disk
 * 
 * in: file name of stored virtual disk
 */

void writedisk ( const char * filename )
{
   printf ( "writedisk> virtualdisk[0] = %s\n", virtualDisk[0].data ) ;
   FILE * dest = fopen( filename, "w" ) ;
   if ( fwrite ( virtualDisk, sizeof(virtualDisk), 1, dest ) < 0 )
      fprintf ( stderr, "write virtual disk to disk failed\n" ) ;
   //write( dest, virtualDisk, sizeof(virtualDisk) ) ;
   fclose(dest) ;
   
}

void readdisk ( const char * filename )
{
   FILE * dest = fopen( filename, "r" ) ;
   if ( fread ( virtualDisk, sizeof(virtualDisk), 1, dest ) < 0 )
      fprintf ( stderr, "write virtual disk to disk failed\n" ) ;
   //write( dest, virtualDisk, sizeof(virtualDisk) ) ;
   fclose(dest) ;
}


/* the basic interface to the virtual disk
 * this moves memory around
 */

void writeblock ( diskblock_t * block, int block_address )
{
   //printf ( "writeblock> block %d = %s\n", block_address, block->data ) ;
   memmove ( virtualDisk[block_address].data, block->data, BLOCKSIZE ) ;
   //printf ( "writeblock> virtualdisk[%d] = %s / %d\n", block_address, virtualDisk[block_address].data, (int)virtualDisk[block_address].data ) ;
}


/* read and write FAT
 * 
 * please note: a FAT entry is a short, this is a 16-bit word, or 2 bytes
 *              our blocksize for the virtual disk is 1024, therefore
 *              we can store 512 FAT entries in one block
 * 
 *              how many disk blocks do we need to store the complete FAT:
 *              - our virtual disk has MAXBLOCKS blocks, which is currently 1024
 *                each block is 1024 bytes long
 *              - our FAT has MAXBLOCKS entries, which is currently 1024
 *                each FAT entry is a fatentry_t, which is currently 2 bytes
 *              - we need (MAXBLOCKS /(BLOCKSIZE / sizeof(fatentry_t))) blocks to store the
 *                FAT
 *              - each block can hold (BLOCKSIZE / sizeof(fatentry_t)) fat entries
 */

/* implement format()
 */
void format ( )
{
   diskblock_t block ;
   direntry_t  rootDir ;
   int         pos             = 0 ;
   int         fatentry        = 0 ;
   int         fatblocksneeded =  (MAXBLOCKS / FATENTRYCOUNT ) ;

   /* prepare block 0 : fill it with '\0',
    * use strcpy() to copy some text to it for test purposes
	* write block 0 to virtual disk
	*/

   // initialize block 0
   block = virtualDisk[0];
   reset_block(block);
   strcpy((char *) block.data, "good evening");
   writeblock(&block, 0) ;

	/* prepare FAT table
	 * write FAT blocks to virtual disk
	 */

	// set all entries of FAT to unused
	for (int i = 1; i < MAXBLOCKS; i++)
	    FAT[i] = UNUSED;

	// initialize the FAT with its own block chain
	// this works with any FAT size
	FAT[0] = ENDOFCHAIN;
    for (int i = 1; i < fatblocksneeded; i++)
    {
       FAT[i] = (fatentry_t) (i + 1);
    }
    FAT[fatblocksneeded] = ENDOFCHAIN;    // FAT table
    FAT[fatblocksneeded +1] = ENDOFCHAIN; // index block

    // copy FAT to VD
    copy_FAT();

    /* prepare root directory
     * write root directory block to virtual disk
     */

    // initialize root directory block
    dirblock_t root_block;
    root_block.isdir = 1;
    root_block.nextEntry = 0;
    memset(&root_block, '\0', BLOCKSIZE);
    // determine index of root directory
    rootDirIndex = (fatentry_t) (fatblocksneeded + 1);
    //write to disk
    writeblock((diskblock_t *) &root_block, rootDirIndex);

}

void copy_FAT()
{
    int fatblocksneeded =  (MAXBLOCKS / FATENTRYCOUNT );
    diskblock_t temporary_block;
    // uses fatblocksneeded VD blocks (default 2)
    // then copies each cell from FAT to VD [default 1024 cells = BLOCKSIZE]
    // BLOCKSIZE also works as an offset ensuring the outer loop doesn't copy the same code on each iteration

    for (int i = 1, index = 0; i < fatblocksneeded + 1; i++){
        for (int j = 0; j < FATENTRYCOUNT; j++, index++)
            temporary_block.fat[j] = FAT[index]; //seg faults lol
        writeblock( &temporary_block, i);
    }
}

void reset_block (diskblock_t block)
{
    for (int i = 0; i < BLOCKSIZE; i++)
        block.data[i] = '\0';
}


/* use this for testing
 */

void printBlock ( int blockIndex )
{
   printf ( "virtualdisk[%d] = %s\n", blockIndex, virtualDisk[blockIndex].data ) ;
}

