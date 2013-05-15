_abpInitialized = false;
require("filterNotifier").FilterNotifier.addListener(function(action)
{
  if (action === "load")
  {
    _abpInitialized = true;
  }
});
