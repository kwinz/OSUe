#include <stdlib.h>

/** @defgroup Tools */

/** @addtogroup Tools
 * @brief Provides Utility Tools
 *
 * @details Right now mostly a vector library
 *
 * @author Markus Krainz
 * @date December 2018
 *  @{
 */

#include "tools.h"

/**
 * @brief Initializes a vector and allocates some memory
 *
 * @detail Terminates the application if memory allocation fails
 * @param myvect The vector to be initialized
 */
void init_myvect(Myvect_t *myvect) {
  myvect->data = malloc(sizeof(float) * INITIAL_ARRAY_CAPACITY);
  if(unlikely(myvect->data == NULL)){
    //out of memory
    exit(EXIT_FAILURE);
  }
  myvect->size = 0;
  myvect->capacity = INITIAL_ARRAY_CAPACITY;
}

/**
 * @brief Stores a new Float in a vector
 *
 * @detail Terminates the application if memory allocation fails,
 * or if passed an invalid vector.
 * @param myvect The vector
 * @param data the float to be stored in the vector
 */
void push_myvect(Myvect_t *myvect, float data) {
  if (unlikely(myvect->size == myvect->capacity)) {
    if (unlikely(myvect->capacity == 0)) {
      // using a myvect that has not been initialized or already freed
      exit(EXIT_FAILURE);
    }

    myvect->capacity *= 2;
    myvect->data = realloc(myvect->data, sizeof(float) * myvect->capacity);
    if(unlikely(myvect->data == NULL)){
      //out of memory
      exit(EXIT_FAILURE);
    }
  }
  myvect->data[myvect->size] = data;
  myvect->size++;
}

/**
 * @brief Frees a vector
 *
 * @detail Frees the internal memory of vector.
 * After this function has been called do not reuse the vector.
 * If the vector structure was allocated on the heap do not forget to free it separately!
 * @param myvect The vector
 */
void freedata_myvect(Myvect_t *myvect) {
  free(myvect->data);
  myvect->size = 0;
  myvect->capacity = 0;
}

/** @}*/