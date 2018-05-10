#include "picross.h"

int main(int argc, char **argv) {
    cwebserv(argc > 1 ? argv[1] : PORT, generate);
}
