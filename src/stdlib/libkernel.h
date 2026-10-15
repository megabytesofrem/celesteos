#ifndef CELESTEOS_STD_LIBKERNEL_H
#define CELESTEOS_STD_LIBKERNEL_H

extern "C" {

enum LogLevel {
    LOG_NONE,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
};

void printk(const char* format, ...);
void kputs(char* str);
void klog(enum LogLevel level, const char* format, ...);
}

#endif  // CELESTEOS_STD_LIBKERNEL_H