#include <iostream>
#include <mutex>
#include "./lib/SendCurl.h"
#include "./lib/Pgsql.h"

#include <thread>
#include <sstream>
#include <fstream>
#include <chrono>

#include <queue>
#include <map>

const char text[] = "&fields=clan_id,members_count,members.account_id,members.account_name,members.joined_at,members.role_i18n&language=cs";

using namespace std;
queue<int>clan_id_queue; // pre posielanie dotazov
queue<int>clan_id_queue2; // pre databazu
queue<int>clan_id_queue3; // pre citanie suborov

// pa.account_id,pa.clan_id as players_clan_id, mr.clan_id as mr_clan_id, nickname,role,odkedy
struct A {
            int account_id = 0;
            int players_clan_id = 0;
            int mr_clan_id = 0;
            string nickname = "";
            string role = "";
            string odkedy  = "";
        };

map<int, A> database_players;

map<int, A> clan_players;


/** Zamok pre kriticku oblast json */
mutex id_lock;
mutex json_lock;
int counter_send_post = 0;

void GetClanId(){

    PGconn *conn;    
    Pgsql *pg = new Pgsql();
    conn = pg->Get();

    string query = "SELECT clan_id FROM clan_all WHERE language != 'cs' AND clan_id NOT IN (SELECT clan_id FROM clan_all_empty) AND clan_id !=  500150759 ORDER BY clan_id DESC LIMIT 253";
    //string query = "SELECT clan_id FROM " + table + " WHERE language = 'cs' AND clan_id NOT IN (SELECT clan_id FROM clan_all_empty) ORDER BY clan_id DESC";
 
    PGresult *result;

    result = PQexec(conn, query.c_str());
    if (PQresultStatus(result) != PGRES_TUPLES_OK)
        {cout << "Chyba select clan_all" <<  PQresultErrorMessage(result) << endl;}
        
    int riadkov     = PQntuples(result);

    cout << "Pocet klanov na spracovanie: " << riadkov << endl;

    int i;

    for(i = 0; i < riadkov; i++)
    {
        clan_id_queue.push(stoi(PQgetvalue(result,i,0)) );
        clan_id_queue2.push(stoi(PQgetvalue(result,i,0)) ); 
        clan_id_queue3.push(stoi(PQgetvalue(result,i,0)) ); 
    }
    delete pg;
    PQclear(result);
    PQfinish(conn);
}

void UlozSubor(string file_name, const char *json_string)
{
    ofstream file;
    file.open ("./tmp/"+file_name, ios::trunc | ios::out);
    file << json_string;
    file.close();
}

string CitajSubor(int id_clan)
{
    string data;    
    ifstream file;
    
    string filename = "./tmp/clan_members_"+ to_string(id_clan) + ".json";
    file.open(filename, ios::in);

    if(file.is_open()) {
       file >> data;
    }
    else {
         cout << "Subor " << filename << " sa nepodarilo otvorit" << endl;
    }
    
    file.close();

    // Vymazanie suboru po precitani
    if( remove( "filename" ) != 0 )
     cout << "Error deleting file " << filename << endl;  

    return data;
}

/** Ziskaj data zo servera a uloz ich do docasneho suboru */
void GetDataFromServer(int a)
{       
    string json_string;
    int x = 1;
    int counter = 0;
    string id = "";
    
    chrono::seconds dura(3); // pausa 3 sec
    id_lock.lock();              
     while(clan_id_queue.size() > 0)
     {
               id += to_string(clan_id_queue.front()) + ",";
               clan_id_queue.pop();                       
          
          counter ++;
          if(counter > 99) break;        
     }
     id_lock.unlock();

     if( id.length() > 5 )
     {
          cout << "COUNTER " << counter << endl;
          id.pop_back();
          string field(text);        
          const string method   = "/clans/info/";
          
          string post_data = field  + "&clan_id="+ id; 
          
          
          SendCurl send;
          do{
          try {
               json_string = send.SendWGN(method, post_data);
               x = 0;
               
          }
          catch(exception& e) {
               cout << e.what() << endl;
               this_thread::sleep_for( dura );
               x = 1;
               //json = send.SendWOT(method, post_data);
          }
          }
          while(x != 0);
          
          /** Ulozenie json do docasneho suboru */
          id_lock.lock();
          counter_send_post ++ ; 
          id_lock.unlock();

          string file_name = "clan_members_"+to_string(counter_send_post)+".json";
          const char *data = json_string.c_str();
          UlozSubor(file_name, data);
          json_string.clear();
     }    
    
}

void GetDataFromWOT(){

     
        
             thread t1(GetDataFromServer,1);
        
        
        
          thread t2(GetDataFromServer,2);
        

        
          thread t3(GetDataFromServer,3);
        

        
          thread t4(GetDataFromServer,4);
        

        
          thread t5(GetDataFromServer,5);
        

        t1.join();t2.join();t3.join(); t4.join();t5.join();

     
        
}

void getDataFromDatabase(string clan_ids) {

     PGconn *conn;    
     Pgsql *pg = new Pgsql();
     conn = pg->Get();

     //string clan_ids = "(500165424,500165423,500165422)";
     string query = "SELECT pa.account_id,pa.clan_id as players_clan_id, mr.clan_id as mr_clan_id, nickname,role,odkedy ";
     query += "FROM players_all pa LEFT JOIN members_role mr ON pa.account_id = mr.account_id WHERE pa.clan_id IN " + clan_ids + " AND dokedy IS NULL ORDER BY mr.clan_id";

     PGresult *result;

     result = PQexec(conn, query.c_str());
     if (PQresultStatus(result) != PGRES_TUPLES_OK)
          {cout << "Chyba select LEFT JOIN" <<  PQresultErrorMessage(result) << endl;}
          
     int riadkov     = PQntuples(result);
     cout << "Pocet hracov na spracovanie: " << riadkov << endl;

     int i;
     A data;
     
     int clan_id;

     for(i = 0; i < riadkov; i++)
     {
        data.account_id = stoi(PQgetvalue(result,i,0));
        data.players_clan_id = stoi(PQgetvalue(result,i,1));
        data.mr_clan_id = stoi(PQgetvalue(result,i,2));
        data.nickname = PQgetvalue(result,i,3);
        data.role = PQgetvalue(result,i,4);
        data.odkedy  = PQgetvalue(result,i,5);        
        
        database_players[data.players_clan_id] = data;
     }
     
    delete pg;
    PQclear(result);
    PQfinish(conn);

}
string StringForDatabase() {
     
     string clan_ids = "(";
     int ic= 0;
      while(clan_id_queue2.size() > 0)
      {       
          int id = clan_id_queue2.front();          
          clan_id_queue2.pop();  

          clan_ids += to_string(id) + ",";
          
          ic ++;
          
      }  
     clan_ids.pop_back(); // odstran ciarku za poslednym cislom

     clan_ids += ")";

     return clan_ids;

}

void ReadDataFromFile() {

     int i;
     string json_data;
     void SpracujData(string id_clan);

     
     for( i = 1; i < counter_send_post ; i++) {
          
          json_data = CitajSubor(i);          

          SpracujData(json_data);
          json_data.clear();
     }
}

void SpracujData(string json_data) {
     cout <<  json_data << endl;
}


int main() {
    

     // Ziskam do fronty vsetky clan_id
     GetClanId();
     GetDataFromWOT();
     
     string clan_ids = StringForDatabase();
     getDataFromDatabase(clan_ids);

     ReadDataFromFile();


     char i;
     cout << "Stlac nieco -> ";
     cin >> i  ;

}