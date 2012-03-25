// Simple C++ wrapper for the cURL API
//
// by Georg Sauthoff <mail@georg.so>, 2012, GPLv3+

#ifndef CURL_HH
#define CURL_HH

#include <curl/curl.h>

#include <stdexcept>
#include <string>
#include <vector>
#include <sstream>
#include <cassert>

size_t ccurl_cb_header(void *h, size_t size, size_t nmemb, void *p);

namespace curl {

  using namespace std;

  class global {
    private:
      // http://curl.haxx.se/libcurl/c/libcurl-tutorial.html :
      // 'Repeated calls to curl_global_init(3) and curl_global_cleanup(3) should be avoided. They should only be called once each.'
      //
      // -> once during program lifetime
      static bool init;

      global(const global&);
      global &operator=(const global&);
    public:
      global();
      ~global();
  };

  namespace callback {

    class base {
      private:
      public:
        virtual bool fn(const char *buffer, size_t size) = 0;
        virtual void clear() = 0;
    };
  
    struct string : public base {
      std::string &str;
      size_t maxi;

      string(std::string &s, size_t maxi_ = 4 * 1024 * 1024);
      bool fn(const char *b, size_t n);
      void clear();
    };

    struct vector : public base {
      std::vector<char> &str;
      size_t maxi;

      vector(std::vector<char> &s, size_t maxi_ = 4 * 1024 * 1024);
      bool fn(const char *b, size_t n);
      void clear();
    };

    struct ostream : public base {
      std::ostream *str;
      size_t seen, maxi;

      ostream(std::ostream &s, size_t maxi_ = 4 * 1024 * 1024);
      bool fn(const char *b, size_t n);
      void clear();
      void clear(std::ostream &o);
    };

  }

  class tag {
    private:
      string etag_;
      time_t mtime_;
    public:
      tag()
        : mtime_(0)
      {
      }
      tag(const string &etag, time_t mtime)
        : etag_(etag), mtime_(mtime)
      {
      }
      time_t mtime() const { return mtime_; }
      void set_mtime(time_t t) { mtime_ = t; }
      const string &etag() const { return etag_; }
      void set_etag(const string &s) { etag_ = s; }
      bool operator==(const tag &o) const
      {
        return (mtime_ && mtime_ == o.mtime_)
          || (!etag_.empty() && etag_ == o.etag_);
      }
  };

  inline std::ostream &operator<<(std::ostream &o, const tag &t)
  {
    o << t.mtime() << ' ' << t.etag();
    return o;
  }

  class handle {
    private:
      friend
        size_t ::ccurl_cb_header(void *h, size_t size, size_t nmemb, void *p);

      CURL *h;
      callback::base &cb;
      tag status, old_status;
      callback::base *cb_header_;

      void check(CURLcode r);
      void set_defaults();

      size_t cb_header(void *v, size_t size, size_t nmemb);

      handle(const handle&);
      handle &operator=(const handle&);
    public:
      handle(const global &g, callback::base &cb_);
      ~handle();
      tag get(const string &url, const tag *t = 0);
      void set_useragent(const string &s);
      void set_timeout(long secs);
      void set_option(CURLoption id, long val);
      void set_option(CURLoption id, const char *val);
  };

}

#endif
