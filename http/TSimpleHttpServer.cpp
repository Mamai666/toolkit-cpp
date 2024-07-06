#include "TSimpleHttpServer.h"
#include <openssl/md5.h>
#include <iomanip>
#include <regex>
#include <memory>
#include <functional>

TSimpleHttpServer::TSimpleHttpServer(const std::string &host, const uint16_t &port,
                                     const bool isDigestAuthorized)
{
    m_isDigestAuthorized = isDigestAuthorized;
    m_host = host;
    m_port = port;

    m_pService = std::make_shared< restbed::Service >( );//new restbed::Service();

    std::function<void(const std::shared_ptr<restbed::Session>)> f_get_heartbeat(
                std::bind(&TSimpleHttpServer::get_heartbeat, this, std::placeholders::_1)
                );

    auto resourceHeartBeat = std::make_shared< restbed::Resource >( );
    resourceHeartBeat->set_path( "/heartbeat" );
    resourceHeartBeat->set_method_handler( "GET", f_get_heartbeat );
    m_resources.emplace("/heartbeat", resourceHeartBeat);
}

void service_ready_handler( restbed::Service& service)
{
    LOG(INFO) <<  "HTTP server running on " << service.get_http_uri()->to_string();
}

std::string build_authenticate_header( void )
{
    std::string header = "Digest realm=\""+realmStr+"\",";
    header += "algorithm=\"MD5\",";
    header += "stale=false,";
    header += "opaque=\"0000000000000000\",";
    header += "nonce=\""+nonceStr+"\"";

    return header;
}

const std::string generate_MD5_HA2(const std::string methodType, const std::string digestURI)
{
    unsigned char HA2[MD5_DIGEST_LENGTH] = {'0'};           // MD5(method:digestURI)
    const std::string prepHA2 = methodType+":"+digestURI;
    MD5((unsigned char*)prepHA2.data(), prepHA2.size(), HA2); // generate HA2
    std::ostringstream soutHA2;
    soutHA2<<std::hex<<std::setfill('0');
    for(long long c: HA2) {
        soutHA2<<std::setw(2)<<(long long)c;
    }
    return soutHA2.str();
}

const std::string generate_MD5_Resp(const unsigned char* HA1, const std::string nonce, const std::string HA2)
{
    unsigned char ResponseHash[MD5_DIGEST_LENGTH] = {'0'};  // MD5(HA1:nonce:HA2)
    std::basic_string<unsigned char> prepResponse = (unsigned char*)HA1;
    prepResponse += (unsigned char*)":";
    prepResponse += (unsigned char*)nonce.c_str();
    prepResponse += (unsigned char*)":";
    prepResponse += (unsigned char*)(HA2.c_str());
    MD5(prepResponse.data() , prepResponse.size(), ResponseHash);

    std::ostringstream soutResp;
    soutResp<<std::hex<<std::setfill('0');
    for(long long c: ResponseHash) {
        soutResp<<std::setw(2)<<(long long)c;
    }
    return soutResp.str();
}

void authentication_handler( const std::shared_ptr< restbed::Session > session,
                             const std::function< void ( const std::shared_ptr< restbed::Session > ) >& callback )
{
    const auto request = session->get_request( );
    auto authorisation = request->get_header( "Authorization" );
    std::string digestURI = "/";
    if(!authorisation.empty()) {
        auto posB = authorisation.find("uri=\"");
        auto posE = authorisation.find("\",",posB+5);
        digestURI = authorisation.substr(posB+5, posE-posB-5);
    }

    // 9285fe49721f13d8af6af59858c286c8  --- MD5(user:Glosav:terlus)
    const unsigned char HA1[] = {"9285fe49721f13d8af6af59858c286c8"}; // MD5(username:realm:password)
    const std::string methodType = session->get_request()->get_method();

    auto HA2 = generate_MD5_HA2(methodType, digestURI);
    auto RespH = generate_MD5_Resp(HA1, nonceStr, HA2);

    std::basic_string<unsigned char> strResponse;
    strResponse += (unsigned char*)".*response=";
    strResponse += (unsigned char*)"\"" ;
    strResponse += (unsigned char*)RespH.c_str();
    strResponse += (unsigned char*)"\"" ;
    strResponse += (unsigned char*)".*" ;

    bool authorised = regex_match( authorisation, std::regex( (char*)strResponse.c_str()) );
    if ( authorised ) {
        callback( session );
    }
    else {
        session->close( restbed::UNAUTHORIZED, { { "WWW-Authenticate", build_authenticate_header( ) } } );
    }
}

