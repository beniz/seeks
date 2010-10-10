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

#include "qprocess.h"
#include <iostream>
#include <math.h>
#include <assert.h>

#define RMDsize 160

using namespace lsh;

int main(int argc, char *argv[])
{
   // ref.
   std::string ref_keys[113] =
     {
	"ba9f4c802718618fed1bdea7451c90f9ac1df0ed",
	"dd18c88faf953857074a7bef356c348184ec6a5d",
	"a6bef3be023ba6bc43111eefc13f473947d141c5",
	"05cfa58a7bbb5837df0fc8abf5ee4ca7b8328331",
	"b26560d41f9801859f532aed6c59a9fea132004c",
	"339d24cbf8b0df516ad0204b03403a1e4b0f3d39",
	"c856f6ee6f392fda914f86076bca3dbfdd766765",
	"2ba673cc60dafb830abaa7cb579b43653cde4bd0",
	"9b7352c64a5171105ac709ce932737c01f74781f",
	"8816817e2c96d9933be020e543bbf2e9a33098c5",
	"877fc107e0c775a0bbc3b5b39084b77e79c29fb1",
	"856346ba0171289c041124cef08d598239894919",
	"b860738dc65b357769c06c03fc2f6fee0f38520e",
	"a54856e45f4918648d4ad1da48157d48ac41df6e",
	"fda0c04da8b9297bc85211c21e4996f3487a4535",
	"66d7ea70aedbee02dc128cbe25a009f657d17794",
	"85c4794b36c9462c8210b1acca94711f81c6617a",
	"2b6928a9fa366a548b2ab6ba459f2599721a3546",
	"275ce72195489f23f3afa5a2a7cb4a4de3cd6f5e",
	"61186f6f6ad9997c00112f90ea3abe8e514eabca",
	"ca42646a08cb49b762f8376bf0266b3f3c55e557",
	"1586f7a571db7e0623fee867623962ac54073659",
	"4af23a941d7401d2959f2a8a6b605fa42bf9ec21",
	"0c63b7d42239e5434a23eebdc7b62c868dc2928a",
	"385f9b1606699a117cea0cc0475d310bf73e6839",
	"f38c9a03cc0ee71f26e2e1c262a6d392667d424c",
	"e763f1ba90d80b4f772d929863488720c4ed17f1",
	"be5442e55cd2276d3c53f1591f58107c23482069",
	"335340afee3aabc46d30e862743f9374a51bb7f0",
	"f48c29c4c7d9929844c1fc4347ff41215fe701e5",
	"c87155bd0898da3b8ecfa83a165333994e884a97",
	"adba3d96a493191773ef6c648d6136fc6f3ce546",
	"40d107850ed6ac348c425decdaeaa565562723ec",
	"b415fbc385aa0f84594355bc14cb7df228e14fd4",
	"4a3ac12924d84d10ee0e9ac90399f83d68993deb",
	"183229c43559af8d1e433043ad38ea0560554285",
	"2b646908868fb2f063400f458b4b316143767075",
	"3aa63559a2b48c28d2c3a8673746443473067e9d",
	"b26560d41f9801859f532aed6c59a9fea132004c",
	"cca849c65ed9fc4d604231ed477e2dc29a0c04c7",
	"64cdb887cabaf687c7109bcb974dc08b667d174d",
	"6a815da4314aca7ddc90e536ee0db624e6c47f97",
	"64ffc8bfa19979df9290bc747f2522944d03b32c",
	"9f866a6001f987544beb6647cb938e0a77d9fb85",
	"d29cb2500ebaa6581c849f6d4cb2301042033425",
	"0aa9c347b489d58cf6ff99d7bd7667338b9c3cd4",
	"28c971585cd3a55be207e6df490ec5429f2af077",
	"c80aa196bad228b0c9b0e70b809aad33c14c7a74",
	"49870d07385a970647939fb32fb457c923370a40",
	"21283e1af7b98a00e331cf6415096c24500602c1",
	"26aca9a596563a660c2e19310fd61df72d372492",
	"fce3a9ea21c7f052dc782d31bc243ee111517bcb",
	"059d0a19edda991bab7a710c4ecc487d90cff699",
	"fec6a86f65d738a218922be4f1464fab44ade596",
	"9c07d2df0e3a226c934e9a07ff44ccf830f6bf74",
	"8816817e2c96d9933be020e543bbf2e9a33098c5",
	"c80ef224c4d7addb4a6f1c06af0d40f921241ddb",
	"f679216a98b967047c920f3ac40cd87d593ac690",
	"748f481dca92937edd6f008cfd854794fefce19b",
	"cc122cab067414184f5300105823623b7aaf6805",
	"5d4729c917b96a32ecefc98bb8260a001e3e4173",
	"e2bc8467bcb87747970ad2dbcc3790d978e68391",
	"e1297a9196c9b010c8d603006ed64489824a4e05",
	"8da9a8a3ac13576817f02f5b4c4b781154d3b339",
	"3b38febe50bafdb7ca4856416658a1812b3aa2f8",
	"fe8b8f69a9bb8eb83c0b1ae2f1c62654a4c6361c",
	"749a6375e0b4d0977e71e5602e846eb1bd131544",
	"17a470ab49b0a23651bc5ab738f846dac8e0ec01",
	"ee95295173d13452188ef1250d338ad8aec5de0b",
	"c65bc3996b0923960e7901ce75c5430ef9a88fd3",
	"38244b4ab4d0765d91f103836d6ff6200e130c11",
	"83a81675ea40712f4bc070f835e62534f0ae8020",
	"62f823e7b8d9a077479cdd2d4cfedb2e70af2049",
	"e2a83ab820794d0e8e3831ea0bc6716ba4651bb4",
	"ecf106fdcb1b2b93f491948b6a80af62e14899a7",
	"57f82c407e3808ec43d021b4d715bc507858763b",
	"5e6a01b8a3d9a417087f04667b1a2f41ea7f19e4",
	"a54856e45f4918648d4ad1da48157d48ac41df6e",
	"659b0de5b5d6b8e9685258dbb19b50e7803cc219",
	"fd576bcfc3821a8975dee505f3e1893b9cb2a3be",
	"7fe8c036f4dbb26b3d35e2fbaae9e0d097da36bc",
	"2b6928a9fa366a548b2ab6ba459f2599721a3546",
	"275ce72195489f23f3afa5a2a7cb4a4de3cd6f5e",
	"61186f6f6ad9997c00112f90ea3abe8e514eabca",
	"3fc7d8285ae22f2cd4022eb61079158ce186c05e",
	"ea2ae74d6f5121bba54eace40217861587fc04d7",
	"f111a6c22c3714f6ada02739848e056c3fb840c3",
	"519d4a8f73d4e62c6ccb7d5e06fb9465c80b3c53",
	"91a776b7f1551480cfd27ecfe6e47c5d74d3ba01",
	"f83781d97c5c68f7999c0fa8594b8b14f433d993",
	"8dc4aff606e3f9bbb2d0238cfcfb611a727b1dd0",
	"f546452bc1585713450f1ffd09558a9da44f1d64",
	"e2c3567e5ddb3772afcd43f5be9a5efaff449894",
	"e85f4de1ee98b88b5490311083909d8d1eb1e7c0",
	"726537c4e88935cfa9972baeafbcc1d190c772d7",
	"94b4fc93b878aacd434bf0cb24cfe800b0b2bfd3",
	"62a47201acd97085305079fd0135971ca36dc141",
	"4af23a941d7401d2959f2a8a6b605fa42bf9ec21",
	"5f67539fdaf9c1c4b4c852a79991656ab5c73ba0",
	"37352a6ffdb32acf0cd09fcd0c9b44f81762e0f5",
	"385f9b1606699a117cea0cc0475d310bf73e6839",
	"f38c9a03cc0ee71f26e2e1c262a6d392667d424c",
	"21201d6c3b0f2f764172317b266e4bee07dceab7",
	"be5442e55cd2276d3c53f1591f58107c23482069",
	"335340afee3aabc46d30e862743f9374a51bb7f0",
	"f48c29c4c7d9929844c1fc4347ff41215fe701e5",
	"c816e59793af68d4b5d3f0e8468b489ab1ac45a2",
	"fd86ec4edaab31cc31aa29e01431be58b3c545da",
	"856c992820db06458da8ddff2bd2eb8231909cff",
	"507046e956b4244685d1dafad70c32961fd49158",
	"adba3d96a493191773ef6c648d6136fc6f3ce546",
	"40d107850ed6ac348c425decdaeaa565562723ec",
	"b415fbc385aa0f84594355bc14cb7df228e14fd4"
     };
   int k = 0;
      
   // tests.
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
	std::cout << "radius=" << (*hit).first << " / " << rstring << std::endl;
	assert(rstring == ref_keys[k++]);
	++hit;
     }
   std::cout << std::endl;
   
   features.clear();
   query = "one two three four five";
   qprocess::generate_query_hashes(query,min_radius,max_radius,features);
   assert(features.size() == pow(2,5)-1);
   hit = features.begin();
   while(hit!=features.end())
     {
	rstring = (*hit).second.to_rstring();
	std::cout << "radius=" << (*hit).first << " / " << rstring << std::endl;
	assert(rstring == ref_keys[k++]);
	++hit;
     }
   std::cout << std::endl;
   
   features.clear();
   query = "one two three four, what we fighting for?";
   qprocess::generate_query_hashes(query,min_radius,max_radius,features);
   assert(features.size() == 79);
   hit = features.begin();
   while(hit!=features.end())
     {
	rstring = (*hit).second.to_rstring();
	std::cout << "radius=" << (*hit).first << " / " << rstring << std::endl;
	assert(rstring == ref_keys[k++]);
	++hit;
     }
   
   std::cout << "passed!\n";
}
