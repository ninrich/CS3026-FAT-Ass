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

void read_block(diskblock_t *block, int block_address)
{
    memmove(block->data, virtualDisk[block_address].data, BLOCKSIZE);
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
   reset_block(&block);
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
    currentDirIndex = rootDirIndex;

}

diskblock_t create_empty_block()
{
    diskblock_t new_block;
    reset_block(&new_block);
    return new_block;
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

int get_first_free_block()
{
    for (int i = rootDirIndex + 1; i < MAXBLOCKS; i++)
        if (FAT[i] == UNUSED)
            return i;
    return FALSE;
}

void reset_block (diskblock_t * block)
{
    for (int i = 0; i < BLOCKSIZE; i++)
        block->data[i] = '\0';
}



int myfgetc (MyFILE * stream)
{

    if (stream->blockno == ENDOFCHAIN
    || strcmp(stream->mode, "r") != 0)
        return EOF;

    if (stream->pos % BLOCKSIZE == 0) {
        memcpy(&stream->buffer, &virtualDisk[stream->blockno], BLOCKSIZE);
        stream->blockno = FAT[stream->blockno];

        stream->pos = 0;
    }
    return stream->buffer.data[stream->pos++];
}

void myfputc(int b, MyFILE * stream)
{
    if (strcmp(stream->mode, "r") == 0)
        return;

    if (stream->pos >= BLOCKSIZE) {
       int new_block_index = get_first_free_block();
       FAT[stream->blockno] = (fatentry_t) new_block_index;
       FAT[new_block_index] = ENDOFCHAIN;
       copy_FAT();

       stream->pos = 0;
       writeblock(&stream->buffer, stream->blockno);

       stream->buffer = create_empty_block();
       stream->blockno = (fatentry_t) new_block_index;
    }
    stream->buffer.data[stream->pos] = (Byte) b;
    stream->pos++;

}

MyFILE * myfopen(const char * filename, const char * mode)
{
    if (! (strcmp(mode, "w") == 0 || strcmp(mode, "r") == 0) ) {
        printf("Wrong mode selected.\nUse \"w\" or \"r\".");
        return FALSE;
    }

    int file_location;
    if ( (file_location = get_file_location(filename)) == FALSE ) {
        // file doesn't exist
        if (strcmp(mode, "r") == 0) {
            // read mode file doesn't exist
            printf("Trying to read a nonexisting file");
            exit(1);
        } else {
            // write mode file doesn't exist
            int index = get_first_free_block(); // check if directory full

            // create a new directory entry
            direntry_t * new_dir_entry = calloc(1, sizeof(direntry_t));
            new_dir_entry->unused = FALSE;
            new_dir_entry->isdir = FALSE;
            new_dir_entry->filelength = 0;
            new_dir_entry->firstblock = (fatentry_t) index;

            FAT[index] = ENDOFCHAIN;
            copy_FAT();

            diskblock_t block;
            read_block(&block, currentDirIndex);

            int entry_number = block.dir.nextEntry;
            currentDir = block.dir.entrylist;
            currentDir[entry_number] = *new_dir_entry;
            strcpy(currentDir[entry_number].name, filename);

            block.dir.nextEntry++;
            writeblock(&block, currentDirIndex);


            MyFILE * new_file = malloc(sizeof(MyFILE));
            new_file->pos = 0;
            strcpy(new_file->mode, mode);
            new_file->blockno = (fatentry_t) index;

            free(new_dir_entry);
            return new_file;
        }
    } else {
        // file exists
        if (strcmp(mode, "r") == 0) {
            // read mode file exist
            MyFILE *file = malloc(sizeof(MyFILE));
            memcpy(file->mode, mode, sizeof(char[3]));
            file->pos = 0;
            file->buffer = create_empty_block(); /////////////////////////////////////////////
            file->blockno = (fatentry_t) file_location;
            return file;
        } else {
            // write mode file exists
            if (file_location < BLOCKSIZE/FATENTRYCOUNT + 1) {
                printf("Trying to write to reserver blocks ( storage info or FAT table )");
                exit(1);
            }
            end_chain(file_location);
            copy_FAT();
            clear_directory(filename);
        }

    }
}

void clear_directory(const char * filename)
{
    diskblock_t directory_block;
    fatentry_t next_dir = rootDirIndex;

    while(next_dir != ENDOFCHAIN)
    {
        currentDirIndex = next_dir;
        memmove(&directory_block.data, virtualDisk[next_dir].data, BLOCKSIZE);
        currentDir = directory_block.dir.entrylist;
        for (int i = 0; i < directory_block.dir.nextEntry; i++)
            if (strcmp(currentDir[i].name, filename) == 0)
            {
                currentDir[i].firstblock = (fatentry_t) get_first_free_block();
                currentDir[i].entrylength = sizeof(currentDir);
                return;
            }
        next_dir = FAT[next_dir];
    }
}

void end_chain(index)
{
    int next = FAT[index];
    while (next != ENDOFCHAIN)
        next = FAT[next];

    diskblock_t block = create_empty_block();
    memmove(virtualDisk[index].data, &block.data, BLOCKSIZE);
    FAT[index] = UNUSED;
}

void myfclose(MyFILE * stream)
{
    int next = get_first_free_block();
    FAT[stream->blockno] = (fatentry_t) next;
    FAT[next] = ENDOFCHAIN;
    copy_FAT();
    memmove(virtualDisk[stream->blockno].data, &stream->buffer.data, BLOCKSIZE);
    free(stream);

}
void printBlock ( int blockIndex )
{
   printf ( "virtualdisk[%d] = %s\n", blockIndex, virtualDisk[blockIndex].data ) ;
}

int get_file_location(const char *filename)
{
    diskblock_t dir_block;
    fatentry_t next_dir = rootDirIndex;

    while(next_dir != ENDOFCHAIN)
    {
        currentDirIndex = next_dir;
        memmove(&dir_block.data, virtualDisk[next_dir].data, BLOCKSIZE);
        currentDir = dir_block.dir.entrylist;
        for(int i=0; i < dir_block.dir.nextEntry; i++)
            if(strcmp(currentDir[i].name, filename) == 0)
                return currentDir[i].firstblock;
        next_dir = FAT[next_dir];
    }
    return FALSE;
}