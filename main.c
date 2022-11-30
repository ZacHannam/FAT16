#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include <locale.h>

/// +--------------------------------------------------------------------------------------------------+
/// |                                                                                                  |
/// |                                      Useful Information                                          |
/// |                                                                                                  |
/// +--------------------------------------------------------------------------------------------------+

// Size of each sector = BPB_BytsPerSec  512 bytes

// Each Cluster is 4 Sectors

// RESERVED SECTORS
// = bootSector->BPB_RsvdSecCnt ~ 4 "Boot Sector [62 bytes] ..."                                0 - 4

// FAT AREA
//  - FAT 1: bootSector->BPB_RsvdSecCnt + (fN(0) * BPB_FATSz16)"    -> fN(1)                    4 - 36
//  - FAT 2: bootSector->BPB_RsvdSecCnt + (fN(1) * BPB_FATSz16)"    -> fN(0)                    36 - 68

// ROOT DIRECTORY
// = (bootSector->BPB_RootEntCnt * sizeof(Entry)) / BPB_BytsPerSec  size: 32sec                 68 - 100

// DATA SECTOR
// = 100 ->

/// +--------------------------------------------------------------------------------------------------+
/// |                                                                                                  |
/// |                                           Utilities                                              |
/// |                                                                                                  |
/// +--------------------------------------------------------------------------------------------------+

/**
 * Prints a number of spaces in the console
 * @param paramIndentSize
 */
void printIndent(int paramIndentSize) {

    for(int index = 0; index < paramIndentSize; index++) {
        printf(" ");
    }
}


/// +--------------------------------------------------------------------------------------------------+
/// |                                                                                                  |
/// |                                          Exceptions                                              |
/// |                                                                                                  |
/// +--------------------------------------------------------------------------------------------------+

/*
 * Exceptions are created then attached to a return stack, when a return stack is returned it may contain
 * an exception which can be checked.
 */

#define EXCEPTION_UNABLE_TO_OPEN_FILE 1
#define EXCEPTION_CLUSTER_OUT_OF_RANGE 2

/**
 * Stores the id of a singular exception
 */
struct Exception {
    int exception;              // The exception id
}; typedef struct Exception Exception;

/**
 * Create an exception
 * @param paramException - int value of the exception.
 * @return               - the created exception
 */
Exception *createException(int paramException) {
    Exception *exception = (Exception *) malloc(sizeof(Exception));
    exception->exception = paramException;

    return exception;
}


/// +--------------------------------------------------------------------------------------------------+
/// |                                                                                                  |
/// |                                       Return Stacks                                              |
/// |                                                                                                  |
/// +--------------------------------------------------------------------------------------------------+

/*
 * Return stacks bundle up the value to be returned in a pointer and any exceptions that occurred.
 */

/**
 * The return stack stores a value to be returned and any exceptions that have occurred
 */
struct ReturnStack {
    int *returnedValue;                 // The returned value
    Exception exceptions[64];           // Exceptions within the return stack
    int numberOfExceptions;             // Total number of exceptions

}; typedef struct ReturnStack ReturnStack;

/**
 *  Prints the errors contained within a return stack
 * @param paramReturnStack  - the return stack in which the errors are
 */
void printExceptionsOnReturnStack(ReturnStack *paramReturnStack) {

    if(paramReturnStack->numberOfExceptions == 0) {
        return;
    }

    for(int index = 0; index < paramReturnStack->numberOfExceptions; index++) {

        switch ((int) ((paramReturnStack->exceptions)+index)->exception) {

            case EXCEPTION_UNABLE_TO_OPEN_FILE:
                printf("Unable to open file.\n");
                break;
            case EXCEPTION_CLUSTER_OUT_OF_RANGE:
                printf("Cluster index out of range for cluster.\n");
                break;
            default:
                printf("Unknown exception occurred.\n");
                break;
        }
    }
}

/**
 * Create a return stack
 * @return - The newly instantiated Return Stack
 */
ReturnStack *createReturnStack() {
    ReturnStack *returnStack = (ReturnStack *) malloc(sizeof(ReturnStack));
    returnStack->numberOfExceptions = 0;

    return returnStack;
}

/**
 * Sets the value of a Return Stack
 * @param paramReturnStack - the return stack that's value is being set
 * @param returnValue      - the value which is being set
 */
void setReturnValueToReturnStack(ReturnStack *paramReturnStack, int *returnValue) {
    paramReturnStack->returnedValue = returnValue;
}

/**
 * Adds an exception to the return stack
 * @param paramReturnStack - the return stack that's exception is being added
 * @param paramException   - the exception which is being added
 */
void addExceptionToReturnStack(ReturnStack *paramReturnStack, Exception *paramException) {
    if((void *) paramReturnStack->numberOfExceptions == NULL) {
        paramReturnStack->numberOfExceptions = 0;
    }

    paramReturnStack->exceptions[paramReturnStack->numberOfExceptions] = *paramException;
    paramReturnStack->numberOfExceptions++;
}


