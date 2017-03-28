#include <iostream>
#include <thread>
#include <sstream>
#include <chrono>
#include <thread>
#include <mutex>
#include <fstream>

#include "./lib/SendCurl.h"

/** Kontainer **/
#include <queue>

using namespace std;

queue<int> id;

// Zamok pre kriticku cast
mutex mtx;
mutex filetx;


// Naplni Frontu FIFO int o dlzke 9 znakov
void NaplnFifo()
{
  int from = 540000000; // 9 - cifier
  int to   = 545999999; // 9 - cifier

  for(; from < to; from++)
  {
    id.push(from);
  }
}

int GetIdFIFO()
{
  int i = 0;

  if(!id.empty())
  {
    mtx.lock(); // zamok pre vyberanie ID z fronty
      i = id.front();
      id.pop();
    mtx.unlock(); // uvolnenie zamku
  }

  return i;
}

void PripravDavku()
{
  int i = 0;
  int id;

  string davka;

  for (i = 0; i < 100; i++)
  {

      id = GetIdFIFO();
      davka += to_string(id) + ",";

  }

  void SendPost(string data);
  SendPost(davka);

}

static void UlozJson(string json)
{
  static int i = 0;
  string nameoffile = "json";
  string konec = ".json";

  ofstream file;

  file.open("./json/" + nameoffile + to_string(i) + konec );


  file << json;

  file.close();

  i = i + 1 ;  
  

}

void SendPost(string id)
{
   
    const char text[] = "&fields=client_language,created_at,last_battle_time&language=cs";
    string field(text);

    const string method   = "/account/info/";

    string post_data = field  + "&account_id="+ id;

    SendCurl send;

    
    string json = send.SendWOT(method, post_data);

  filetx.lock();
    UlozJson(json);
  filetx.unlock();

}


int main()
{
  NaplnFifo();

  int num_thread = 8;
  thread t[num_thread];

      while(id.size() > 0) 
      {
        for(int i = 0; i < num_thread; i++)
        {
          t[i] = thread(PripravDavku);
        }

        for(int i = 0; i < num_thread; i++)
        {
          t[i].join();
        }
      }

  return 0;
}
