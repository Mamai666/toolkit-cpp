#ifndef TTEMPLATEPAGE_H
#define TTEMPLATEPAGE_H

#include <restbed>
template<typename T>
static bool containInList(T item, std::vector<T> list)
{
    bool contain = false;
    for(auto &it : list) {
        if(item == it) {
            contain = true;
            break;
        }
    }
    return contain;
}

typedef std::shared_ptr<restbed::Resource> RestResource ;
typedef std::shared_ptr<restbed::Session>  RestSession  ;
typedef std::shared_ptr<const restbed::Request>  const_RestRequest  ;

class TTemplatePage
{
public:
    TTemplatePage(std::shared_ptr<restbed::Service> service, std::map<std::string, RestResource> *resources);
    static void options_CORS(const std::shared_ptr< restbed::Session > session);

protected:
    std::shared_ptr<restbed::Service> p_service;//   = nullptr;
    std::map<std::string, RestResource> *p_resources = nullptr;
};

#endif // TTEMPLATEPAGE_H