/// +--------------------------------------------------------------------------------------------------+
/// |                                                                                                  |
/// |                                             Buffers                                              |
/// |                                                                                                  |
/// +--------------------------------------------------------------------------------------------------+

/*
 * Buffers store an array of bytes as unsigned chars. Buffers can be any length
 */

/**
 * Buffer stores a buffer pointer which points to the first element in the buffer and the total size of the buffer
 */
struct Buffer {
    int size;
    unsigned char *bufferPtr;
}; typedef struct Buffer Buffer;

/**
 * Prints a buffer
 * @param paramBuffer       - Buffer to be printed
 * @param paramValuesPerRow - The number of hex values per row
 */
void printBuffer(Buffer *paramBuffer, int paramValuesPerRow) {

    printf("Buffer Pointer: %d\nBuffer Size: %d\n\n          ", paramBuffer->bufferPtr, paramBuffer->size);

    for(int index = 0; index < paramValuesPerRow; index++) {
        printf("%02x ", index);
    }

    printf("\n");

    for(int index = 0; index < paramBuffer->size; index++) {

        if(index % paramValuesPerRow == 0) {
            printf("\n%8x  ", index);
        }

        printf("%02x ", *(paramBuffer->bufferPtr + index));

    }
}

/**
 * Prints a buffer as ASCII text
 * @param paramBuffer       - Buffer to be printed
 * @param paramIndentSize   - Number of spaces before each line
 */
void printBufferAsASCII(Buffer *paramBuffer, int paramIndentSize) {

    printIndent(paramIndentSize);

    for(int index = 0; index < paramBuffer->size; index++) {
        printf("%c", *(paramBuffer->bufferPtr + index));

        if( (char) *(paramBuffer->bufferPtr + index) == '\n') {
            printIndent(paramIndentSize);
        }

    }
}

/**
 * Used to create a Buffer
 * @param paramSize - Number of bytes a buffer holds
 * @return          - The created buffer
 */
Buffer *createBuffer(int paramSize) {

    Buffer *buffer = (Buffer *) malloc(sizeof(Buffer));
    buffer->size = paramSize;
    buffer->bufferPtr = (char *)calloc(buffer->size, sizeof(char)); // Enough memory for the file

    return buffer;
}

/**
 * Sub buffers another buffer
 * @param paramBuffer - Buffer to sub-buffered
 * @param paramStart  - Where the new buffer starts
 * @param paramReadLength - The length of the new buffer in bytes
 * @return
 */
ReturnStack *getBytesFromByteStream(Buffer *paramBuffer, int paramStart, int paramReadLength) {

    ReturnStack *returnStack = createReturnStack();

    Buffer *buffer = createBuffer(paramReadLength);

    memcpy(buffer->bufferPtr, (paramBuffer->bufferPtr + paramStart),buffer->size);

    setReturnValueToReturnStack(returnStack, buffer);

    return returnStack;
}

/**
 * Converts a file into a buffer
 * @param paramFile - File to be turned into a buffer
 * @return          - The new buffer containing the binary of the file
 */
ReturnStack *convertFileToBuffer(FILE *paramFile) {

    ReturnStack *returnStack = createReturnStack();

    fseek(paramFile, 0, SEEK_END);
    long length = ftell(paramFile);

    fseek(paramFile, 0L, SEEK_SET);

    Buffer *buffer = createBuffer(length);

    fread(buffer->bufferPtr, sizeof(char), length, paramFile);

    setReturnValueToReturnStack(returnStack, buffer);

    return returnStack;
}


/// +--------------------------------------------------------------------------------------------------+
/// |                                                                                                  |
/// |                                        Linked Lists                                              |
/// |                                                                                                  |
/// +--------------------------------------------------------------------------------------------------+

/*
 * Linked lists are primarily used in this application to store directory entries
 */

/**
 * Linked lists store the value of the current list, and a pointer to the next list
 */
struct LinkedList {

    unsigned int *pointer;
    struct LinkedList *next;

}; typedef struct LinkedList LinkedList;

/**
 * Creates a linked list with null values
 * @return - returns the created linked list
 */
LinkedList *createLinkedList() {
    LinkedList *linkedList = (LinkedList *) malloc(sizeof(LinkedList));
    linkedList->pointer = NULL;
    return linkedList;
}

/**
 * Adds a new value to a linked list
 * @param paramLinkedList   - The linked list the value is being added to
 * @param paramPointer      - Pointer to the value being stored
 * @return
 */
LinkedList *addNewLink(LinkedList *paramLinkedList, int *paramPointer) {
    while(paramLinkedList->next != NULL) {
        paramLinkedList = paramLinkedList->next;
    }
    LinkedList *linkedList = createLinkedList();
    linkedList->pointer = paramPointer;

    paramLinkedList->next = linkedList;

    return linkedList;
}

/**
 * Recursively frees memory within a linked list
 * @param paramLinkedList - The linked list to be freed from memory
 */
void freeLinkedList(LinkedList *paramLinkedList) {
    if(paramLinkedList->next != NULL) {
        freeLinkedList(paramLinkedList->next);
    }
    free(paramLinkedList);
}


