#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define SHM_NAME "/myshm"
#define MAX_DATA (50)
struct myshm {
  unsigned int state;
  unsigned int data[MAX_DATA];
};

// taken heavy inspiration from "Exercise 3: Shared Memory [..]" slides Platzer (2018)

#define BUF_LEN 8
// points to shared memory mapped with mmap(2)
int *buf;
// tracks free space, initialized to BUF_LEN
sem_t *free_sem;
// tracks used space, initialized to 0
sem_t *used_sem;
int write_pos = 0;
void circ_buf_write(int val) {
  // writing requires free space
  sem_wait(free_sem);
  buf[write_pos] = val;
  // space is used by written data
  sem_post(used_sem);
  write_pos = (write_pos + 1) % BUF_LEN;
}

int read_pos = 0;
int circ_buf_read() {
  // reading requires data (used space)
  sem_wait(used_sem);
  int val = buf[read_pos];
  // reading frees up space
  sem_post(free_sem);
  read_pos = (read_pos + 1) % BUF_LEN;
  return val;
}

int main(int argc, char *argv[]) {

  // create and/or open the shared memory object:
  int shmfd = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0600);
  if (shmfd == -1) {
    // error
  }
  // set the size of the shared memory:
  if (ftruncate(shmfd, sizeof(struct myshm)) < 0) {
    // error
  }
  // map shared memory object:
  struct myshm *myshm;
  myshm = mmap(NULL, sizeof(*myshm), PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);

  if (myshm == MAP_FAILED) {
    // error
  }

  if (close(shmfd) == -1) {
    // error
  }

  // unmap shared memory:
  if (munmap(myshm, sizeof(*myshm)) == -1) {
    // error
  }
  // remove shared memory object:
  if (shm_unlink(SHM_NAME) == -1) {
    // error
  }
}