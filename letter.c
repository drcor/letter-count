#include <stdlib.h>
#include <stdio.h>
#include "letter.h"
#include "debug.h"
#include "memory.h"

void lettercount_init(struct letter_t **hist) {
    *hist = (struct letter_t *)MALLOC(sizeof(struct letter_t) * ALPHABETH_SIZE);

    for (int i = 0; i < ALPHABETH_SIZE; i++) {
        (*hist)[i].letter = ALPHABETH[i];
        (*hist)[i].count = 0;
    }
}

void lettercount_destroy(struct letter_t *hist) {
    FREE(hist);
}