/// +--------------------------------------------------------------------------------------------------+
/// |                                                                                                  |
/// |                                               Files                                              |
/// |                                                                                                  |
/// +--------------------------------------------------------------------------------------------------+

/*
 * Files in this application are the fat16 image files. They must be opened and closed to see whats inside
 */

/**
 * Opens and returns a file from the file name
 * @param paramFileName - the name / directory of the file to be opened
 * @return              - returns the file
 */
ReturnStack *openFile(char *paramFileName) {

    ReturnStack *returnStack = createReturnStack();

    // OPEN FILE
    FILE *file;
    file = fopen(paramFileName, "r");

    if (file == NULL)
    {
        addExceptionToReturnStack(returnStack, createException(EXCEPTION_UNABLE_TO_OPEN_FILE));
        return returnStack;
    }

    setReturnValueToReturnStack(returnStack, (int *) file);

    return returnStack;
}

/**
 * Closes the given file.
 * @param paramFile - The file to be closed
 */
void closeFile(FILE *paramFile) {
    fclose(paramFile);
}


/// +--------------------------------------------------------------------------------------------------+
/// |                                                                                                  |
/// |                                         Boot Sector                                              |
/// |                                                                                                  |
/// +--------------------------------------------------------------------------------------------------+

/*
 * The first sector in a FAT16 image is the Reserved / Boot Sector
 *
 * The reserved sector starts at byte 0 and ends at the reserved sector count.
 * The Boot Sector occupies the first 62 bytes of the served sector
 *
 */

/**
 * All Boot Sector properties, imported from the SCC.211 FAT16 Exercise.pdf file
 */
struct __attribute__((__packed__)) BootSector {
    uint8_t	    BS_jmpBoot[ 3 ];		// x86 jump instr. to boot code             0-2
    uint8_t		BS_OEMName[ 8 ];		// What created the filesystem              3-10
    uint16_t	BPB_BytsPerSec;			// Bytes per Sector                         11-12
    uint8_t		BPB_SecPerClus;			// Sectors per Cluster                      13
    uint16_t	BPB_RsvdSecCnt;			// Reserved Sector Count                    14-15
    uint8_t		BPB_NumFATs;			// Number of copies of FAT                  16
    uint16_t	BPB_RootEntCnt;			// FAT12/FAT16: size of root DIR            17-18
    uint16_t	BPB_TotSec16;			// Sectors, may be 0, see below             19-20
    uint8_t		BPB_Media;			    // Media type, e.g. fixed                   21
    uint16_t	BPB_FATSz16;			// Sectors in FAT (FAT12 or FAT16)          22-23
    uint16_t	BPB_SecPerTrk;			// Sectors per Track                        24-25
    uint16_t	BPB_NumHeads;			// Number of heads in disk                  26-27
    uint32_t	BPB_HiddSec;			// Hidden Sector count                      28-31
    uint32_t	BPB_TotSec32;			// Sectors if BPB_TotSec16 == 0             32-35
    uint8_t		BS_DrvNum;			    // 0 = floppy, 0x80 = hard disk             36
    uint8_t		BS_Reserved1;			//                                          37
    uint8_t		BS_BootSig;			    // Should = 0x29                            38
    uint32_t	BS_VolID;			    // 'Unique' ID for volume                   39-42
    uint8_t		BS_VolLab[ 11 ];		// Non zero terminated string               43-53
    uint8_t		BS_FilSysType[ 8 ];		// e.g. 'FAT16 ' (Not 0 term.)              54-61
}; typedef struct BootSector BootSector;

/**
 * Prints a boot sector
 * @param paramBootSector - The boot sector to be printed
 */
