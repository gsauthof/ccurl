// Simple C++ wrapper for the cURL API
//
// by Georg Sauthoff <mail@georg.so>, 2012, GPLv3+

#include "curl.hh"

#include <string.h>


static size_t ccurl_cb_fn(char *buffer, size_t size, size_t nmemb, void *p)
{
  curl::callback::base *cb = static_cast<curl::callback::base*>(p);
  assert(cb);
  if (cb->fn(buffer, size*nmemb))
    return size*nmemb;
  else
    return 0;
}

size_t ccurl_cb_header(void *v, size_t size, size_t nmemb, void *p)
{
  curl::handle *handle = static_cast<curl::handle*>(p);
  assert(handle);
  return handle->cb_header(v, size, nmemb);
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
  size_t handle::cb_header(void *v, size_t size, size_t nmemb)
  {
    size_t n = size * nmemb;
    char *h = static_cast<char*>(v);
    assert(h);

    if (n > 14 && !strncasecmp("Last-Modified:", h, 14)) {
      string s(h+14, n-14);
      time_t r = curl_getdate(s.c_str(), 0);
      if (r != -1)
        status.set_mtime(r);
    } else if (n > 5 && !strncasecmp("ETag:", h, 5)) {
      string s(h+5, n-5);
      size_t a = s.find_first_not_of(" \t\r\n");
      if (a != string::npos) {
        size_t b = s.find_last_not_of(" \t\r\n");
        assert(b != string::npos);
        status.set_etag(s.substr(a, b-a+1));
      }
    }
    if (cb_header_)
      return cb_header_->fn(h, n) ? n : 0;
    else
      return n;
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
    r = curl_easy_setopt(h, CURLOPT_HEADERFUNCTION, &ccurl_cb_header);
    check(r);
    r = curl_easy_setopt(h, CURLOPT_WRITEHEADER, this);
    check(r);
  }

  handle::handle(const global &g, callback::base &cb_)
    : h(0), cb(cb_), cb_header_(0)
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
  tag handle::get(const string &url, const tag *t)
  {
    if (t)
      old_status = *t;
    else
      old_status = tag();
    status = tag();

    set_option(CURLOPT_URL, url.c_str());

    CURLcode r;
    r = curl_easy_setopt(h, CURLOPT_TIMECONDITION, CURL_TIMECOND_IFMODSINCE);
    check(r);
    if (t)
      set_option(CURLOPT_TIMEVALUE, long(old_status.mtime()));
    else
      set_option(CURLOPT_TIMEVALUE, long(0));

    curl_slist *headers = 0;
    if (t) {
      ostringstream m;
      m << "If-None-Match: " << old_status.etag();
      headers = curl_slist_append(headers, m.str().c_str());
      if (!headers)
        throw runtime_error("curl_slist_append failed");
      r = curl_easy_setopt(h, CURLOPT_HTTPHEADER, headers);
      if (r != CURLE_OK) {
        curl_slist_free_all(headers);
        check(r);
      }
    } else {
      r = curl_easy_setopt(h, CURLOPT_HTTPHEADER, 0);
      check(r);
    }

    r = curl_easy_perform(h);
    if (headers) {
      curl_slist_free_all(headers);
      CURLcode a = curl_easy_setopt(h, CURLOPT_HTTPHEADER, 0);
      check(a);
    }
    check(r);

    long cond_code = 0;
    r = curl_easy_getinfo(h, CURLINFO_CONDITION_UNMET, &cond_code);
    check(r);
    if (cond_code)
      throw underflow_error("not modified since");

    long status_code = 0;
    r = curl_easy_getinfo(h, CURLINFO_RESPONSE_CODE, &status_code);
    check(r);
    if (status_code == 304) {
      // should be the same as checking CURLINFO_CONDITION_UNMET
      // but just in case the semantic changes for If-None-Match ...
      throw underflow_error("304 Not Modified");
    }
    if (status_code >= 400) {
      ostringstream m;
      m << "Get of " << url << " failed with a " << status_code << " code";
      throw runtime_error(m.str());
    }
    return status;
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


