/**
 * This is the p2p messaging component of the Seeks project,
 * a collaborative websearch overlay network.
 *
 * Copyright (C) 2006, 2010  Emmanuel Benazera, juban@free.fr
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

#include "SGNode.h"
#include "dht_api.h"
#include "sg_api.h"
#include "miscutil.h"
#include "errlog.h"

#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <assert.h>

using namespace dht;
using sp::miscutil;
using sp::errlog;

const char *usage = "Usage: dht_cli <ip:port> (--join ip:port) or (--self-bootstrap) (--persist) (--config config_file) (--nvnodes number_of_virtual_nodes)\n";

bool persistence = false;
SGNode *dnode = NULL;

void sig_handler(int the_signal)
{
  switch(the_signal)
    {
    case SIGTERM:
    case SIGINT:
    case SIGHUP:
      dnode->leave();
      delete dnode; // hibernates, stop threads and destroys internal structures.
      exit(the_signal);
      break;
    default:
      std::cerr << "sig_handler: exiting on unexpected signal " << the_signal << std::endl;
    }
  return;
}

Location *node = NULL;

int exec_command(std::vector<std::string> &tokens,
                 std::string &output, const SGNode &sgnode)
{
  if (tokens.empty())
    return 0; // no effect, no statement.

  std::string command = tokens.at(0);
  if (command == "help")
    {
      output = "help\t\t\t\t\tThis message\n";
      output += "ping <dhtkey> <addr:port>\t\tPing a node\n";
      output += "find <dht or sg key>\t\t\tFind the host node to a key\n";
      output += "subscribe\t\t\tPoll a search group without subscribing\n";
      output += "subscribe_sg <sg key>\t\t\tSubscribe to a search group\n";
      output += "quit\t\t\t\t\tLeave";
    }
  else if (command == "ping")
    {
      if (tokens.size() != 3)
        {
          output = "Usage: ping <dhtkey> <addr:port>";
          return 1;
        }
      DHTKey nodekey = DHTKey::from_rstring(tokens.at(1));
      std::string addr_str = tokens.at(2);
      std::vector<std::string> elts;
      miscutil::tokenize(addr_str,elts,":");
      if (elts.size() != 2)
        {
          output = "Wrong address format";
          return 1;
        }
      NetAddress na(elts.at(0),atoi(elts.at(1).c_str()));
      bool alive = false;
      dht_err err = DHT_ERR_OK;
      try
        {
          err = dht_api::ping(sgnode,nodekey,na,alive);
        }
      catch (dht_exception &e)
        {
          output = e.what();
          return 1;
        }
      if (err == DHT_ERR_UNKNOWN_PEER)
        {
          output = "unknown peer on remote node";
          return 1;
        }
      output = "node " + tokens.at(1) + " at " + addr_str + " is ";
      if (alive)
        output += "alive!";
      else output += "not alive!";
      return 0;
    }
  else if (command == "find")
    {
      if (tokens.size() != 2)
        {
          output = "Usage: find <dht or sg key>";
          return 1;
        }
      DHTKey key = DHTKey::from_rstring(tokens.at(1));
      if (node)
        delete node;
      node = new Location();
      dht_err err = DHT_ERR_OK;
      try
        {
          err = sg_api::find_sg(sgnode,key,*node);
        }
      catch (dht_exception &e)
        {
          output = e.what();
          return 1;
        }
      if (err != DHT_ERR_OK)
        {
          output = "no closest node found";
          if (err == DHT_ERR_MAXHOPS)
            output += " in the max number of hops (" + miscutil::to_string(dht_configuration::_dht_config->_max_hops) + ")";
          return 1;
        }
      output = "host node to " + tokens.at(1) + " is "
               + node->getDHTKey().to_rstring() + " at " + node->getNetAddress().toString();
      return 0;
    }
  else if (command == "subscribe" || command == "subscribe_sg")
    {
      if (tokens.size() != 2)
        {
          output = "Usage: subscribe_sg <sg key>";
          return 1;
        }
      DHTKey key = DHTKey::from_rstring(tokens.at(1));
      std::vector<Subscriber*> peers;
      assert(node != NULL);
      dht_err err;
      if (command == "subscribe_sg")
        {
          try
            {
              err = sg_api::get_sg_peers_and_subscribe(sgnode,key,*node,peers);
            }
          catch (dht_exception &e)
            {
              output = e.what();
              return 1;
            }
        }
      else if (command == "subscribe")
        {
          try
            {
              err = sg_api::get_sg_peers(sgnode,key,*node,peers);
            }
          catch (dht_exception &e)
            {
              output = e.what();
              return 1;
            }
        }
      if (err != DHT_ERR_OK)
        {
          output = "no closest node found";
          if (err == DHT_ERR_MAXHOPS)
            output += " in the max number of hops (" + miscutil::to_string(dht_configuration::_dht_config->_max_hops) + ")";
          return 1;
        }
      output = "host node to " + tokens.at(1) + " is "
               + node->getDHTKey().to_rstring() + " at " + node->getNetAddress().toString();
      if (peers.empty())
        output += "\nno peers";
      else
        {
          output += "\n" + miscutil::to_string(peers.size()) + " peers:\n";
          std::ostringstream os;
          for (size_t i=0; i<peers.size(); i++)
            {
              Subscriber *su = peers.at(i);
              if (su) // XXX: should never be NULL.
                os << *su;
              if (i != peers.size()-1)
                os << std::endl;
            }
          output += os.str();
        }
      return 0;
    }
  else if (command == "q" || command == "quit")
    {
      output = "leaving";
      return 0;
    }
  return 0;
}

int main(int argc, char **argv)
{
  if (argc < 2)
    {
      std::cout << usage;
      exit(0);
    }

  char *net_addr = argv[1];
  char *vec1[2];
  miscutil::ssplit(net_addr,":",vec1,SZ(vec1),0,0);
  net_addr = vec1[0];
  int net_port = atoi(vec1[1]);

  // init logging module.
  errlog::init_log_module();
  //errlog::set_debug_level(LOG_LEVEL_ERROR | LOG_LEVEL_DHT);

  int nvnodes = -1;
  bool joinb = false;
  bool sbootb = false;
  if (argc > 2)
    {
      std::vector<NetAddress> bootstrap_nodelist;
      int i = 2;
      while(i<argc)
        {
          const char *arg = argv[i];
          if (strcmp(arg, "--join") == 0)
            {
              joinb = true;
              char *bootnode = argv[i+1];
              char* vec[2];
              miscutil::ssplit(bootnode,":",vec,SZ(vec),0,0);
              int bootstrap_port = atoi(vec[1]);
              std::cout << "bootstrap node at ip: " << vec[0] << " -- port: " << bootstrap_port << std::endl;
              NetAddress bootstrap_na(vec[0],bootstrap_port);
              bootstrap_nodelist.push_back(bootstrap_na);
              i+=2;
            }
          else if (strcmp(arg,"--self-bootstrap") == 0)
            {
              sbootb = true;
              i++;
            }
          else if (strcmp(arg,"--persist") == 0)
            {
              persistence = true;
              i++;
            }
          else if (strcmp(arg,"--config") == 0)
            {
              DHTNode::_dht_config_filename = argv[i+1];
              i+=2;
            }
          else if (strcmp(arg,"--nvnodes") == 0)
            {
              nvnodes = atoi(argv[i+1]);
              i+=2;
            }
          else
            {
              std::cout << usage << std::endl;
              exit(0);
            }
        }

      dht_configuration::_dht_config = new dht_configuration(DHTNode::_dht_config_filename);
      dht_configuration::_dht_config->_routing = false;
      if (nvnodes > 0)
        dht_configuration::_dht_config->_nvnodes = nvnodes;

      dnode = new SGNode(net_addr,net_port);

      /**
       * unix signal handling for graceful termination.
       */
      const int catched_signals[] = { SIGTERM, SIGINT, SIGHUP, 0 };
      for (int idx=0; catched_signals[idx] != 0; idx++)
        {
          if (signal(catched_signals[idx],&sig_handler) == SIG_ERR)
            {
              std::cerr << "Can't set signal-handler value for signal "
                        << catched_signals[idx] << std::endl;
            }
        }

      /**
       * Join an existing ring or perform self-bootstrap.
       */
      if (joinb)
        {
          bool reset = true;
          dht_api::join_start(*dnode,bootstrap_nodelist,reset);
        }
      else if (sbootb)
        dht_api::self_bootstrap(*dnode);
    }

  // let the server thread start.
  NetAddress na_dnode(net_addr,net_port);
#if 0
  // I very much doubt this does anything useful (loic 2010/12)
  while(true)
    {
      dht_err status;
      dnode->_l1_client->RPC_ping(DHTKey(),na_dnode,status);
      if (status != DHT_ERR_COM_TIMEOUT)
        break;
    }
#endif

  // enter the prompt.
  std::string promptc = "###> ";
  std::string command;
  std::string output;
  while(true && output != "leaving")
    {
      std::cout << promptc;
      std::getline(std::cin,command);

      std::vector<std::string> tokens;
      miscutil::tokenize(command,tokens," ");

      output.clear();
      exec_command(tokens,output,*dnode);
      std::cout << output;
      std::cout << std::endl;
    }

  // clean exit.
  dnode->leave();
  delete dnode;
  delete dht_configuration::_dht_config;
}