void printBootSector(BootSector *paramBootSector) {
    for(int index=0; index<3; index++) {
        printf("BS_jmpBoot[%d]= %d | %08x\n", index, paramBootSector->BS_jmpBoot[index], paramBootSector->BS_jmpBoot[index]);
    }
    for(int index=0; index<8; index++) {
        printf("BS_OEMName[%d]= %d | %08x\n", index, paramBootSector->BS_OEMName[index], paramBootSector->BS_OEMName[index]);
    }
    printf("BPB_BytsPerSec= %d | %08x\n", paramBootSector->BPB_BytsPerSec, paramBootSector->BPB_BytsPerSec);
    printf("BPB_SecPerClus= %d | %08x\n", paramBootSector->BPB_SecPerClus, paramBootSector->BPB_SecPerClus);
    printf("BPB_RsvdSecCnt= %d | %08x\n", paramBootSector->BPB_RsvdSecCnt, paramBootSector->BPB_RsvdSecCnt);
    printf("BPB_NumFATs= %d | %08x\n", paramBootSector->BPB_NumFATs, paramBootSector->BPB_NumFATs);
    printf("BPB_RootEntCnt= %d | %08x\n", paramBootSector->BPB_RootEntCnt, paramBootSector->BPB_RootEntCnt);
    printf("BPB_TotSec16= %d | %08x\n", paramBootSector->BPB_TotSec16, paramBootSector->BPB_TotSec16);
    printf("BPB_Media= %d | %08x\n", paramBootSector->BPB_Media, paramBootSector->BPB_Media);
    printf("BPB_FATSz16= %d | %08x\n", paramBootSector->BPB_FATSz16, paramBootSector->BPB_FATSz16);
    printf("BPB_SecPerTrk= %d | %08x\n", paramBootSector->BPB_SecPerTrk, paramBootSector->BPB_SecPerTrk);
    printf("BPB_NumHeads= %d | %08x\n", paramBootSector->BPB_NumHeads, paramBootSector->BPB_NumHeads);
    printf("BPB_HiddSec= %d | %08x\n", paramBootSector->BPB_HiddSec, paramBootSector->BPB_HiddSec);
    printf("BPB_TotSec32= %d | %08x\n", paramBootSector->BPB_TotSec32, paramBootSector->BPB_TotSec32);
    printf("BS_DrvNum= %d | %08x\n", paramBootSector->BS_DrvNum, paramBootSector->BS_DrvNum);
    printf("BS_Reserved1= %d | %08x\n", paramBootSector->BS_Reserved1, paramBootSector->BS_Reserved1);
    printf("BS_BootSig= %d | %08x\n", paramBootSector->BS_BootSig, paramBootSector->BS_BootSig);
    printf("BS_VolID= %d | %08x\n", paramBootSector->BS_VolID, paramBootSector->BS_VolID);
    for(int index=0; index<11; index++) {
        printf("BS_VolLab[%d]= %d | %08x\n", index, paramBootSector->BS_VolLab[index], paramBootSector->BS_VolLab[index]);
    }
    for(int index=0; index<8; index++) {
        printf("BS_FilSysType[%d]= %d | %08x\n", index, paramBootSector->BS_FilSysType[index], paramBootSector->BS_FilSysType[index]);
    }
}

/**
 * Turns a the first 62 bytes of a buffer into a boot sector
 * @param paramBuffer
 * @return
 */
ReturnStack *createBootSector(Buffer *paramBuffer) {

    ReturnStack *returnStack = createReturnStack();

    ReturnStack *selectedElements = getBytesFromByteStream(paramBuffer, 0,  sizeof(BootSector));

    BootSector *bootSector = (BootSector *) malloc(sizeof(BootSector));
    memcpy(bootSector, ((Buffer *) selectedElements->returnedValue)->bufferPtr, sizeof(BootSector));

    returnStack->returnedValue = bootSector;

    return returnStack;
}

/// +--------------------------------------------------------------------------------------------------+
/// |                                                                                                  |
/// |                                                FATS                                              |
/// |                                                                                                  |
/// +--------------------------------------------------------------------------------------------------+

/*
 * The File allocation tables are stored after the reserved sector, there are usually 2.
 * This reads the first FAT table and ignores the second as it is redundant
 *
 * Starts after the reserved sectors and carries on for the length of the fat table
 */

/**
 * Returns the next cluster in the list of linked clusters from the FAT
 * @param paramBootSector    - Boot sector of the FAT16 image
 * @param paramBuffer        - Buffer containing the file
 * @param paramClusterNumber - The cluster entry number
 * @return
 */
ReturnStack *getNextClusterFromFat(BootSector *paramBootSector, Buffer *paramBuffer, int paramClusterNumber) {

    const int FAT_ENTRY_SIZE = 2;
    const int FAT_TABLE_START = paramBootSector->BPB_RsvdSecCnt;

    ReturnStack *returnStack = createReturnStack();

    int numberOfClustersInFat = ((paramBootSector->BPB_FATSz16 * paramBootSector->BPB_BytsPerSec) / FAT_ENTRY_SIZE);

    if(paramClusterNumber > numberOfClustersInFat) {
        addExceptionToReturnStack(returnStack, EXCEPTION_CLUSTER_OUT_OF_RANGE);
        return returnStack;
    }

    Buffer *buffer = getBytesFromByteStream(paramBuffer, (FAT_TABLE_START * paramBootSector->BPB_BytsPerSec) + ((FAT_ENTRY_SIZE) * paramClusterNumber), FAT_ENTRY_SIZE)->returnedValue;

    uint16_t clusterNumber = (256 * (buffer->bufferPtr[1])) + *buffer->bufferPtr;

    setReturnValueToReturnStack(returnStack, clusterNumber);

    return returnStack;
}

/**
 * Gets the number of clusters in the linked list before reaching 0xFF
 * @param paramBootSector    - Boot sector of the FAT16 image
 * @param paramBuffer        - Buffer containing the file
 * @param paramStartCluster  - The first cluster being search
 * @return                   - The number of clusters in the sequence (Will always be 1 or larger)
 */
ReturnStack *getNumberOfClustersInSequence(BootSector *paramBootSector, Buffer *paramBuffer, int paramStartCluster) {

    ReturnStack *returnStack = createReturnStack();

    int currentCluster = paramStartCluster;
    int numberOfClusters = 0;

    while(currentCluster != 0xFFFF) {
        numberOfClusters++;
        currentCluster = getNextClusterFromFat(paramBootSector, paramBuffer, currentCluster)->returnedValue;
    }

    setReturnValueToReturnStack(returnStack, numberOfClusters);

    return returnStack;
}

