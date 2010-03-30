#include <ext/hash_map>
#include <string>
#include <iostream>

using __gnu_cxx::hash;

int main()
{
   hash<const char*> Hs;
   std::string t1("cool");
   std::cout << "Hs(t1): " << Hs(t1.data()) << std::endl;
   std::string t2("cool");
   std::cout << "Hs(t2): " << Hs(t2.data()) << std::endl;
}
