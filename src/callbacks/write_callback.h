#ifndef WRITECALLBACK_H
#define WRITECALLBACK_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata);

#endif