/// +--------------------------------------------------------------------------------------------------+
/// |                                                                                                  |
/// |                                           Directory                                              |
/// |                                                                                                  |
/// +--------------------------------------------------------------------------------------------------+

/*
 * Directories are the folders within fat image.
 * Each entry is either a folder or file within the directory
 */

/**
 * An entry is a 32 byte entry from a directory, this holds all of the standard values but not longer names
 */
struct __attribute__((__packed__)) Entry {
    uint8_t         DIR_Name[ 11 ];         // Non zero terminated string
    uint8_t         DIR_Attr;               // File attributes
    uint8_t         DIR_NTRes;              // Used by Windows NT, ignore
    uint8_t         DIR_CrtTimeTenth;       // Tenths of sec. 0...199
    uint16_t        DIR_CrtTime;            // Creation Time in 2s intervals
    uint16_t        DIR_CrtDate;            // Date file created
    uint16_t        DIR_LstAccDate;         // Date of last read or write
    uint16_t        DIR_FstClusHI;          // Top 16 bits file's 1st cluster
    uint16_t        DIR_WrtTime;            // Time of last write
    uint16_t        DIR_WrtDate;            // Date of last write
    uint16_t        DIR_FstClusLO;          // Lower 16 bits file's 1st cluster
    uint32_t        DIR_FileSize;           // File size in bytes
}; typedef struct Entry Entry;

/**
 * Print values stored within an entry
 * @param paramEntry
 */
void printEntry(Entry *paramEntry) {
    for(int index=0; index<11; index++) {
        printf("DIR_Name[%d]= %d | %08x\n", index, paramEntry->DIR_Name[index], paramEntry->DIR_Name[index]);
    }
    printf("DIR_Attr= %d | %08x\n", paramEntry->DIR_Attr, paramEntry->DIR_Attr);
    printf("DIR_NTRes= %d | %08x\n", paramEntry->DIR_NTRes, paramEntry->DIR_NTRes);
    printf("DIR_CrtTimeTenth= %d | %08x\n", paramEntry->DIR_CrtTimeTenth, paramEntry->DIR_CrtTimeTenth);
    printf("DIR_CrtTime= %d | %08x\n", paramEntry->DIR_CrtTime, paramEntry->DIR_CrtTime);
    printf("DIR_CrtDate= %d | %08x\n", paramEntry->DIR_CrtDate, paramEntry->DIR_CrtDate);
    printf("DIR_LstAccDate= %d | %08x\n", paramEntry->DIR_LstAccDate, paramEntry->DIR_LstAccDate);
    printf("DIR_FstClusHI= %d | %08x\n", paramEntry->DIR_FstClusHI, paramEntry->DIR_FstClusHI);
    printf("DIR_WrtTime= %d | %08x\n", paramEntry->DIR_WrtTime, paramEntry->DIR_WrtTime);
    printf("DIR_WrtDate= %d | %08x\n", paramEntry->DIR_WrtDate, paramEntry->DIR_WrtDate);
    printf("DIR_FstClusLO= %d | %08x\n", paramEntry->DIR_FstClusLO, paramEntry->DIR_FstClusLO);
    printf("DIR_FileSize= %d | %08x\n", paramEntry->DIR_FileSize, paramEntry->DIR_FileSize);
}

/**
 * Creates an entry from a buffer and a start location
 * @param paramBuffer       - The buffer for the entry to be copied from
 * @param paramEntryStart   - The first byte of the entry in the buffer
 * @return
 */
ReturnStack *createEntry(Buffer *paramBuffer, int paramEntryStart) {

    ReturnStack *returnStack = createReturnStack();

    ReturnStack *selectedElements = getBytesFromByteStream(paramBuffer, paramEntryStart,  sizeof(Entry));

    Entry *entry = (Entry *) malloc(sizeof(Entry));
    memcpy(entry, ((Buffer *) selectedElements->returnedValue)->bufferPtr, sizeof(Entry));

    returnStack->returnedValue = entry;

    return returnStack;
}

/**
 * Entry attributes makes the values within the Entry-Attribute value accessible
 */
struct __attribute__((__packed__)) EntryAttributes {
    uint8_t archive;
    uint8_t directory;
    uint8_t volume_name;
    uint8_t system;
    uint8_t hidden;
    uint8_t read_only;

    uint8_t is_file;
    uint8_t is_valid;

}; typedef struct EntryAttributes EntryAttributes;

/**
 * Prints the entries attribute
 * @param paramEntryAttributes - Attributes to be printed
 */
void printEntryAttributes(EntryAttributes *paramEntryAttributes) {
    printf("archive= %d\n", paramEntryAttributes->archive);
    printf("directory= %d\n", paramEntryAttributes->directory);
    printf("volume_name= %d\n", paramEntryAttributes->volume_name);
    printf("system= %d\n", paramEntryAttributes->system);
    printf("hidden= %d\n", paramEntryAttributes->hidden);
    printf("read_only= %d\n", paramEntryAttributes->read_only);
    printf("is_file= %d\n", paramEntryAttributes->is_file);
    printf("is_valid= %d\n", paramEntryAttributes->is_valid);
}

