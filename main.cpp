/*
 * CEC anyway
 *
 * (C) by Magnus Kulke 2013 (mkulke at gmail dot com)
 *
 * This program is released and can be redistributed and/or modified
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "libcec/cec.h"
#include "lib/xbmcclient.h"
#include <cstdio>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <map>
#include <stdlib.h>
#include <unistd.h>

using namespace CEC;
using namespace std;

#define CEC_CONFIG_VERSION CEC_CLIENT_VERSION_CURRENT;

#include "libcec/cecloader.h"

ICECCallbacks        callbacks;
libcec_configuration configuration;
string               port;
bool                 aborted;
bool                 daemonize;
bool                 logEvents;
string               configFilePath;
unsigned int         rpcPort;
map<int, string>     keyMap;
map<int, string>     eventMap;

void populateKeyMapDefault()
{
  /* NOT USED */
  keyMap[CEC_USER_CONTROL_CODE_LEFT] = "{\"jsonrpc\": \"2.0\", \"id\": 1, \"method\": \"Input.Left\"}";
  keyMap[CEC_USER_CONTROL_CODE_RIGHT] = "{\"jsonrpc\": \"2.0\", \"id\": 1, \"method\": \"Input.Right\"}";
  keyMap[CEC_USER_CONTROL_CODE_DOWN] = "{\"jsonrpc\": \"2.0\", \"id\": 1, \"method\": \"Input.Down\"}";
  keyMap[CEC_USER_CONTROL_CODE_UP] = "{\"jsonrpc\": \"2.0\", \"id\": 1, \"method\": \"Input.Up\"}";
  keyMap[CEC_USER_CONTROL_CODE_SELECT] = "{\"jsonrpc\": \"2.0\", \"id\": 1, \"method\": \"Input.Select\"}";
  keyMap[CEC_USER_CONTROL_CODE_EXIT] = "{\"jsonrpc\": \"2.0\", \"id\": 1, \"method\": \"Input.Back\"}";
  keyMap[CEC_USER_CONTROL_CODE_PLAY] = "{\"jsonrpc\": \"2.0\", \"method\": \"Player.PlayPause\", \"params\": { \"playerid\": 1 }, \"id\": 1}";
  keyMap[CEC_USER_CONTROL_CODE_STOP] = "{\"jsonrpc\": \"2.0\", \"method\": \"Player.Stop\", \"params\": { \"playerid\": 1 }, \"id\": 1}";
  keyMap[CEC_USER_CONTROL_CODE_PAUSE] = "{\"jsonrpc\": \"2.0\", \"method\": \"Player.PlayPause\", \"params\": { \"playerid\": 1 }, \"id\": 1}";
  keyMap[CEC_USER_CONTROL_CODE_REWIND] = "{\"jsonrpc\": \"2.0\", \"method\": \"Player.Seek\", \"params\": { \"playerid\": 1, \"value\": \"smallbackward\" }, \"id\": 1}";
  keyMap[CEC_USER_CONTROL_CODE_BACKWARD] = "{\"jsonrpc\": \"2.0\", \"method\": \"Player.Seek\", \"params\": { \"playerid\": 1, \"value\": \"bigbackward\" }, \"id\": 1}";
  keyMap[CEC_USER_CONTROL_CODE_FAST_FORWARD] = "{\"jsonrpc\": \"2.0\", \"method\": \"Player.Seek\", \"params\": { \"playerid\": 1, \"value\": \"smallforward\" }, \"id\": 1}";
  keyMap[CEC_USER_CONTROL_CODE_FORWARD] = "{\"jsonrpc\": \"2.0\", \"method\": \"Player.Seek\", \"params\": { \"playerid\": 1, \"value\": \"bigforward\" }, \"id\": 1}";

  /* USED */
  keyMap[CEC_USER_CONTROL_CODE_CLEAR] = "{\"jsonrpc\": \"2.0\", \"method\": \"Input.Home\", \"id\": 1}";
  keyMap[CEC_USER_CONTROL_CODE_ELECTRONIC_PROGRAM_GUIDE] = "{\"jsonrpc\": \"2.0\", \"method\": \"GUI.SetFullscreen\", \"params\": { \"name\": \"fullscreen\", \"value\": \"toggle\" }, \"id\": 1}";
}

