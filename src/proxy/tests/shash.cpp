#include "miscutil.h"

#include <iostream>
#include <stdlib.h>

using namespace sp;

int main(int argc, char *argv[])
{
  if (argc < 2)
    {
      std::cout << "Usage: shash <string>\n";
      exit(0);
    }

  std::string str = std::string(argv[1]);

  std::cout << miscutil::hash_string(str.c_str(),str.size()) << std::endl;;
}
