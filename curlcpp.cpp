#include "curlcpp.h"
#include "curlcpp_error.h"

#define curl_easy_setopt_report_error(handler, key, args...)    do { \
    CURLcode _code_ = curl_easy_setopt((handler), (key), ##args); \
    if(_code_) { \
        curlcpp_error("call curl_easy_setopt failed, key=%d, code=%d", (key), _code_); \
    } \
} while(0)

bool Curlcpp::GlobalInit() {
    int code = (int)curl_global_init(CURL_GLOBAL_ALL);

    if(code) {
        curlcpp_error("call curl global init failed: %d", code);
        return false;
    }

    return true;
}

void Curlcpp::GlobalDestroy() {
    curl_global_cleanup();
}

void Curlcpp::CurlmDeleter(CURLM *c) {
    curlcpp_error("delete curlm instance: %p", c);
    curl_multi_cleanup(c);
}

void Curlcpp::AssignCurlm(CURLM *c) {
    if(c) {
        m_curlm.reset(c, CurlmDeleter);
    } else {
        m_curlm.reset();
    }
}

void Curlcpp::Lazyload() {
    if(!m_curlm) {
        AssignCurlm(curl_multi_init());
    }
}

void Curlcpp::HttpRequest(const REQUEST &r, HTTP_RESPONSE_CALLBACK cb) {
    Lazyload();

    CONTEXT context;
    context.cb = std::move(cb);

    if(r.headers.size()) {
        curl_slist *headers = NULL;

        for(const std::string &header : r.headers) {
            headers = curl_slist_append(headers, header.c_str());
        }

        context.headers.reset(headers, CurlSlistDeleter);
    }

    curl_slist *header_ptr = context.headers.get();
    void *context_id = m_contexts.Push(std::move(context));

    CURL *curl = curl_easy_init();

    curl_easy_setopt_report_error(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt_report_error(curl, CURLOPT_TIMEOUT_MS, r.timeoutms); // TIMEOUT_MS long
    curl_easy_setopt_report_error(curl, CURLOPT_HTTPHEADER, header_ptr); // struct curl_slist *, header
    curl_easy_setopt_report_error(curl, CURLOPT_WRITEFUNCTION, CurlGetDataCallback); // callback to get data
    curl_easy_setopt_report_error(curl, CURLOPT_WRITEDATA, m_contexts.Get(context_id)); // void *
    curl_easy_setopt_report_error(curl, CURLOPT_HEADER, 0L);
    curl_easy_setopt_report_error(curl, CURLOPT_URL, r.url.c_str()); // chat *, URL
    curl_easy_setopt_report_error(curl, CURLOPT_PRIVATE, context_id); // void *
    curl_easy_setopt_report_error(curl, CURLOPT_POSTFIELDSIZE, r.posts.size()); // long, size of post data
    curl_easy_setopt_report_error(curl, CURLOPT_COPYPOSTFIELDS, r.posts.c_str()); // char *, address of post data
    curl_easy_setopt_report_error(curl, CURLOPT_POST, r.method == CURLCPP_REQUEST_POST); // long

    CURLMcode code = curl_multi_add_handle(m_curlm.get(), curl);

    if(code) {
        curlcpp_error("add curlm handle failed: %d", code);
        curl_easy_cleanup(curl);
        m_contexts.Pop(context_id, NULL);
        cb(code, 0, "");
    }
}

void Curlcpp::CurlSlistDeleter(curl_slist *s) { curl_slist_free_all(s); }

size_t Curlcpp::CurlGetDataCallback(char *ptr, size_t size, size_t count, void *userdata) {
    CONTEXT *context = (CONTEXT *)userdata;

    size_t nb = size * count;

    context->body.append(ptr, nb);
    return nb;
}

int Curlcpp::Update() {
    Lazyload();

    CURLM *curlm = m_curlm.get();

    int numfds;
    CURLMcode code = curl_multi_wait(curlm, NULL, 0, 0, &numfds);

    if(code) {
        curlcpp_error("curl_multi_wait failed: %d", code);
        return 0;
    }

    int running_handles;
    curl_multi_perform(curlm, &running_handles);

    int msgs_left;
    CURLMsg *msg;
    int processed = 0;

    while( (msg = curl_multi_info_read(curlm, &msgs_left)) ) {
        CURL *curl = msg->easy_handle;
        curl_multi_remove_handle(curlm, curl);
        void *context_id = NULL;
        curl_easy_getinfo(curl, CURLINFO_PRIVATE, &context_id);
        
        CONTEXT context;
        m_contexts.Pop(context_id, &context);

        if(msg->msg == CURLMSG_DONE && msg->data.result == 0) {
            long httpcode = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpcode);
            context.cb(0, httpcode, context.body);
        } else {
            context.cb(msg->data.result ? msg->data.result : -1, 0, "");
        }

        curl_easy_cleanup(curl);
        ++processed;
    }

    return processed;
}

const char *Curlcpp::ErrorMessage(int code) {
    return curl_easy_strerror((CURLcode)code);
}
