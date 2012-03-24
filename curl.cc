// Simple C++ wrapper for the cURL API
//
// by Georg Sauthoff <mail@georg.so>, 2012, GPLv3+

#include "curl.hh"

static size_t ccurl_cb_fn(char *buffer, size_t size, size_t nmemb, void *p)
{
  curl::callback::base *cb = static_cast<curl::callback::base*>(p);
  assert(cb);
  if (cb->fn(buffer, size*nmemb))
    return size*nmemb;
  else
    return 0;
}

bool curl::global::init = false;


namespace curl {

  global::global()
  {
    if (init)
      throw logic_error("Curl already globally initialized");
    if (curl_global_init(CURL_GLOBAL_SSL))
      throw runtime_error("Curl global initialization failed");
    init = true;
  }
  global::~global()
  {
    curl_global_cleanup();
  }

  void handle::check(CURLcode r)
  {
    if (!r)
      return;
    ostringstream m;
    m << "Curl function failed with: " << curl_easy_strerror(r);
    throw runtime_error(m.str());
  }
  void handle::set_defaults()
  {
    set_option(CURLOPT_NOSIGNAL, 1);
    set_option(CURLOPT_ENCODING,  "gzip, deflate");
    set_option(CURLOPT_FOLLOWLOCATION, 1);
    set_option(CURLOPT_MAXREDIRS, 8);
    set_option(CURLOPT_FAILONERROR, 1);
    CURLcode r = curl_easy_setopt(h, CURLOPT_WRITEFUNCTION, ccurl_cb_fn);
    check(r);
    r = curl_easy_setopt(h, CURLOPT_WRITEDATA, &cb);
    check(r);
  }

  handle::handle(const global &g, const callback::base &cb_)
    : h(0), cb(cb_)
  {
    h = curl_easy_init();
    if (!h)
      throw runtime_error("Curl: obtaining handle failed");
    set_defaults();
  }
  // http://curl.haxx.se/libcurl/c/libcurl-tutorial.html :
  // 'This will effectively close all connections this handle has used and possibly has kept open until now. Don't call this function if you intend to transfer more files.'
  handle::~handle()
  {
    curl_easy_cleanup(h);
  }
  void handle::get(const string &url)
  {
    set_option(CURLOPT_URL, url.c_str());
    CURLcode r = curl_easy_perform(h);
    check(r);
    long status = 0;
    r = curl_easy_getinfo(h, CURLINFO_RESPONSE_CODE, &status);
    check(r);
    if (status >= 400) {
      ostringstream m;
      m << "Get of " << url << " failed with a " << status << " code";
      throw runtime_error(m.str());
    }
  }
  void handle::set_useragent(const string &s)
  {
    set_option(CURLOPT_USERAGENT, s.c_str());
  }
  void handle::set_timeout(long secs)
  {
    set_option(CURLOPT_TIMEOUT, secs);
  }
  void handle::set_option(CURLoption id, long val)
  {
    CURLcode r = curl_easy_setopt(h, id, val);
    check(r);
  }
  void handle::set_option(CURLoption id, const char *val)
  {
    CURLcode r = curl_easy_setopt(h, id, val);
    check(r);
  }

  namespace callback {
    string::string(std::string &s, size_t maxi_)
        : str(s), maxi(maxi_)
      {
      }
      bool string::fn(const char *b, size_t n)
      {
        if (str.size() + n > maxi)
          return false;
        str.append(b, n);
        return true;
      }
      void string::clear() { str.clear(); }

      vector::vector(std::vector<char> &s, size_t maxi_)
        : str(s), maxi(maxi_)
      {
      }
      bool vector::fn(const char *b, size_t n)
      {
        if (str.size() + n > maxi)
          return false;
        str.insert(str.end(), b, b+n);
        return true;
      }
      void vector::clear() { str.clear(); }

      ostream::ostream(std::ostream &s, size_t maxi_)
        : str(&s), seen(0), maxi(maxi_)
      {
      }
      bool ostream::fn(const char *b, size_t n)
      {
        if (seen + n > maxi)
          return false;
        seen += n;
        str->write(b, n);
        if (str->bad()) {
          throw runtime_error("Output stream writing error");
        }
        return true;
      }
      void ostream::clear() { throw logic_error("Unable to clear on stream"); }
      void ostream::clear(std::ostream &o) { seen = 0; str = &o; }
  }


}


