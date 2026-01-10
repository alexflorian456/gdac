#ifndef UTILS_H
#define UTILS_H

#define BLOCK_SCHEDULER(code)                         \
    do {                                              \
        sigset_t block_set, old_set;                  \
        sigemptyset(&block_set);                      \
        sigaddset(&block_set, SIGALRM);               \
        sigprocmask(SIG_BLOCK, &block_set, &old_set); \
        code                                          \
        sigprocmask(SIG_SETMASK, &old_set, NULL);     \
    } while (0)

#endif // UTILS_H