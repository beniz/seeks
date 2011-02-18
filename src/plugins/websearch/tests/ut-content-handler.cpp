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

#include "content_handler.h"

#include "errlog.h"

using namespace seeks_plugins;

TEST(CTTest,feature_based_similarity_scoring_fail)
{
  query_context qc;
  size_t nsps = 1;
  search_snippet *sps[nsps];
  search_snippet *ref_sp = NULL;
  int code = SP_ERR_OK;
  try
    {
      content_handler::feature_based_similarity_scoring(&qc,nsps,sps,ref_sp);
    }
  catch (sp_exception &e)
    {
      code = e.code();
    }
  ASSERT_EQ(WB_ERR_NO_REF_SIM,code);
}

TEST(CTTest,feature_based_similarity_scoring_fail_feat)
{
  query_context qc;
  size_t nsps = 1;
  search_snippet *sps[nsps];
  search_snippet ref_sp;
  int code = SP_ERR_OK;
  try
    {
      content_handler::feature_based_similarity_scoring(&qc,nsps,sps,&ref_sp);
    }
  catch (sp_exception &e)
    {
      code = e.code();
    }
  ASSERT_EQ(WB_ERR_NO_REF_SIM,code);
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
