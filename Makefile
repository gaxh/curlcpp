CPU_CORE_NUM = $(shell cat /proc/cpuinfo | grep ^processor | wc -l)
CURL_ROOT = curl/curl-7.78.0/
CURL_STATIS_LIB = $(CURL_ROOT)/lib/.libs/libcurl.a
CURL_INC = $(CURL_ROOT)/include/

CFLAGS = -g -Wall -I$(CURL_INC)
CFILES = curlcpp.cpp
CHEADERS = curlcpp.h curlcpp_error.h
LDFLAGS = -lpthread -lz -lssl -lcrypto

all : curlcpp_test

$(CURL_STATIS_LIB) : $(CURL_ROOT)/Makefile
	cd $(CURL_ROOT) && ./configure --with-openssl && $(MAKE) -j$(CPU_CORE_NUM)

curlcpp_test: $(CURL_STATIS_LIB) $(CFILES) $(CHEADERS) curlcpp_test.cpp
	$(CXX) $(CFLAGS) -o $@ $(CFILES) curlcpp_test.cpp $(LDFLAGS) $(CURL_STATIS_LIB)

clean:
	cd $(CURL_ROOT) && $(MAKE) clean
	rm -f curlcpp_test
