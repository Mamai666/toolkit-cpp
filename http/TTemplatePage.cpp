#include "TTemplatePage.h"
#include "include_logger.h"

TTemplatePage::TTemplatePage(std::shared_ptr<restbed::Service> service, std::map<std::string, RestResource> *resources)
{
    p_service = service;
    p_resources = resources;
}

void TTemplatePage::options_CORS(const std::shared_ptr<restbed::Session> session)
{
    std::string body = "";
    session->close(restbed::OK, body,
                    {
                        { "Access-Control-Allow-Origin", "*"},
                        { "Access-Control-Allow-Methods", "POST, GET, OPTIONS, DELETE"},
                        {"Access-Control-Allow-Headers", "X-Requested-With,content-type,Authorization"},
                        { "Connection", "close" }
                    }
                   );
}
