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

  class handle {
    private:
      CURL *h;
      const callback::base &cb;

      void check(CURLcode r);
      void set_defaults();

      handle(const handle&);
      handle &operator=(const handle&);
    public:
      handle(const global &g, const callback::base &cb_);
      ~handle();
      void get(const string &url);
      void set_useragent(const string &s);
      void set_timeout(long secs);
      void set_option(CURLoption id, long val);
      void set_option(CURLoption id, const char *val);
  };

}

#endif
