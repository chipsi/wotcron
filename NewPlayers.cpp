#include <iostream>
#include <thread>
#include <sstream>
#include <chrono>
#include <thread>
#include <mutex>


/** Kontainer **/
#include <queue>

using namespace std;

queue<int> id;

// Zamok pre kriticku cast
mutex mtx;


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
  cout << davka << endl;

}


int main()
{
  NaplnFifo();

  int num_thread = 8;
  thread t[num_thread];


  for(int i = 0; i < num_thread; i++)
  {
    t[i] = thread(PripravDavku);
  }

  for(int i = 0; i < num_thread; i++)
  {
    t[i].join();
  }

    cout << id.size() << endl;
  return 0;
}
