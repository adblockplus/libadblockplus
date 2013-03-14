//
// The values are hardcoded for now.
//

var Prefs = exports.Prefs = {
  enabled: true,
  patternsfile: "patterns.ini",
  patternsbackups: 5,
  patternsbackupinterval: 24,
  data_directory: "",
  savestats: false,
  privateBrowsing: false,
  subscriptions_fallbackerrors: 5,
  subscriptions_fallbackurl: "https://adblockplus.org/getSubscription?version=%VERSION%&url=%SUBSCRIPTION%&downloadURL=%URL%&error=%ERROR%&channelStatus=%CHANNELSTATUS%&responseStatus=%RESPONSESTATUS%",
  subscriptions_autoupdate: true,
  subscriptions_exceptionsurl: "https://easylist-downloads.adblockplus.org/exceptionrules.txt",
  documentation_link: "https://adblockplus.org/redirect?link=%LINK%&lang=%LANG%",
  addListener: function() {}
};
