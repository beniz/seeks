/**
 * The Locality Sensitive Hashing (LSH) library is part of the SEEKS project and
 * does provide several locality sensitive hashing schemes for pattern matching over
 * continuous and discrete spaces.
 * Copyright (C) 2010 Emmanuel Benazera, ebenazer@seeks-project.info
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define _PCREPOSIX_H // avoid pcreposix.h conflict with regex.h used by gtest
#include <gtest/gtest.h>

#include "qprocess.h"
#include <iostream>
#include <math.h>
#include <assert.h>

#define RMDsize 160

using namespace lsh;


   // ref.
   std::string ref_keys[113] =
     {
	"e02e7dd2dbf606ca54b6842dfe7d2ae2635fe420",
	"1645a6897e62417931f26bcbdf4687c9c026b626",
	"f1e765bc8dbce1b523949be28084dbf49fe7e881",
	"6dbf15eb7cfdab7b978d4f08bc53131e3e4c2936",
	"9e8f4401eccc61ae847da7e6e5f26339ac372df7",
	"84927f81faad92313f05bc298086892a4ff7d1a1",
	"4439f708a7503ad7656c74b5df6dfcb894a73b8f",
	"a3f6d0c800c1e147d5f322b0ebd70ef45a99bc79",
	"2852569f76ad1073b50a441ba76e1dbabb1ff1fb",
	"825adf4250829e62691060f0a6322bac97d488d4",
	"43694f7c3be944d55f9f72450c0a3281f82d3f8e",
	"e274671d9595ad7fef0eab26b42e4d10b7517a45",
	"971a352fe65bc4dedd5a7b1f7e3308932b13598e",
	"23f389dbfa8cddc6b52447b0faa17f96860f8aab",
	"25739c5098237082852b6af60d5afe87ae823dc0",
	"653ffec715754c2c98f9aa38151f9f5e7f621ebb",
	"8376a35d2f649a37bffa7fa6ad4eb7b5a2a8b7c1",
	"8275b17484d61679d6465dd7cb0e7711de4adfcd",
	"9bb4158fc924658144f77160cb973a64c4f66dbc",
	"e59792adee5a70bd85729b6afc44a2660c9ed822",
	"f008171b0533d82d5f388266f15d0b5b46b2988b",
	"519c3f1baa1147d38d33f98131f8f720c3ab613d",
	"d5a53d71c901eab98cf2a35d890cabf7ed2fe5c9",
	"ab115a9f57a4ce7863108643f43317f5588f6e7d",
	"f8d51a68739282e022af7cf4e89f7b6ce76188a2",
	"516f7824662eabdabfc0846f00ebc58798affe94",
	"28eb334fec4baafba8fdf661885962f51a196630",
	"02c55195a35448bd150b81124a09af9d22bcf74b",
	"b1fa7b2d9a7b840488e5c0b2cb8e632177579954",
	"086c70194ea1f9f6a294b11bbf286c998dfbf0ab",
	"43a834ef7878c2fb529e4e74e028213f69a80e4d",
	"20cfadffc7efa8a8ae80b6f0ae6b482ed5361bc4",
	"4f6c98ac84d03babb639aa8a9271e4ae07b9f033",
	"a578b2bad7930d56c6c94f9c97c4f5ecea7e33ae",
	"0dc84fd0bf24576e978bed6ed8520aea0b738104",
	"7b0a6fb576e317231c5dd9eb021bc902a26596bd",
	"230dc88368140cc51476db91407f58302c57fe7a",
	"b35b78af4bf204970e83212342b81bf87eb38c3f",
	"9e8f4401eccc61ae847da7e6e5f26339ac372df7",
	"4334a8e50022edd63c997720fc369e501394fe1f",
	"9d0ef68b49b6f29bf99b9be47169a84fa4a0d60b",
	"fcbd950b47901c7c2773ca479c5b996f24a1ba84",
	"cee485a15097ae2e6117af8869ee24092d6c1247",
	"1f78d38536d8211ec25a79790dd3254513aa839a",
	"d56371d10d45662a400f1b655c57556ced50eba5",
	"c5a5bcbd38262a14602b9f33dea2f89556e89a3b",
	"15e6f2f4233eec855bbf0bbecf7f82a0f954e89c",
	"ee16a29761ec38f65fd40eb38bd4d1d5a6cad7df",
	"5d9fbf994f3367c0c2f683b633dd223a48f3dd50",
	"2945182426c3655c168d29f8374fb93c14e8263f",
	"262e0ba971382aef3b11ccb5db3d59b658d9dfca",
	"b67c9ca159888a706eaafea0d88067fa9c1cf6a5",
	"6de2a4bb1a6a08cd3f2ca39011f8f0f4ce130267",
	"a8c3983fbcb560d04eed200ea5d488a40702c7f3",
	"fcd6715b38e03efd7f146177546e535561616930",
	"825adf4250829e62691060f0a6322bac97d488d4",
	"9206831f6f8355254174036abcb8a83df31c2caf",
	"f9cc06b2319d2aa964ebc9c31dc82a6f680323d7",
	"547cee65fb1aef3bc84773454c289a5deb642c60",
	"d93ee949959d96df3a56d3a8cdc759c5e92c2a72",
	"1ecc38a8d65ed7c42843c692cb8f69be68ed10ab",
	"6d9083e55b4c3da8e31f54afafc1d64c13d2cf95",
	"2ed964ac6a2fd282584d5cec690e859eec4a8821",
	"9f3f11d51407f1af8ccc2cfa33dc176a497f5542",
	"d5fe399c8532e376b6b4ee4b6b6b64d41e6d0648",
	"81f77adb6e1da2f32d2014513c0fe642a1ccb4d6",
	"c190aa30eba8173074e239f454ad46d7a038bcf1",
	"f66f75d1bba8bd2dd974dd0f9991ca28d117225d",
	"175b7fc6e71016b9c055a6033b4805b78e77b58e",
	"426468c78a1569a5a3a10476de0239e1dfde6359",
	"3021c2cd9ae8ebbba93400e89d872ac4c6994ed8",
	"7ea3ada45ae5aa67c9c3eb42f25220febf31b7e2",
	"08f4f503ca5615532a2f2473e8b2d4131ea26aba",
	"4abe5d44353dffdc01845463c2bbcd32900b76bc",
	"e2869256c451ab46ada6d2074c127c45b41a226e",
	"c81dea6ff86903c5b8a443eaabe68ccc2cf22a8e",
	"4ae55ad22db8e89dbf13bbe4ee26318bb126de0f",
	"23f389dbfa8cddc6b52447b0faa17f96860f8aab",
	"1a0b7aa8d2d30af832c590526c77a17b1640c0e2",
	"ccb25fd94bd80a9a5fe4e7b948fe43dde9e3dc99",
	"c562955f9b93f865e8ad3e1d78c2eb76d626e426",
	"8275b17484d61679d6465dd7cb0e7711de4adfcd",
	"9bb4158fc924658144f77160cb973a64c4f66dbc",
	"e59792adee5a70bd85729b6afc44a2660c9ed822",
	"f1cbd62d418afcb2f8f15348a8465009df98e9c9",
	"338cd09a54f130bdf66619acbea907592f4a9991",
	"73a160735b23244874c5034b70202e65033d09fe",
	"19dce38c39d3ca630575dfbdd5982a5c22837cf1",
	"0ad27b841204ce5c5d4254e5f5a756f39089d775",
	"830a11a3be03503b0036623f030f4b9bbdb5e853",
	"0043ad873bad2bac56ec308fa7e766836bae86ae",
	"a4c4096a53e92593b6c92bf9141685ee056d5d58",
	"1141e6f78c7f4d4feef47b2453ff0e75198cf0cc",
	"ccc2c8147fbdd810459b1ae0d7307c8ffbf4b0df",
	"1e12769e8dffaad1ee683b687571b76071971a74",
	"1155c2f2e011db68aa09482823d7343c670c74a7",
	"f8accb06e2126701dec6ad564ae7d3d00bf09da8",
	"d5a53d71c901eab98cf2a35d890cabf7ed2fe5c9",
	"109b4c9849b5ae519c3a92e404fdf68f32f309a9",
	"3fb1f251bf7b0b4fc64c39752563e4c5f8b72c35",
	"f8d51a68739282e022af7cf4e89f7b6ce76188a2",
	"516f7824662eabdabfc0846f00ebc58798affe94",
	"de0cad20fac4e392effa248914e677d109b63fdb",
	"02c55195a35448bd150b81124a09af9d22bcf74b",
	"b1fa7b2d9a7b840488e5c0b2cb8e632177579954",
	"086c70194ea1f9f6a294b11bbf286c998dfbf0ab",
	"3ecc77ade03da34f56d66db9a0db768d80f659e3",
	"775fdf3053e1f3c1ff80cffc8435efd534592a88",
	"f4d40fd8c0118d8583dc877b6f292b72bc334fc0",
	"57daecf4fe9396d4cc9323bf16c65e85f3d6b009",
	"20cfadffc7efa8a8ae80b6f0ae6b482ed5361bc4",
	"4f6c98ac84d03babb639aa8a9271e4ae07b9f033",
	"a578b2bad7930d56c6c94f9c97c4f5ecea7e33ae"
     };
   int k = 0;
      
// tests.
TEST(QProcessTest, generate_query_hashes1)
{
   std::string query = "seeks project";
   int min_radius = 0;
   int max_radius = 5;
   
   hash_multimap<uint32_t,DHTKey,id_hash_uint> features;
   qprocess::generate_query_hashes(query,min_radius,max_radius,features);
   
   std::string rstring;
   hash_multimap<uint32_t,DHTKey,id_hash_uint>::const_iterator hit
     = features.begin();
   while(hit!=features.end())
     {
	rstring = (*hit).second.to_rstring();
	//std::cout << "radius=" << (*hit).first << " / " << rstring << std::endl;
	ASSERT_EQ(ref_keys[k++],rstring);
	++hit;
     }
   //std::cout << std::endl;
}

TEST(QProcessTest, generate_query_hashes2)
{
   int min_radius = 0;
   int max_radius = 5;
   hash_multimap<uint32_t,DHTKey,id_hash_uint> features;
   std::string query = "one two three four five";
   qprocess::generate_query_hashes(query,min_radius,max_radius,features);
   ASSERT_EQ(pow(2,5)-1,features.size());
   
   std::string rstring;
   hash_multimap<uint32_t,DHTKey,id_hash_uint>::const_iterator hit = features.begin();
   while(hit!=features.end())
     {
	rstring = (*hit).second.to_rstring();
	//std::cout << "radius=" << (*hit).first << " / " << rstring << std::endl;
	ASSERT_EQ(ref_keys[k++],rstring);
	++hit;
     }
   //std::cout << std::endl;
}
   
TEST(QProcessTest, generate_query_hashes3)
{
   int min_radius = 0;
   int max_radius = 5;
   hash_multimap<uint32_t,DHTKey,id_hash_uint> features;
   std::string query = "one two three four, what we fighting for?";
   qprocess::generate_query_hashes(query,min_radius,max_radius,features);
   ASSERT_EQ(79,features.size());
   
   std::string rstring;
   hash_multimap<uint32_t,DHTKey,id_hash_uint>::const_iterator hit = features.begin();
   while(hit!=features.end())
     {
	rstring = (*hit).second.to_rstring();
	//std::cout << "radius=" << (*hit).first << " / " << rstring << std::endl;
	ASSERT_EQ(ref_keys[k++],rstring);
	++hit;
     }
   //std::cout << "passed!\n";
}

int main(int argc, char **argv)
{
   ::testing::InitGoogleTest(&argc, argv);
   return RUN_ALL_TESTS();
}


