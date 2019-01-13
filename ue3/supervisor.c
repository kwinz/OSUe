#include "tools.h"


// taken heavy inspiration from "Exercise 3: Shared Memory [..]" slides Platzer (2018)


// points to shared memory mapped with mmap(2)
int *buf;
// tracks free space, initialized to BUF_LEN
sem_t *free_sem;
// tracks used space, initialized to 0
sem_t *used_sem;
sem_t *write_sem;

Result_t circ_buf_read(Myshm_t* shm, sem_t *free_sem, sem_t *used_sem) {
  // reading requires data (used space)
  sem_wait(used_sem);
  Result_t val = shm->buf[shm->read_pos];
  // reading frees up space
  sem_post(free_sem);
  shm->read_pos = (shm->read_pos + 1) % BUF_LEN;
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

  //FIXME sem_open


  // unmap shared memory:
  if (munmap(myshm, sizeof(*myshm)) == -1) {
    // error
  }

  if (close(shmfd) == -1) {
    // error
  }

  // remove shared memory object:
  if (shm_unlink(SHM_NAME) == -1) {
    // error
  }

    //FIXME sem_close

    //FIXME sem_unlink
}