std::shared_ptr<restbed::Service> TSimpleHttpServer::getPService()
{
    return m_pService;
}

void TSimpleHttpServer::setPort(uint16_t newPort)
{
    m_port = newPort;
}

void TSimpleHttpServer::setHost(const std::string &newHost)
{
    m_host = newHost;
}

void TSimpleHttpServer::get_heartbeat(const std::shared_ptr<restbed::Session> session)
{
    LOG(INFO) << "\t**** Request get_heartbeat **** :";

    int retStatus = restbed::OK;
    std::string body = "{\"Connected\": true}";

    session->close(retStatus , body,
                    {
                        { "Access-Control-Allow-Origin", "*"},
                        { "Content-Type", "application/json"},
                        { "Content-Length", std::to_string(body.size()) },
                        { "Connection", "close" }
                    });
}

void TSimpleHttpServer::sigStop_handler( const int signal_number )
{
    LOG(WARNING) << "Http server catch signal " << signal_number;
    m_pService->stop();
    //stopListen();
    //this->~TSimpleHttpServer();
}

void TSimpleHttpServer::startListen(bool isNoneEmptyResources)
{
    if(!m_pService) {
        m_pService = std::make_shared< restbed::Service >( );//new restbed::Service();
    }

    if(isNoneEmptyResources)
    {
        if(m_resources.empty())
        {
            LOG(ERROR) << "Не задано ни одного ресурса, кроме /heartbeat !";
            exit(-1);
        }
        else {
            LOG(INFO) << "Публикуемые HTTP-ресурсы:";
            for(auto &itR : m_resources)
            {
                LOG(INFO) << "\t Res: " << itR.first ;
            }
        }
    }

    try
    {
        for(const auto &itRes : m_resources) {
            if(itRes.second)
                m_pService->publish(itRes.second);
        }
    }
    catch(std::exception &e) {
        LOG(ERROR) << "Exception: Error publish resources!" << std::endl;
    }
//-----------------------

    m_settings = std::make_shared< restbed::Settings >( );
    m_settings->set_worker_limit( std::thread::hardware_concurrency( ) );
    m_settings->set_reuse_address(true);
    m_settings->set_port( m_port );
    m_settings->set_bind_address(m_host);

    try
    {
        if(m_isDigestAuthorized) {
            m_pService->set_authentication_handler( authentication_handler );
        }
        m_pService->set_ready_handler( service_ready_handler );
        //m_pService->set_signal_handler(SIGINT, [this](const int sig) {sigStop_handler(sig);});
       // m_pService->set_signal_handler(SIGTERM, [this](const int sig) {sigStop_handler(sig);});
    }
    catch (std::exception &e) {
        LOG(ERROR) << "Exception: Error set handler!" << std::endl;
        return;
    }

    try
    {
        m_pService->start( m_settings );
    }
    catch (std::exception &e) {
        LOG(ERROR) << "http-server start(" << m_host << ":" << m_port << ") : "
                    << e.what() << std::endl;
    }
}

bool TSimpleHttpServer::stopListen()
{
    bool retOK = true;
    try
    {
        LOG(WARNING) << "Http-server stopping..";
        m_pService->stop();
        LOG(DEBUG) << "m_pService->is_down(): " << m_pService->is_down();
        if(m_pService->is_down()) {
            m_pService.reset();
            m_settings.reset();
        }
        else {
            retOK = false;
        }

    }
    catch(std::exception &e) {
        LOG(ERROR) << "Exception HttpServer stop: " << e.what() << std::endl;
        retOK = false;
    }
    return retOK;
}

//void TSimpleHttpServer::restartListen()
//{
//    m_settings->set_port( m_port );
//    m_settings->set_bind_address(m_host);

//    try
//    {
//        LOG(INFO) << "Http-server restart(" << m_settings->get_bind_address()
//                  << ":" << m_settings->get_port() << ")" << std::endl;
//        m_pService->set_ready_handler( service_ready_handler );
//        m_pService->start( m_settings );
//    }
//    catch (std::exception &e) {
//        LOG(ERROR) << "Exception HttpServer restart(" << m_host << ":" << m_port << ") : "
//                   << e.what() << std::endl;
//    }
//}

void TSimpleHttpServer::setIsDigestAuthorized(bool isDigestAuthorized)
{
    m_isDigestAuthorized = isDigestAuthorized;
}
