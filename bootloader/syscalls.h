#pragma once

#include <sys/stat.h>
#include <sys/types.h>

int _close(int file);
int _fstat(int file, struct stat *st);
int _isatty(int file);
off_t _lseek(int file, off_t ptr, int dir);
int _read(int file, char *ptr, int len);
int _write(int file, char *ptr, int len);
int _kill(int pid, int sig);
int _getpid(void);
void *_sbrk(ptrdiff_t incr);