/**
 * Creates the entries attributes
 * @param paramEntry - Entry for the entry attributes to be based off
 * @return           - Returns the entry attributes
 */
ReturnStack *createEntryAttributes(Entry *paramEntry) {

    ReturnStack *returnStack = createReturnStack();

    EntryAttributes *entryAttributes = (EntryAttributes *) malloc(sizeof(EntryAttributes));
    if((paramEntry->DIR_Attr & 0x01) == 0x01) {
        entryAttributes->read_only = 1;
    } else {
        entryAttributes->read_only = 0;
    }

    if((paramEntry->DIR_Attr & 0x02) == 0x02) {
        entryAttributes->hidden = 1;
    } else {
        entryAttributes->hidden = 0;
    }

    if((paramEntry->DIR_Attr & 0x04) == 0x04) {
        entryAttributes->system = 1;
    } else {
        entryAttributes->system = 0;
    }

    if((paramEntry->DIR_Attr & 0x08) == 0x08) {
        entryAttributes->volume_name = 1;
    } else {
        entryAttributes->volume_name = 0;
    }

    if((paramEntry->DIR_Attr & 0x10) == 0x10) {
        entryAttributes->directory = 1;
    } else {
        entryAttributes->directory = 0;
    }

    if((paramEntry->DIR_Attr & 0x20) == 0x20) {
        entryAttributes->archive = 1;
    } else {
        entryAttributes->archive = 0;
    }

    if(!entryAttributes->directory && !entryAttributes->volume_name) {
        entryAttributes->is_file = 1;
    } else {
        entryAttributes->is_file = 0;
    }

    if(entryAttributes->directory || entryAttributes->volume_name || entryAttributes->is_file) {
        entryAttributes->is_valid = 1;
    } else {
        entryAttributes->is_valid = 0;
    }

    setReturnValueToReturnStack(returnStack, entryAttributes);

    return returnStack;
} // CLEAN UP

/**
 * A Long File Name is a 32 byte array that contains values used in a Long File Name entry in the directory
 */
struct __attribute__((__packed__)) LongFileNameEntry {
    uint8_t         LDIR_Ord;               // Order/ position in sequence/ set
    uint8_t         LDIR_Name1[ 10 ];       // First 5 UNICODE characters
    uint8_t         LDIR_Attr;              // = ATTR_LONG_NAME (xx001111)
    uint8_t         LDIR_Type;              // Should = 0
    uint8_t         LDIR_Chksum;            // Checksum of short name
    uint8_t         LDIR_Name2[ 12 ];       // Middle 6 UNICODE characters
    uint16_t        LDIR_FstClusLO;         // MUST be zero
    uint8_t         LDIR_Name3[ 4 ];        // Last 2 UNICODE characters
}; typedef struct LongFileNameEntry LongFileNameEntry;

/**
 * Creates a long file name entry from a buffer
 * @param paramBuffer       - Buffer holding the binary entry of the long file name
 * @param paramEntryStart   - The first byte of the long file name entry
 * @return
 */
ReturnStack *createLongFileNameEntry(Buffer *paramBuffer, int paramEntryStart) {

    ReturnStack *returnStack = createReturnStack();

    ReturnStack *selectedElements = getBytesFromByteStream(paramBuffer, paramEntryStart,  sizeof(LongFileNameEntry));

    LongFileNameEntry *longFileNameEntry = (LongFileNameEntry *) malloc(sizeof(LongFileNameEntry));
    memcpy(longFileNameEntry, ((Buffer *) selectedElements->returnedValue)->bufferPtr, sizeof(LongFileNameEntry));

    returnStack->returnedValue = longFileNameEntry;

    return returnStack;
}

/**
 * A directory entry stores the filename long or short of the file / directory and the entry
 */
struct __attribute__((__packed__)) DirectoryEntry {
    Entry *entry;
    EntryAttributes *entryAttributes;
    wchar_t *longFileName;
    int fileNameSize;

}; typedef struct DirectoryEntry DirectoryEntry;

/**
 * Prints the long file name from the directory entry
 * @param paramDirectoryEntry - The entry containing the file name to be printed
 * @param paramIndentSize     - The ident size
 */
void printLongFileName(DirectoryEntry *paramDirectoryEntry, int paramIndentSize) {

    printIndent(paramIndentSize);

    for(int index = 0; index < paramDirectoryEntry->fileNameSize; index++) {
        printf("%c", paramDirectoryEntry->longFileName[index]);
    }
    printf("\n");
}

/**
 * Creates a directory entry where all values are null
 * @return the new directory entry
 */
ReturnStack *createDirectoryEntry() {

    ReturnStack *returnStack = createReturnStack();

    DirectoryEntry *entry = (DirectoryEntry *) malloc(sizeof(DirectoryEntry));

    setReturnValueToReturnStack(returnStack, entry);

    return returnStack;

}

