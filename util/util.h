#ifndef UTIL_H
#define UTIL_H

#include "legato.h"
#include <termios.h>

#define HIGH 1
#define LOW 0

typedef struct {
  int i;
  void* arr;
  void* ctxp;
} FunctionalArgs;

typedef struct {
  int n;                                   // number of elements in array
  bool (*callback)(FunctionalArgs* args);  // callback applied to arr[i]
  void* (*derefCallback)(int i, void* j);  // callback used to deference arr[i]
  FunctionalArgs args;
} Functional;

// ioutil
le_result_t readFromFile(const char* path,
                         void* value,
                         int (*scanCallback)(FILE* f, void* val));
int scanDoubleCallback(FILE* f, void* value);
le_result_t ioutil_readIntFromFile(const char* path, int* value);
le_result_t ioutil_readDoubleFromFile(const char* filePath, double* value);
le_result_t ioutil_writeToFile(const char* path,
                               void* value,
                               size_t size,
                               size_t count);

le_result_t ioutil_appendToFile(const char* path,
                                void* value,
                                size_t size,
                                size_t count);

// TODO fix this name (no PascalCase)
uint64_t GetCurrentTimestamp(void);
time_t util_getMTime(char* path);
int util_getUnixDatetime();
le_result_t util_flattenRes(le_result_t* res, int nRes);
bool util_fileExists(const char* path);
bool util_alreadyMounted(const char* devPath);
double util_avgDouble(double* readings, int nReadings);

// TODO verify this is working
le_result_t gpio_exportPin(char* pin);
le_result_t gpio_unexportPin(char* pin);
void getGpioPath(char* outputStr, char* pin, char* subDir);
le_result_t gpio_setDirection(char* pin, char* direction);
le_result_t gpio_setInput(char* pin);
le_result_t gpio_setOutput(char* pin);
le_result_t gpio_setActiveState(char* pin, bool isActiveLow);
le_result_t gpio_isActive(char* pin, bool* isActive);
le_result_t gpio_setValue(char* pin, bool state);
le_result_t gpio_setLow(char* pin);
le_result_t gpio_setHigh(char* pin);
le_result_t gpio_setPull(char* pin, char* pull);
le_result_t gpio_pullDown(char* pin);
le_result_t gpio_pullUp(char* pin);

void* util_find(Functional* f);

void util_listDir(const char* dir, char* dest, size_t size);

int fd_openSerial(const char* device, int baud);
speed_t fd_convertBaud(int baud);
int fd_puts(const int fd, const char* s);
int fd_getChar(const int fd);
void fd_flush(const int fd);
int fd_dataAvail(int fd, int* data);
void fd_flushInput(const int fd);

#endif
