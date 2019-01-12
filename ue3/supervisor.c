#include <fcntl.h>
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