

#include "curl.hh"

#include <iostream>


using namespace std;

int main(int argc, char **argv)
{
  assert(argc > 1);
  curl::global g;
  string s;
  curl::callback::string cb(s);
  curl::handle h(g, cb);
  curl::tag a, b;
  a = h.get(argv[1]);
  cout << s << '\n' << "mod: " << a << '\n';
  cb.clear();
  try {
    a = h.get(argv[1], &a);
    cout << s << '\n' << "mod: " << a << '\n';
  } catch (const underflow_error &e) {
    cout << "Saved page retrieval.\n";
  }
  return 0;
}
