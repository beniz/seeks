/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2011 Emmanuel Benazera <ebenazer@seeks-project.info>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 **/

#define _PCREPOSIX_H // avoid pcreposix.h conflict with regex.h used by gtest
#include <gtest/gtest.h>
#include "charset_conv.h"

using sp::charset_conv;

TEST(QCStaticTest,charset_check_and_conversion)
{
  std::list<const char*> http_headers;
  std::string q = "seeks";
  std::string convq = charset_conv::charset_check_and_conversion(q,http_headers);
  ASSERT_EQ(q,convq);

  q = "\xe6\x97\xa5\xd1\x88\xfa"; // valid.
  convq = charset_conv::charset_check_and_conversion(q,http_headers);
  ASSERT_EQ(q,convq);

  q = "a\x80\xe0\xa0\xc0\xaf\xed\xa0\x80z"; // invalid.
  convq = charset_conv::charset_check_and_conversion(q,http_headers);
  ASSERT_EQ("",convq);

  /*#ifdef FEATURE_ICU
  q = "tu pourrais me g\xe9n\xe9rer une phrase plus longue comme \xe7a en latin1 stp";
  convq = charset_conv::charset_check_and_conversion(q,http_headers);
  ASSERT_FALSE(convq.empty());
  q = convq;
  convq = charset_conv::charset_check_and_conversion(q,http_headers);
  ASSERT_EQ(q,convq);
  #endif*/

  q = "\xe0\xe9";
  convq = charset_conv::charset_check_and_conversion(q,http_headers);
  ASSERT_EQ("",convq);

  http_headers.push_back("Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7");
  convq = charset_conv::charset_check_and_conversion(q,http_headers);
  ASSERT_EQ("àé",convq);

  http_headers.clear();
  http_headers.push_back("Accept-Charset: utf-8,ISO-8859-1;q=0.7,*;q=0.7");
  convq = charset_conv::charset_check_and_conversion(q,http_headers);
  ASSERT_EQ("àé",convq);
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