/**
 * Get all of the directory entries within a buffer / stops at 0x00
 * @param paramBuffer       - The buffer the entries are being generated from
 * @param paramStartingByte - The starting byte for reading the buffer
 * @return                  - A linked list containing all of the entries
 */
ReturnStack *getAllEntriesFromDirectory(Buffer *paramBuffer, int paramStartingByte) {

    ReturnStack *returnStack = createReturnStack();
    LinkedList *entries = createLinkedList();

    int startingByte = paramStartingByte;

    int longFileNameEntryCount = 0;
    while(paramBuffer->bufferPtr[startingByte] != 0x00) {


        if(paramBuffer->bufferPtr[startingByte] == 0xe5) {
            startingByte += sizeof(Entry);
            continue;
        }

        if(paramBuffer->bufferPtr[startingByte] == 0x2e) {                                          // File begins with "." meaning should not be read
            startingByte += sizeof(Entry);
            continue;
        }

        if(paramBuffer->bufferPtr[startingByte + 11] == 0x0f) {                                     // Long File Name Entry
            longFileNameEntryCount += 1;
        } else {

            Entry *entry = createEntry(paramBuffer, startingByte)->returnedValue;

            EntryAttributes *entryAttributes = createEntryAttributes(entry)->returnedValue;
            DirectoryEntry *directoryEntry = createDirectoryEntry()->returnedValue;

            directoryEntry->entry = entry;
            directoryEntry->entryAttributes = entryAttributes;

            if(longFileNameEntryCount) {
                int startingMemoryAddress = startingByte - sizeof(LongFileNameEntry);

                wchar_t characters[longFileNameEntryCount * 13];
                int nextCharacterPosition = 0;

                for(int index = longFileNameEntryCount; index > 0; index--) {

                    LongFileNameEntry *longFileNameEntry = createLongFileNameEntry(paramBuffer, startingMemoryAddress)->returnedValue;

                    for(int cIndex = 0; cIndex < 10; cIndex+=2) {
                        characters[nextCharacterPosition] = (wchar_t) (longFileNameEntry->LDIR_Name1[cIndex] + longFileNameEntry->LDIR_Name1[cIndex+1] * 256);
                        nextCharacterPosition++;
                    }

                    for(int cIndex = 0; cIndex < 12; cIndex+=2) {
                        characters[nextCharacterPosition] = (wchar_t) (longFileNameEntry->LDIR_Name2[cIndex] + longFileNameEntry->LDIR_Name2[cIndex+1] * 256);
                        nextCharacterPosition++;
                    }

                    for(int cIndex = 0; cIndex < 4; cIndex+=2) {
                        characters[nextCharacterPosition] = (wchar_t) (longFileNameEntry->LDIR_Name3[cIndex] + longFileNameEntry->LDIR_Name3[cIndex+1] * 256);
                        nextCharacterPosition++;
                    }

                    startingMemoryAddress -= sizeof(LongFileNameEntry);
                }

                int totalNameSize = 0;
                for(int index = 0; index < longFileNameEntryCount * 13; index++) {
                    if(characters[index] == 0x0000) {
                        break;
                    }
                    totalNameSize++;
                }

                wchar_t *entryName = (wchar_t *) malloc(sizeof(wchar_t) * totalNameSize);

                for(int index = 0; index < totalNameSize; index++) {
                    entryName[index] = characters[index];
                }

                directoryEntry->longFileName = entryName;
                directoryEntry->fileNameSize = totalNameSize;

                longFileNameEntryCount = 0;

            } else {

                wchar_t *entryName = (wchar_t *) malloc(sizeof(wchar_t) * 11);

                for(int index = 0; index < 11; index++) {
                    entryName[index] = (wchar_t) entry->DIR_Name[index];
                }

                directoryEntry->longFileName = entryName;
                directoryEntry->fileNameSize = 11;

            }

            addNewLink(entries, directoryEntry);

        }
        startingByte += sizeof(Entry);
    }

    setReturnValueToReturnStack(returnStack, entries);

    return returnStack;
}

/**
 * Get the cluster number from the entry
 * @param paramDirectoryEntry - The directory being used
 * @return                    - uint16 value containing the first cluster number
 */
ReturnStack *getClusterNFromDirectoryEntry(DirectoryEntry *paramDirectoryEntry) {

    ReturnStack *returnStack = createReturnStack();

    uint16_t clusterN = paramDirectoryEntry->entry->DIR_FstClusHI * 256 + paramDirectoryEntry->entry->DIR_FstClusLO;

    setReturnValueToReturnStack(returnStack, clusterN);

    return returnStack;
}


/// +--------------------------------------------------------------------------------------------------+
/// |                                                                                                  |
/// |                                      Root Directory                                              |
/// |                                                                                                  |
/// +--------------------------------------------------------------------------------------------------+

/*
 * The root directory is a standard directory that is common in all fat images
 * It is located after the FAT tables.
 */

/**
 * Gets all the entries from the root directory
 * @param paramBootSector   - The boot sector of the fat image
 * @param paramBuffer       - The buffer for the entire file
 * @return                  - A linked list containing all of the entries in the root directory
 */
