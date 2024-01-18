#include "bmcweb_config.h"

#include "http/app.hpp"

#include "dbus_monitor.hpp"
#include "redfish.hpp"

#include <systemd/sd-daemon.h>

#include <boost/asio/io_context.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>
#include "shared_library.hpp"
#include <exception>
#include <memory>
#include <string>
#include "plugin-ifaces/router-plugin.h"

constexpr int defaultPort = 18080;

inline void addSubscription(crow::App& app){
  boost::beast::http::request<boost::beast::http::string_body> req;
      req.method(boost::beast::http::verb::post);
      req.target("/redfish/v2/EventService/Subscriptions/");
      req.body()=R"({"Destination":"https://9.3.84.101:8443/events","Protocol":"Redfish","DeliveryRetryPolicy": "RetryForever"})";
      req.set(boost::beast::http::field::content_type, "application/json");
      req.prepare_payload();
      std::error_code ec;
      crow::Request cReq(req,ec);
      auto resp =std::make_shared<bmcweb::AsyncResp>();
      app.handle(cReq,resp);
      BMCWEB_LOG_INFO("{}", resp->res.jsonValue.dump(2));
}
inline void sendTestEvent(crow::App& app){
  boost::beast::http::request<boost::beast::http::string_body> req;
  req.method(boost::beast::http::verb::post);
      req.target("/redfish/v2/EventService/Actions/EventService.SubmitTestEvent/");
      req.body()=R"({"EventType":"Alert","MessageId":"TestEvent","Message":"Test Event","Severity":"Critical"})";
      req.set(boost::beast::http::field::content_type, "application/json");
      req.prepare_payload();
      std::error_code ec;
      crow::Request cReq(req,ec);
      auto resp =std::make_shared<bmcweb::AsyncResp>();
      app.handle(cReq,resp);
      BMCWEB_LOG_INFO("{}", resp->res.jsonValue.dump(2));
  
}
inline void setupSocket(crow::App& app)
{
    app.port(defaultPort);
    return;
    int listenFd = sd_listen_fds(0);
    if (1 == listenFd)
    {
        BMCWEB_LOG_INFO("attempting systemd socket activation");
        if (sd_is_socket_inet(SD_LISTEN_FDS_START, AF_UNSPEC, SOCK_STREAM, 1,
                              0) != 0)
        {
            BMCWEB_LOG_INFO("Starting webserver on socket handle {}",
                            SD_LISTEN_FDS_START);
            app.socket(SD_LISTEN_FDS_START);
        }
        else
        {
            BMCWEB_LOG_INFO(
                "bad incoming socket, starting webserver on port {}",
                defaultPort);
            app.port(defaultPort);
        }
    }
    else
    {
        BMCWEB_LOG_INFO("Starting webserver on port {}", defaultPort);
        app.port(defaultPort);
    }

    
}

static int run()
{
    auto io = std::make_shared<boost::asio::io_context>();
    App app(io);
    setupSocket(app);
    sdbusplus::asio::connection systemBus(*io);
    crow::connections::systemBus = &systemBus;

    // Static assets need to be initialized before Authorization, because auth
    // needs to build the whitelist from the static routes
    redfish::RedfishService redfish(app);

    bmcweb::PluginDb db("/tmp/demo/plugins");
    auto interfaces = db.getInterFaces<bmcweb::RouterPlugin>();
    for (auto &plugin : interfaces) {
      BMCWEB_LOG_INFO("Registering plugin ");
      BMCWEB_LOG_INFO("{}", plugin->registerRoutes(app,crow::connections::systemBus));
    }
    app.run();
    io->run();
    crow::connections::systemBus = nullptr;
    return 0;
}

int main(int /*argc*/, char** /*argv*/)
{
    try
    {
        return run();
    }
    catch (const std::exception& e)
    {
        BMCWEB_LOG_CRITICAL("Threw exception to main: {}", e.what());
        return -1;
    }
    catch (...)
    {
        BMCWEB_LOG_CRITICAL("Threw exception to main");
        return -1;
    }
}