void populateEventMapDefault()
{
  eventMap[CEC_USER_CONTROL_CODE_LEFT] = "left";
  eventMap[CEC_USER_CONTROL_CODE_RIGHT] = "right";
  eventMap[CEC_USER_CONTROL_CODE_DOWN] = "down";
  eventMap[CEC_USER_CONTROL_CODE_UP] = "up";
  eventMap[CEC_USER_CONTROL_CODE_SELECT] = "select";
  eventMap[CEC_USER_CONTROL_CODE_EXIT] = "back";
  eventMap[CEC_USER_CONTROL_CODE_PLAY] = "play";
  eventMap[CEC_USER_CONTROL_CODE_STOP] = "stop";
  eventMap[CEC_USER_CONTROL_CODE_PAUSE] = "pause";
  eventMap[CEC_USER_CONTROL_CODE_REWIND] = "reverse";
  eventMap[CEC_USER_CONTROL_CODE_BACKWARD] = "skipminus";
  eventMap[CEC_USER_CONTROL_CODE_FAST_FORWARD] = "forward";
  eventMap[CEC_USER_CONTROL_CODE_FORWARD] = "skipplus";
  // eventMap[CEC_USER_CONTROL_CODE_F1_BLUE] = "blue";
  eventMap[CEC_USER_CONTROL_CODE_F2_RED] = "red";
  eventMap[CEC_USER_CONTROL_CODE_F3_GREEN] = "green";
  eventMap[CEC_USER_CONTROL_CODE_F4_YELLOW] = "yellow";
  eventMap[CEC_USER_CONTROL_CODE_SETUP_MENU] = "title";
  eventMap[CEC_USER_CONTROL_CODE_ELECTRONIC_PROGRAM_GUIDE] = "backslash";

  eventMap[CEC_USER_CONTROL_CODE_CHANNEL_UP] = "pageplus";
  eventMap[CEC_USER_CONTROL_CODE_CHANNEL_DOWN] = "pageminus";
}

int CecKeyPressCB(void*, const cec_keypress key)
{
  std::cout<<"Key press "<<key.keycode<<" " << key.duration<<std::endl;
  if (key.duration == 0 || key.keycode == CEC_USER_CONTROL_CODE_STOP)
  {
    string json = "unmapped";

    if (key.keycode == CEC_USER_CONTROL_CODE_VOLUME_UP)
    {
      system("pactl set-sink-volume 1 -- +10%");
    }
    else if (key.keycode == CEC_USER_CONTROL_CODE_VOLUME_DOWN)
    {
      system("pactl set-sink-volume 1 -- -10%");
    }
    else if (key.keycode == CEC_USER_CONTROL_CODE_MUTE)
    {
      system("pactl list sinks | grep -q Mute:.no && pactl set-sink-mute 1 1 || pactl set-sink-mute 1 0");
    }
    else if (key.keycode == CEC_USER_CONTROL_CODE_F1_BLUE)
    {
      system("returntodesktop.sh");
    }
    else if (eventMap.find(key.keycode) != eventMap.end())
    {
      /* connect to localhost, port 9777 using a UDP socket
         this only needs to be done once.
         by default this is where XBMC will be listening for incoming
         connections. */
      CAddress my_addr; // Address => localhost on 9777
      int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
      if (sockfd < 0)
      {
        cout << "error creating socket" << endl;
        return 1;
      }

      my_addr.Bind(sockfd);

      if (key.keycode == CEC_USER_CONTROL_CODE_ELECTRONIC_PROGRAM_GUIDE) {
        CPacketBUTTON btn1(eventMap[key.keycode].c_str(), "KB", BTN_DOWN | BTN_USE_NAME | BTN_QUEUE);
        btn1.Send(sockfd, my_addr);

        CPacketBUTTON btn2(eventMap[key.keycode].c_str(), "KB", BTN_UP | BTN_USE_NAME | BTN_QUEUE | BTN_NO_REPEAT);
        btn2.Send(sockfd, my_addr);
      }
      else
      {
        CPacketBUTTON btn1(eventMap[key.keycode].c_str(), "R1", BTN_DOWN | BTN_USE_NAME | BTN_QUEUE);
        btn1.Send(sockfd, my_addr);

        CPacketBUTTON btn2(eventMap[key.keycode].c_str(), "R1", BTN_UP | BTN_USE_NAME | BTN_QUEUE | BTN_NO_REPEAT);
        btn2.Send(sockfd, my_addr);
      }
    }
    else if (keyMap.find(key.keycode) != keyMap.end())
    {
      json = keyMap[key.keycode];

      int sockfd = socket(AF_INET, SOCK_STREAM, 0);
      if (sockfd < 0)
      {
        cout << "error opening socket" << endl;
        return 1;
      }
      
      struct sockaddr_in serv_addr; 
      memset(&serv_addr, '0', sizeof(serv_addr)); 
      serv_addr.sin_family = AF_INET;
      inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
      serv_addr.sin_port = htons(rpcPort);
 
      if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(struct sockaddr_in)) < 0)
      {
        cout << "error connecting to 127.0.0.1:" << rpcPort << endl;
        return 1;
      }

      write(sockfd, json.c_str(), json.length());
      
      shutdown(sockfd, SHUT_WR);
 
      close(sockfd);
    }
    
    if (logEvents)
      cout << "keycode: " << key.keycode << ", xbmc command: " << json << endl;
  }
    
  return 0;
}

