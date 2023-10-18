#ifndef LETTER_H_
#define LETTER_H_

#define ALPHABETH_SIZE 26
#define ALPHABETH      "abcdefghijklmnopqrstuvwxyz"

struct letter_t {
    char letter;
    int count;
};

void lettercount_init(struct letter_t **hist);
void lettercount_destroy(struct letter_t *hist);

#endif // LETTER_H_
