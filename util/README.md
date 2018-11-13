# brnkl-util

Small collection of helper functions required by some of our open source code

## File I/O

Functions to handle boilerplate when reading and writing from files.

```c
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

```

## General

Collection of unsorted helper functions

```c
uint64_t GetCurrentTimestamp(void);
time_t util_getMTime(char* path);
int util_getUnixDatetime();
le_result_t util_flattenRes(le_result_t* res, int nRes);
bool util_fileExists(const char* path);
bool util_alreadyMounted(const char* devPath);
void* util_find(Functional* f);
void util_listDir(const char* dir, char* dest, size_t size);
```

## GPIO (Linux SysFS)

Provides API calls for all functionality supported by the Linux SysFS GPIO interface

```c
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
```

## Serial

Handler boilerplate for reading and writing to a serial port

```c
int fd_openSerial(const char* device, int baud);
speed_t fd_convertBaud(int baud);
int fd_puts(const int fd, const char* s);
int fd_getChar(const int fd);
void fd_flush(const int fd);
int fd_dataAvail(int fd, int* data);
void fd_flushInput(const int fd);
```
