#ifndef SERVER_HPP
#define SERVER_HPP

#include <App.h>
#include <libusockets.h>

#include <nlohmann/json.hpp>

#include <string>

class WebServer {
    public:
    WebServer(class AnimationController &controller);
    ~WebServer();

    nlohmann::json Handle(int type, const nlohmann::json &metadata);

    void Run();
    void Halt();

    private:
    us_listen_socket_t *_socket;

    AnimationController &_controller;

    // static
    const std::string _page;
    const std::string _css;
    const std::string _js;

    short _port;
};

#endif