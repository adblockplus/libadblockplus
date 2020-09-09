const {Utils} = require("utils");

function isValidString(s)
{
  return s && s.indexOf("\0") === -1;
}

/**
  * Remove descriptors like 2x or 100w from srcset urls
  */
function trimSrcsetDescriptors(url)
{
  return url.trim().replace(/\s+\S+$/, "");
}

/**
  * Replace scheme and www with || to match any subdomain and protocol
  */
function urlToGenericFilter(url)
{
  return url.replace(/^[\w-]+:\/+(?:www\.)?/, "||");
}

/**
  * Only http, https or relative url are supported.
  * E.g. no file: wss: or any other exotic protocols.
  */
function isSupportedUrl(url)
{
  return !/^(?!https?:)[\w-]+:/i.test(url);
}

function composeForRelatedUrls(baseUrl, relatedUrls)
{
  let filters = [];

  for (let rawUrl of relatedUrls)
  {
    const url = trimSrcsetDescriptors(rawUrl);

    if (isSupportedUrl(url))
    {
      let urlObj = new URL(url, baseUrl);
      let filterText = urlToGenericFilter(urlObj.href);

      if (!filters.includes(filterText))
        filters.push(filterText);
    }
  }

  return filters;
}

function composeForId(id, selectors)
{
  if (isValidString(id))
    selectors.push( "#" + Utils.escapeCSS(id));
}

function composeForClasses(classes, selectors)
{
  let cls = classes.split(/\s+/).filter(isValidString);
  if (cls.length > 0)
    selectors.push(cls.map(c => "." + Utils.escapeCSS(c)).join(""));
}

function composeForSrc(src, tagName, selectors)
{
  if (isValidString(src))
    selectors.push(Utils.escapeCSS(tagName) + "[src=" + Utils.quoteCSS(src) + "]");
}

function composeForStyle(style, tagName, selectors)
{
  if (isValidString(style))
    selectors.push(Utils.escapeCSS(tagName) + "[style=" + Utils.quoteCSS(style) + "]");
}

exports.composeFilterSuggestions = function composeFilterSuggestions(baseUrl, tagName, id, src, style, classes, relatedUrls)
{
  let filters = composeForRelatedUrls(baseUrl, relatedUrls);

  if (filters.length !== 0)
    return filters;

  let selectors = [];
  let docDomain = extractHostFromURL(baseUrl);

  composeForId(id, selectors);
  composeForClasses(classes, selectors);
  composeForSrc(src, tagName, selectors);

  if (selectors.length === 0)
    composeForStyle(style, tagName, selectors);

  const simpleDomain = docDomain.replace(/^www\./, "");

  for (let selector of selectors)
    filters.push(simpleDomain + "##" + selector);

  return filters;
}
