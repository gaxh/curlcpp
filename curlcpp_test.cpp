#include "curlcpp.h"
#include "curlcpp_error.h"
#include <signal.h>
#include <unistd.h>

static bool running = true;

void start(Curlcpp &curl) {
    Curlcpp::REQUEST req;
    req.url = "http://127.0.0.1:8000";
    req.method = Curlcpp::CURLCPP_REQUEST_POST;
    req.posts = "POST_DATA";

    curl.HttpRequest(req, [&curl](int errcode, int httpcode, const std::string &body) {
                curlcpp_error("RESPONSE: errcode=%d, errmsg=%s, httpcode=%d, body=%s", errcode, Curlcpp::ErrorMessage(errcode), httpcode, body.c_str());

                start(curl);
            });
}

int main() {
    Curlcpp::GlobalInit();

    signal(SIGINT, [](int signum){running = false;});

    Curlcpp curl;

    start(curl);

    while(running) {
        usleep(500000);

        curl.Update();
    }

    Curlcpp::GlobalDestroy();
    return 0;
}


