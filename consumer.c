// consumer - the GPU simulation process
// does not manage shared memory, only write to it

#include <semaphore.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


// name of the shared memory object
#define SHMOBJ_PATH1 "/buffer_seg1"
#define SHMOBJ_PATH2 "/buffer_seg2"

// name of the semaphores
#define SEMOBJ_WRITE1 "/buffer_wsem1"
#define SEMOBJ_WRITE2 "/buffer_wsem2"
#define SEMOBJ_READ1 "/buffer_rsem1"
#define SEMOBJ_READ2 "/buffer_rsem2"

// length of the buffer = SPECIES * REACTIONS * LINES
// allocate + 1 as flag to indicate when the buffer is full
#define SHMBUF_LENGTH 100

#define ITERATIONS 1000

/* signal handler to clean up in case of premature termination */
void cleanup(int calledViaSignal) {
    shm_unlink(SHMOBJ_PATH1);
    shm_unlink(SHMOBJ_PATH2);
    sem_unlink(SEMOBJ_WRITE1);
    sem_unlink(SEMOBJ_WRITE2);
    sem_unlink(SEMOBJ_READ1);
    sem_unlink(SEMOBJ_READ2);

    if (calledViaSignal) {
        exit(3);
    }
}

int main(int argc, char *argv[]) {
    int i = 0, j = 0;
    int shmfd1, shmfd2;
    sem_t * sem_write1,* sem_read1,* sem_write2,* sem_read2;

    /* pointers to shared memory segments */
    float* shmptr1, * shmptr2;

    /* use double buffering */
    int shared_seg_size = (SHMBUF_LENGTH * sizeof(float));

    /* POSIX4 style signal handlers */
    struct sigaction sa;
    sa.sa_handler = cleanup;
    sa.sa_flags = 0;
    sigemptyset( &sa.sa_mask );
    (void) sigaction(SIGINT, &sa, NULL);
    (void) sigaction(SIGBUS, &sa, NULL);
    (void) sigaction(SIGSEGV, &sa, NULL);
    
    /* creating the shared memory objects    --  shm_open()  */
    shmfd1 = shm_open(SHMOBJ_PATH1, O_CREAT | O_RDONLY, S_IRWXU | S_IRWXG);
    if (shmfd1 < 0) {
        perror("In shm_open() of buffer 1.");
        exit(1);
    }

    shmfd2 = shm_open(SHMOBJ_PATH2, O_CREAT | O_RDONLY, S_IRWXU | S_IRWXG);
    if (shmfd2 < 0) {
        perror("In shm_open() of buffer 2");
        exit(1);
    }

 /* adjusting mapped file size (make room for the whole segment to map)      --  ftruncate() */
 //   ftruncate(shmfd1, shared_seg_size);
 //   ftruncate(shmfd2, shared_seg_size);
    

    /* requesting the shared segment    --  mmap() */    
    shmptr1 = (float *)mmap(NULL, shared_seg_size, PROT_READ, MAP_SHARED, shmfd1, 0);
    if (shmptr1 == NULL) {
        perror("In mmap() requesting buffer 1.");
        exit(1);
    }
    
    shmptr2 = (float *)mmap(NULL, shared_seg_size, PROT_READ, MAP_SHARED, shmfd2, 0);
    if (shmptr2 == NULL) {
        perror("In mmap() requesting buffer 2.");
        exit(1);
    }

    fprintf(stderr, "Created shared memory buffers %s and %s.\n", SHMOBJ_PATH1, SHMOBJ_PATH2);

 
    /* semaphores initially open to write initial message */
    sem_write1 = sem_open(SEMOBJ_WRITE1, O_CREAT, S_IRWXU, 1);
    if (sem_write1 == NULL) {
        perror("In sem_open() opening write semaphore 1.");
        exit(1);
    }

    sem_write2 = sem_open(SEMOBJ_WRITE2, O_CREAT, S_IRWXU, 1);    
    if (sem_write2 == NULL) {
        perror("In sem_open() opening write semaphore 2.");
        exit(1);
    }

    sem_read1 = sem_open(SEMOBJ_READ1, O_CREAT, S_IRWXU, 0);
    if (sem_read1 == NULL) {
        perror("In sem_open() opening read semaphore 1.");
        exit(1);
    }

    sem_read2 = sem_open(SEMOBJ_READ2, O_CREAT, S_IRWXU, 0);    
    if (sem_read2 == NULL) {
        perror("In sem_open() opening read semaphore 2.");
        exit(1);
    }

    fprintf(stderr, "Created semaphores correctly.\n");

    /* consuming messages */
    for(i = 0; i < ITERATIONS; i ++) {
        if ( i % 2 == 0) {
            // lock buffer 1
            sem_wait(sem_read1);
   
            printf("received message in buffer 1:\t");
            // write
            for (j = 0; j < SHMBUF_LENGTH; j ++) {
                printf("%f, ", shmptr1[j]);
            }
            printf("\n");

            // release buffer
            sem_post(sem_write1);
        } else {
            // lock buffer 2
            sem_wait(sem_read2);
          
            printf("received message in buffer 2:\t");
            // write
            for (j = 0; j < SHMBUF_LENGTH; j ++) {
                printf("%f, ", shmptr2[j]);
            }
            printf("\n");

            // release buffer
            sem_post(sem_write2);
        }
        printf("End of loop body: i=%d\n", i);
    }

    
    //snprintf(shared_msg->content, MAX_MSG_LENGTH, "My message, type %d, num %ld", shared_msg->type, random());
    
   
    /* remove shared memory objects --  shm_unlink() */
    if (shm_unlink(SHMOBJ_PATH1) != 0) {
        perror("In shm_unlink() of buffer 1");
        exit(1);
    }

    if (shm_unlink(SHMOBJ_PATH2) != 0) {
        perror("In shm_unlink() of buffer 2");
        exit(1);
    }
   fprintf(stderr, "Unlinked shared memory segments correctly.\n");

    /* remove semaphore -- sem_unlink() */
    if (sem_unlink(SEMOBJ_WRITE1) != 0) {
        perror("In sem_unlink() of write semaphore 1");
        exit(1);
    }

    if (sem_unlink(SEMOBJ_WRITE2) != 0) {
        perror("In sem_unlink() of write semaphore 2");
        exit(1);
    }

    if (sem_unlink(SEMOBJ_READ1) != 0) {
        perror("In sem_unlink() of read semaphore 1");
        exit(1);
    }

    if (sem_unlink(SEMOBJ_READ2) != 0) {
        perror("In sem_unlink() of read semaphore 2");
        exit(1);
    }
    fprintf(stderr, "Unlinked semaphores correctly.\n");
    
    return 0;
}
