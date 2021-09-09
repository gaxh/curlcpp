#ifndef __CURLCPP_ERROR_H__
#define __CURLCPP_ERROR_H__

#include <stdio.h>

#define curlcpp_error(fmt, args...) fprintf(stderr, "[%s:%d:%s]" fmt "\n", __FILE__, __LINE__, __FUNCTION__, ##args)

#endif
