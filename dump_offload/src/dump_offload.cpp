#include "router-plugin.h"

// #include "registries/privilege_registry.hpp"
//#include "query.hpp"
#include <app.hpp>
#include <nlohmann/json.hpp>
#include <utils/json_utils.hpp>
#include <iostream>
#include <memory>
namespace redfish {
using namespace bmcweb;
class MyDumpRouterPlugin : public RouterPlugin {

public:
  MyDumpRouterPlugin() {}
  std::string registerRoutes(crow::App &app) {
    registerDumpRutes(app);
    return "Registering Dump routes";
  }
  void registerDumpRutes([[maybe_unused]]crow::App &app) {
    // BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/LogServices/")
    //     // .privileges(redfish::privileges::getLogServiceCollection)
    //     .methods(boost::beast::http::verb::get)(
    //         std::bind_front(handleBMCLogServicesCollectionGet, std::ref(app)));
    

    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Managers/bmc/LogServices/Dump/Entries/<str>/attachment/")
        // .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)([app=std::ref(app)](const crow::Request& req,
            const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
            const std::string& dumpId){
                handleLogServicesDumpEntryDownloadGet(app, "BMC", req, asyncResp, dumpId,"0");
            });
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Managers/bmc/LogServices/Dump/Entries/<str>/<str>/attachment/")
        // .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleLogServicesDumpEntryDownloadGet, std::ref(app), "BMC"));
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Managers/bmc/LogServices/Dump/Entries/<str>/<str>/<str>/attachment/")
        // .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleLogServicesDumpEntryDownloadGetPart, std::ref(app), "BMC"));
  }