void sighandler(int iSignal)
{
  cout << "signal caught: " <<  iSignal << " - exiting" << endl;
  aborted = true;
}

void parseOptions(int argc, char* argv[])
{
  stringstream ss;
  ss << argv[0];
  ss << " [-d] (daemonize) [-l] (log keypresses) [-f <path>] (path to config file) [-p <port>] (xbmc json-rpc port) [-h] (help)";
  string usage = ss.str();

  for (int i = 1; i < argc; i++)
  {
    if (strcmp(argv[i], "-d") == 0)
      daemonize = true;
    else if (strcmp(argv[i], "-l") == 0)
      logEvents = true;
    else if (strcmp(argv[i], "-f") == 0)
    {
      if (++i == argc)
      {
        cout << usage << endl;
        exit(1);
      }
      else
        configFilePath = argv[i];
    }
    else if (strcmp(argv[i], "-p") == 0)
    {
      if (++i == argc)
      {
        cout << usage << endl;
        exit(1);
      }
      else
      {
        rpcPort = atoi(argv[i]);
        if ((rpcPort < 1) || (rpcPort > 0xffff))
        {
          cout << usage << endl;
          exit(1);
        }
      }
    }
    else
    {
      cout << usage << endl;
      exit((strcmp(argv[i], "-h") == 0) ? 0 : 1);
    }
  }
}

void populateKeyMapFromFile(ifstream &file)
{
  stringstream ss;
  
  bool error = false;
  int i = 1;
  while (file.good())
  {
    unsigned int keycode;
    string assignLiteral = "=>";
    string literal;
    string json;  
        
    if (!(file >> keycode))
    {       
      if (!file.eof()) error = true;
      break;
    }
    if (!(file >> literal))
    {
      error = true;
      break;
    }       
    if (literal != assignLiteral)
    {
      error = true;
      break;
    }
    getline(file, json);
    keyMap[keycode] = json;
    i++;
  }
  
  if (error)
  {
    cout << "could not parse config file line #" << i << endl;
    exit(1);
  }
}

int main (int argc, char* argv[])
{
  daemonize = false;
  logEvents = false;
  configFilePath = "/etc/cecanyway.conf";
  rpcPort = 9090;
  
  parseOptions(argc, argv);  
 
  populateKeyMapDefault();
  populateEventMapDefault();

  ifstream configFileStream(configFilePath.c_str());
  if (configFileStream) {
  
    populateKeyMapFromFile(configFileStream);
    configFileStream.close();
  }
 
  if (daemonize)
  {
    pid_t pid;
    if ((pid = fork()) < 0) 
    {
      cout << "cannot fork" << endl;
      return 1;
    } 
    else if (pid != 0) 
    { 
      // parent
      exit(0);
    }
#ifndef __WINDOWS__    
    // write pid file
    ofstream pidfile;
    pidfile.open("/var/run/cecanyway.pid");
    stringstream ss;
    ss << getpid();
    pidfile << ss.str();
    pidfile.close();
#endif      
    setsid();
  }

  if (signal(SIGINT, sighandler) == SIG_ERR)
  {
    cout << "can't register sighandler" << endl;
    return -1;
  }

  configuration.Clear();
  callbacks.Clear();
  snprintf(configuration.strDeviceName, 13, "cecanyway");
  configuration.clientVersion = CEC_CONFIG_VERSION;
  configuration.bActivateSource = 0;
  callbacks.CBCecKeyPress = &CecKeyPressCB;
  configuration.callbacks = &callbacks;

  configuration.deviceTypes.Add(CEC_DEVICE_TYPE_PLAYBACK_DEVICE);
  configuration.deviceTypes.Add(CEC_DEVICE_TYPE_AUDIO_SYSTEM);

  ICECAdapter *parser = LibCecInitialise(&configuration);
  if (!parser)
  {
#ifdef __WINDOWS__
    cout << "Cannot load libcec.dll" << endl;
#else
    cout << "Cannot load libcec.so" << endl;
#endif
    return 1;
  }

  // init video on targets that need this
  parser->InitVideoStandalone();

  cout << "autodetect serial port: ";
  cec_adapter devices[10];
  uint8_t iDevicesFound = parser->FindAdapters(devices, 10, NULL);
  if (iDevicesFound <= 0)
  {
    cout << "FAILED" << endl;
    UnloadLibCec(parser);
    return 1;
  }
  else
  {
    port = devices[0].comm;
    cout << port << endl;
  }

  cout << "opening a connection to the CEC adapter..." << endl;

  if (!parser->Open(port.c_str()))
  {
    cout << "unable to open the device on port " << port << endl;
    UnloadLibCec(parser);
    return 1;
  }

  pause();

  parser->Close();

  UnloadLibCec(parser);

  return 0;
}
