#ifndef __CURLCPP_H__
#define __CURLCPP_H__

#include <memory>
#include <vector>
#include <string>
#include <functional>
#include "curlcpp_context.h"

extern "C" {
#include "curl/curl.h"
}

class Curlcpp {
public:
    enum CURLCPP_REQUEST_METHOD {
        CURLCPP_REQUEST_GET,
        CURLCPP_REQUEST_POST
    };

    struct REQUEST {
        std::string url;
        CURLCPP_REQUEST_METHOD method = CURLCPP_REQUEST_GET;
        std::string posts;
        std::vector<std::string> headers;
        long timeoutms = 2000L;
    };

    using HTTP_RESPONSE_CALLBACK = std::function<void(int errcode, long httpcode, const std::string &body)>;

    static bool GlobalInit();
    static void GlobalDestroy();

    void HttpRequest(const REQUEST &r, HTTP_RESPONSE_CALLBACK cb);
    int Update(); // 返回已处理响应的数量; <=0 表示没有任何响应得到处理

    static const char *ErrorMessage(int code);

private:
    std::shared_ptr<CURLM> m_curlm;
    
    struct CONTEXT {
        HTTP_RESPONSE_CALLBACK cb;
        std::string body;
        std::shared_ptr<curl_slist> headers;
    };
    CurlcppContext<CONTEXT> m_contexts;

    void AssignCurlm(CURLM *c);
    static void CurlmDeleter(CURLM *c);
    static void CurlSlistDeleter(curl_slist *s);
    void Lazyload();
    static size_t CurlGetDataCallback(char *ptr, size_t size, size_t count, void *userdata);
};

#endif

