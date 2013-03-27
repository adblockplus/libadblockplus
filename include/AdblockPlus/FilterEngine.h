#include <vector>
#include <string>

namespace AdblockPlus
{
    class JsEngine;

    struct Subscription
    {
        std::string url;
        std::string title;

        Subscription(const std::string& url, const std::string& title);
    };

    class FilterEngine
    {
    public:
        explicit FilterEngine(JsEngine& jsEngine);
        void AddSubscription(Subscription subscription);
        void RemoveSubscription(const Subscription& subscription);
        const Subscription* FindSubscription(const std::string& url) const;
        const std::vector<Subscription>& GetSubscriptions() const;
        void UpdateSubscriptionFilters(const Subscription& subscription);
        std::vector<Subscription> FetchAvailableSubscriptions();
        bool Matches(const std::string& url,
                     const std::string& contentType) const;
        std::vector<std::string> GetElementHidingRules() const;

    private:
        JsEngine& jsEngine;
        std::vector<Subscription> subscriptions;
    };
}
