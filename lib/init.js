require("filterNotifier").FilterNotifier.addListener(function(action)
{
  if (action === "load")
    _triggerEvent("init");
});
