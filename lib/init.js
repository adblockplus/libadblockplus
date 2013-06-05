let {Prefs} = require("prefs");
let {FilterNotifier} = require("filterNotifier");

let prefsInitDone = false;
let filtersInitDone = false;

function checkInitialized()
{
  if (prefsInitDone && filtersInitDone)
  {
    checkInitialized = function() {};
    _triggerEvent("init");
  }
}

Prefs._initListener = function()
{
  prefsInitDone = true;
  checkInitialized();
};

FilterNotifier.addListener(function(action)
{
  if (action === "load")
  {
    let {FilterStorage} = require("filterStorage");
    if (FilterStorage.subscriptions.length == 0)
    {
      // No data, must be a new user or someone with corrupted data - initialize
      // with default settings
      let {Subscription, DownloadableSubscription} = require("subscriptionClasses");
      let {Synchronizer} = require("synchronizer");
      let {Prefs} = require("prefs");
      let {Utils} = require("utils");

      // Choose default subscription and add it
      let subscriptions = require("subscriptions.xml");
      let node = Utils.chooseFilterSubscription(subscriptions);
      if (node)
      {
        let subscription = Subscription.fromURL(node.url);
        FilterStorage.addSubscription(subscription);
        subscription.disabled = false;
        subscription.title = node.title;
        subscription.homepage = node.homepage;
        if (subscription instanceof DownloadableSubscription && !subscription.lastDownload)
          Synchronizer.execute(subscription);
      }
    }

    filtersInitDone = true;
    checkInitialized();
  }
});