ReturnStack *getAllEntriesFromRootDirectory(BootSector *paramBootSector, Buffer *paramBuffer) {

    const int SECTOR_ROOT_DIRECTORY_START = paramBootSector->BPB_RsvdSecCnt + (paramBootSector->BPB_FATSz16 * paramBootSector->BPB_NumFATs);
    unsigned int startingByte = SECTOR_ROOT_DIRECTORY_START; startingByte *= paramBootSector->BPB_BytsPerSec;

    return getAllEntriesFromDirectory(paramBuffer, startingByte);

}


/// +--------------------------------------------------------------------------------------------------+
/// |                                                                                                  |
/// |                                                Tree                                              |
/// |                                                                                                  |
/// +--------------------------------------------------------------------------------------------------+

void tree(BootSector *paramBootSector, Buffer *paramBuffer, LinkedList *paramEntries) {

    const int SECTOR_DATA_START = paramBootSector->BPB_RsvdSecCnt + (paramBootSector->BPB_NumFATs * paramBootSector->BPB_FATSz16) + (paramBootSector->BPB_RootEntCnt * 32) / paramBootSector->BPB_BytsPerSec;
    const int BYTES_PER_CLUSTER = paramBootSector->BPB_SecPerClus * paramBootSector->BPB_BytsPerSec;

    LinkedList *entry = paramEntries;

    while(entry->next != NULL) {
        entry = entry->next;

        DirectoryEntry *directoryEntry = entry->pointer;

        if(directoryEntry->entryAttributes->volume_name) {
            printf("Volume: ");
            printLongFileName(directoryEntry, 0);
            continue;
        }

        printLongFileName(directoryEntry, 0);

        int currentCluster = getClusterNFromDirectoryEntry(directoryEntry)->returnedValue;
        int numberOfClusters = getNumberOfClustersInSequence(paramBootSector, paramBuffer, currentCluster)->returnedValue;
        Buffer *clusterData[numberOfClusters];

        for(int clusterIndex = 0; clusterIndex < numberOfClusters; clusterIndex++) {

            int startSector = ((currentCluster - 2) * paramBootSector->BPB_SecPerClus) + SECTOR_DATA_START;
            int startSectorBytes = paramBootSector->BPB_BytsPerSec * startSector;

            Buffer *buffer;
            if(clusterIndex == numberOfClusters - 1 && directoryEntry->entryAttributes->is_file) {
                buffer = getBytesFromByteStream(paramBuffer, startSectorBytes,(directoryEntry->entry->DIR_FileSize - (BYTES_PER_CLUSTER * (numberOfClusters - 1))))->returnedValue;
            } else {
                buffer = getBytesFromByteStream(paramBuffer, startSectorBytes, BYTES_PER_CLUSTER)->returnedValue;
                currentCluster = getNextClusterFromFat(paramBootSector, paramBuffer, currentCluster)->returnedValue;
            }
            clusterData[clusterIndex] = buffer;

        }

        if(directoryEntry->entryAttributes->is_file) {

            Buffer *fileBuffer = createBuffer(directoryEntry->entry->DIR_FileSize);

            for(int index = 0; index < numberOfClusters; index++) {
                memcpy(fileBuffer->bufferPtr + (index * paramBootSector->BPB_BytsPerSec *paramBootSector->BPB_SecPerClus),
                       clusterData[index]->bufferPtr,
                       paramBootSector->BPB_BytsPerSec * paramBootSector->BPB_SecPerClus);



            }
            printBufferAsASCII(fileBuffer, 0);
        }

        if(directoryEntry->entryAttributes->directory) {

            for(int index = 0; index < numberOfClusters; index++) {
                LinkedList *directory = getAllEntriesFromDirectory(clusterData[index], 0)->returnedValue;
                tree(paramBootSector, paramBuffer, directory);
            }
        }
    }
}

void printTree(BootSector *paramBootSector, Buffer *paramBuffer) {

    LinkedList *rootDirectoryEntries = getAllEntriesFromRootDirectory(paramBootSector, paramBuffer)->returnedValue;
    tree(paramBootSector, paramBuffer, rootDirectoryEntries);

}

/// +--------------------------------------------------------------------------------------------------+
/// |                                                                                                  |
/// |                                                Main                                              |
/// |                                                                                                  |
/// +--------------------------------------------------------------------------------------------------+

int main() {

    setlocale(LC_CTYPE, "");

    FILE *file = openFile("fat16.img")->returnedValue;
    Buffer *byteStream = convertFileToBuffer(file)->returnedValue;
    closeFile(file);

    BootSector *bootSector = createBootSector(byteStream)->returnedValue;

    printTree(bootSector, byteStream);

    //ReturnStack *t = getBytesFromByteStream(byteStream->returnedValue, 0, );
    //printBuffer(t->returnedValue);


}

/// +--------------------------------------------------------------------------------------------------+
/// |                                                                                                  |
/// |                                            END FILE                                              |
/// |                                                                                                  |
/// +--------------------------------------------------------------------------------------------------+