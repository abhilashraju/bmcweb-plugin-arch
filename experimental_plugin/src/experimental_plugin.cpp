#include "router-plugin.h"
#include <iostream>
#include <memory>
namespace redfish {
using namespace bmcweb;
using namespace reactor;
class MyRouterPlugin : public RouterPlugin {

public:
    MyRouterPlugin() {}
    std::string registerRoutes(crow::App &app) {
        registerTestRoutes(app);
        return "Registering Test routes";
    }
    void registerTestRoutes([[maybe_unused]]crow::App &app) {
        BMCWEB_LOG_INFO("registerTestRoutes");
        BMCWEB_ROUTE(app, "/redfish/v1/Test/")
            .privileges(redfish::privileges::postEventService)
            .methods(boost::beast::http::verb::post)(
                []([[maybe_unused]] const crow::Request & /*unused*/,
                    [[maybe_unused]] const std::shared_ptr<bmcweb::AsyncResp> &asyncResp) {
                CLIENT_LOG_INFO("called registerTestRoutes");
                asyncResp->res.jsonValue = {{"Test", "Test"}};
                });
    }
    static std::shared_ptr<MyRouterPlugin> create()
    {
        return std::make_shared<MyRouterPlugin>();
    }
    bool hasInterface(const std::string& interfaceId) override
    {
        return interfaceId == RouterPlugin::iid();
    }
    std::shared_ptr<BmcWebPlugin>
    getInterface(const std::string& interfaceId) override
    {
        if (interfaceId == RouterPlugin::iid())
        {
            return this->shared_from_this();
        }
        return RouterPlugin::getInterface(interfaceId);
    }
};
BMCWEB_SYMBOL_EXPORT std::shared_ptr<MyRouterPlugin> create_object()
{
    return MyRouterPlugin::create();
}
} // namespace redfish
