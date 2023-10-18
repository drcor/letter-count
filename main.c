/**
 * @file main.c
 * @brief Description
 * @date 2023-10-17
 * @author Diogo Correia
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <sys/stat.h>
#include <ctype.h>

#include "debug.h"
#include "memory.h"
#include "args.h"
#include "letter.h"

char *filename;

struct thread_params_t {
    pthread_mutex_t *mutex;
    struct letter_t *hist;
    struct block_t *blocks;
    int blocks_len;
};

struct block_t {
    int offset;
    int size;
};

void increase_letter_count(struct letter_t *hist, char letter) ;
int get_file_size(char *filename);
int get_num_fileblocks(int filesize, int blocksize);
int get_num_threadblocks(int numthreads, int numblocks);
void *count_letters(void *arg);

int main(int argc, char *argv[]) {
    struct gengetopt_args_info args;
    struct thread_params_t *params = NULL;
    struct letter_t *hist = NULL;
    pthread_mutex_t mutex;

    // Parse arguments
    if (cmdline_parser(argc, argv, &args) != 0)
        ERROR(1, "Error parsing arguments");

    // Get values from arguments
    if (args.threads_arg <= 0)
        ERROR(1, "Threads number have to be positive");
    if (args.block_arg <= 0)
        ERROR(1, "Block size have to be positive");

    int numThreads = args.threads_arg;
    int blockSize = args.block_arg;
    filename = args.file_arg;

    /* BLOCKS */
    // Get to separate file in blocks for the threads
    int fileSize = get_file_size(filename);
    int numBlocks = get_num_fileblocks(fileSize, blockSize);

    // Initialize lettercount
    lettercount_init(&hist);

    // Configure threads args
    int numBlocksPerThread = get_num_threadblocks(numThreads, numBlocks);
    params = (struct thread_params_t *)MALLOC(sizeof(struct thread_params_t) * numThreads);
    for (int i = 0; i < numThreads; i++) {
        params[i].mutex = &mutex;
        params[i].hist = hist;
        params[i].blocks = (struct block_t *)MALLOC(sizeof(struct block_t) * numBlocksPerThread);
        params[i].blocks_len = 0;
    }

    // Create list of blocks dimentions and offsets from file
    int count = 0;
    for (int i = 0; i < numBlocks; i++) {
        // count accumulates the last offsets + blocksize
        struct block_t block;

        block.offset = count;

        // if the count will overflow the file size cut the block size
        if (count + blockSize > fileSize)
            block.size = fileSize - count;
        else {
            count += blockSize;
            block.size = blockSize-1;
        }

        // Assign each block to a thread
        params[i % numThreads].blocks[i / numThreads] = block;
        params[i % numThreads].blocks_len += 1; // Increase the number of blocks in the array
    }

    // Show blocks per thread
    /*for (int i = 0; i < numThreads; i++) {
        printf("Thread #%d:\n", i);
        for (int j = 0; j < params[i].blocks_len; j++) {
            printf("\toff=%d, size=%d\n", params[i].blocks[j].offset, params[i].blocks[j].size);
        }
    }*/

    /* THREADS */
    // Initialize threads mutex
    if ((errno = pthread_mutex_init(&mutex, NULL)) != 0)
        ERROR(2, "pthread_mutex_init() failed");

    // Create numThreads threads
    pthread_t *tids = (pthread_t *)MALLOC(sizeof(pthread_t) * numThreads);
    for (int i = 0; i < numThreads; i++) {
        if ((errno = pthread_create(&tids[i], NULL, count_letters, &params[i])))
            ERROR(4, "pthread_create() failed");
    }

    // Run all the threads
    for (int i = 0; i < numThreads; i++) {
        if ((errno = pthread_join(tids[i], NULL)) != 0)
            ERROR(5, "pthread_join() failed");
    }

    // Destroy threads mutex
    if ((errno = pthread_mutex_destroy(&mutex)) != 0)
        ERROR(3, "pthread_mutex_destroy() failed");

    for (int i = 0; i < ALPHABETH_SIZE; i++) {
        printf("%c:%d\n", hist[i].letter, hist[i].count);
    }

    // Free variables
    FREE(tids);
    lettercount_destroy(hist);

    for (int i = 0; i < numThreads; i++) {
        free(params[i].blocks);
    }
    FREE(params);
    cmdline_parser_free(&args);

    return 0;
}

// Called by thread to count letters
void *count_letters(void *arg) {
    struct thread_params_t *param = (struct thread_params_t *)arg;

    // Open file
    FILE *fptr = fopen(filename, "r");
    if (fptr == NULL)
        ERROR(21, "fopen() failed");

    /* Read the file blocks */
    for (int i = 0; i < param->blocks_len; i++) {
        struct block_t block = param->blocks[i];

        fseek(fptr, block.offset, SEEK_SET);

        // Read each character add add it to
        int curPos = 0;
        while (curPos <= block.size) {
            char curLetter;

            if (fread(&curLetter, sizeof(char), 1, fptr) == 0 && ferror(fptr) != 0)
                ERROR(22, "fread() failed");

            // Lock mutex
            if ((errno = pthread_mutex_lock(param->mutex)) != 0)
                ERROR(6, "pthread_mutex_lock() failed");

            increase_letter_count(param->hist, curLetter);

            // Unlock mutex
            if ((errno = pthread_mutex_unlock(param->mutex)) != 0)
                ERROR(7, "pthread_mutex_unlock() failed");

            curPos++;
        }
    }

    fclose(fptr);

    return NULL;
}

// Get file size in bytes
int get_file_size(char *filename) {
    struct stat statbuf;

    if (stat(filename, &statbuf) == -1)
        ERROR(20, "stat() failed");

    return (int)statbuf.st_size;
}

// Calculate number of blocks in a file
int get_num_fileblocks(int filesize, int blocksize) {
    int num = filesize / blocksize;
    if (filesize % blocksize > 0)
        num += 1;
    return num;
}

// Calculate number of block per thread
int get_num_threadblocks(int numthreads, int numblocks) {
    int num = numblocks / numthreads;
    if (numblocks % numthreads)
        num += 1;
    return num;
}

void increase_letter_count(struct letter_t *hist, char letter) {
    char c = tolower(letter);

    if ('a' <= c && c <= 'z') {
        int index = c - 'a';
        hist[index].count += 1;
    }
}
