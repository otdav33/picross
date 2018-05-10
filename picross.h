#include <stdio.h>
#define PORT "8080"

#define stradd(buf, bufend, buflen, addition, addlen) \
    _stradd(&buf, &bufend, &buflen, addition, addlen)
//concatenates the string addition (with length addlen) into buf which has
//text length bufend and capacity buflen
void _stradd(char **buf, size_t *bufend, size_t *buflen,
        const char *addition, size_t addlen);

char *generate(char *query, char *path, int *final_length);