static inline void handleLogServicesDumpEntryDownloadGet(
    crow::App& app, const std::string& dumpType,const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& dumpId,const std::string& offset)
{
    // if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    // {
    //     return;
    // }
    downloadDumpEntry(asyncResp, dumpId, dumpType,offset);
}
static inline void handleLogServicesDumpEntryDownloadGetPart(
    crow::App& app, const std::string& dumpType,const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& dumpId,const std::string& offset,const std::string& size)
{
    // if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    // {
    //     return;
    // }
    std::string dumpEntryPath =
        sdbusplus::message::object_path("/tmp") /
        dumpId;
    boost::system::error_code ec{};
    
    std::ifstream file(dumpEntryPath.c_str(), std::ios::binary);
    file.seekg(std::stoi(offset));
    std::string body;
    uint64_t isize = std::stoi(size);
    body.resize(static_cast<size_t>(isize), '\0');
    file.read(body.data(), body.size());
    asyncResp->res.write(std::move(body));
}
static inline void
    downloadDumpEntry(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& entryID, const std::string& dumpType,const std::string& offset)
{
    if (dumpType != "BMC")
    {
        BMCWEB_LOG_WARNING("Can't find Dump Entry {}", entryID);
        messages::resourceNotFound(asyncResp->res, dumpType + " dump", entryID);
        return;
    }

    // std::string dumpEntryPath =
    //     sdbusplus::message::object_path("/tmp") /
    //     std::string(boost::algorithm::to_lower_copy(dumpType)) / "entry" /
    //     entryID;
    std::string dumpEntryPath =
        sdbusplus::message::object_path("/tmp") /
        entryID;
    boost::system::error_code ec{};
    
    boost::beast::file_posix file;
    file.open(dumpEntryPath.c_str(), boost::beast::file_mode::read, ec);
    sdbusplus::message::unix_fd fd = file.native_handle();
    int intOffset=std::stoi(offset);
    long long int rc = lseek(fd, off_t(intOffset), SEEK_SET);
    downloadEntryCallback(asyncResp, entryID, dumpType, ec, fd);
    // auto downloadDumpEntryHandler =
    //     [asyncResp, entryID,
    //      dumpType](const boost::system::error_code& ec,
    //                const sdbusplus::message::unix_fd& unixfd) {
    //     downloadEntryCallback(asyncResp, entryID, dumpType, ec, unixfd);
    // };

    // crow::connections::systemBus->async_method_call(
    //     std::move(downloadDumpEntryHandler), "xyz.openbmc_project.Dump.Manager",
    //     dumpEntryPath, "xyz.openbmc_project.Dump.Entry", "GetFileHandle");
}
static inline bool chekSizeLimit(int fd,
                          const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    off_t currentPos = lseek(fd, 0, SEEK_CUR);
    long long int size = lseek(fd, currentPos, SEEK_END);
    long long int rc = lseek(fd, currentPos, SEEK_SET);
    if (size <= 0 || rc < 0)
    {
        BMCWEB_LOG_ERROR("Failed to get size of file, lseek() returned {}",
                         size);
        messages::internalError(asyncResp->res);
        close(fd);
        return false;
    }
    
    // Arbitrary max size of 20MB to accommodate BMC dumps
    constexpr uint64_t maxFileSize = 20 * 1024 * 1024;
    if (size > maxFileSize)
    {
        BMCWEB_LOG_ERROR("File size {} exceeds maximum allowed size of {}",
                         size, maxFileSize);
        messages::internalError(asyncResp->res);
        return false;
    }
    return true;
}
static void downloadEntryCallback(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& entryID,
                          const std::string& downloadEntryType,
                          const boost::system::error_code& ec,
                          const sdbusplus::message::unix_fd& unixfd)
{
    if (ec.value() == EBADR)
    {
        messages::resourceNotFound(asyncResp->res, "EntryAttachment", entryID);
        return;
    }
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBUS response error: {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }

    // Make sure we know how to process the retrieved entry attachment
    if ((downloadEntryType != "BMC") && (downloadEntryType != "System"))
    {
        BMCWEB_LOG_ERROR("downloadEntryCallback() invalid entry type: {}",
                         downloadEntryType);
        messages::internalError(asyncResp->res);
    }

    int fd = -1;
    fd = dup(unixfd);
    if (fd < 0)
    {
        BMCWEB_LOG_ERROR("Failed to open file");
        messages::internalError(asyncResp->res);
        return;
    }
    if (!chekSizeLimit(fd, asyncResp))
    {
        upgradeToChunked(asyncResp, fd,entryID);
        return;
    }
    if (downloadEntryType == "System")
    {
        if (!asyncResp->res.openBase64File(fd))
        {
            messages::internalError(asyncResp->res);
            close(fd);
            return;
        }
        asyncResp->res.addHeader(
            boost::beast::http::field::content_transfer_encoding, "Base64");
        return;
    }
    if (!asyncResp->res.openFile(fd))
    {
        messages::internalError(asyncResp->res);
        close(fd);
        return;
    }
    asyncResp->res.addHeader(boost::beast::http::field::content_type,
                             "application/octet-stream");
}
static void upgradeToChunked(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      int fd,const std::string& entryID)
{
    asyncResp->res.result(boost::beast::http::status::ok);
    asyncResp->res.addHeader(boost::beast::http::field::transfer_encoding,
                             "multipart/file");
    nlohmann::json chunkedResponseJson;
    
    off_t currentPos = lseek(fd, 0, SEEK_CUR);
    uint64_t size = lseek(fd, currentPos, SEEK_END);
    constexpr uint64_t chunkSize =  1024 * 1024;
    int count = size / chunkSize;
    
    BMCWEB_LOG_INFO("size {} count {}", size, count);
    std::vector<std::pair<std::string, std::string>> fileParts;
    int i=0;
    while (i<count)
    {
        fileParts.emplace_back(std::make_pair(
            std::to_string(i),
            "/redfish/v1/Managers/bmc/LogServices/Dump/Entries/"+entryID+"/"+
            std::to_string(currentPos+(i*chunkSize))+"/"+std::to_string(chunkSize)+"/attachment/"));
        i++;
    }
    uint64_t lastChunkSize= (size % chunkSize);
    if(lastChunkSize>0){
        fileParts.emplace_back(std::make_pair(
            std::to_string(i),
            "/redfish/v1/Managers/bmc/LogServices/Dump/Entries/"+entryID+"/"+
            std::to_string(currentPos+(i*chunkSize))+"/"+std::to_string(lastChunkSize)+"/attachment/"));
    }    
    chunkedResponseJson["urls"] =fileParts;
    asyncResp->res.jsonValue = chunkedResponseJson;
    
}
 static  void handleBMCLogServicesCollectionGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    // if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    // {
    //     return;
    // }
    // Collections don't include the static data added by SubRoute
    // because it has a duplicate entry for members
    asyncResp->res.jsonValue["@odata.type"] =
        "#LogServiceCollection.LogServiceCollection";
    asyncResp->res.jsonValue["@odata.id"] =
        "/redfish/v1/Managers/bmc/LogServices";
    asyncResp->res.jsonValue["Name"] = "Open BMC Log Services Collection";
    asyncResp->res.jsonValue["Description"] =
        "Collection of LogServices for this Manager";
    nlohmann::json& logServiceArray = asyncResp->res.jsonValue["Members"];
    logServiceArray = nlohmann::json::array();

    nlohmann::json::object_t journal;
    journal["@odata.id"] = "/redfish/v1/Managers/bmc/LogServices/Journal";
    logServiceArray.emplace_back(std::move(journal));

    asyncResp->res.jsonValue["Members@odata.count"] = logServiceArray.size();

    nlohmann::json& logServiceArrayLocal =
            asyncResp->res.jsonValue["Members"];
    std::vector<std::string> subTreePaths = {
        "/xyz/openbmc_project/logging",
        "/xyz/openbmc_project/dump",
    };
    for (const std::string& path : subTreePaths)
    {
        if (path == "/xyz/openbmc_project/dump/bmc")
        {
            nlohmann::json::object_t member;
            member["@odata.id"] =
                "/redfish/v1/Managers/bmc/LogServices/Dump";
            logServiceArrayLocal.emplace_back(std::move(member));
        }
        else if (path == "/xyz/openbmc_project/dump/faultlog")
        {
            nlohmann::json::object_t member;
            member["@odata.id"] =
                "/redfish/v1/Managers/bmc/LogServices/FaultLog";
            logServiceArrayLocal.emplace_back(std::move(member));
        }
    }

    asyncResp->res.jsonValue["Members@odata.count"] =
        logServiceArrayLocal.size();

}
  static std::shared_ptr<MyDumpRouterPlugin> create()
    {
        return std::make_shared<MyDumpRouterPlugin>();
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

BMCWEB_SYMBOL_EXPORT std::shared_ptr<MyDumpRouterPlugin> create_object()
{
    return MyDumpRouterPlugin::create();
}
} // namespace redfish
