#include <iostream>
#include <map>
#include <chrono>
#include <mutex>
#include <thread>

/** Kontainer **/
#include <queue>
#include <map>

/**  Moje kniznice  */
#include "./lib/SendCurl.h"
#include "./lib/Pgsql.h"
#include "./json/src/json.hpp"


/** Co budem pozadovat zo serveru */
string field = "&fields=stronghold_skirmish.battles,stronghold_skirmish.wins,stronghold_skirmish.damage_dealt,stronghold_skirmish.spotted,stronghold_skirmish.frags,stronghold_skirmish.dropped_capture_points,"
"globalmap.battles,globalmap.wins,globalmap.damage_dealt,globalmap.spotted,globalmap.frags,globalmap.dropped_capture_points,"
"stronghold_defense.battles,stronghold_defense.wins,stronghold_defense.damage_dealt,stronghold_defense.spotted,stronghold_defense.frags,stronghold_defense.dropped_capture_points,"
"all.battles,all.wins,all.damage_dealt,all.spotted,all.frags,all.dropped_capture_points,tank_id";

const string method   = "/tanks/stats/";

PGconn *conn;
int riadkov;

/** Kontajner na account_id */
queue<int> account_id;

/** Kontajner na ziskane jsony */
map<int,string> json_map;

/** Struktura na data z databazy */ 
struct tank_data {
    int battles;
    int wins;
    int spot;
    int dmg;
    int frags;
    int dcp;
};
map<int,tank_data> all;
map<int,tank_data> skirmish;
map<int,tank_data> defense;
map<int,tank_data> globalmap;


/** Zamok pre kriticku oblast json */
mutex json_lock;

/** Vytvor jedno spojenie do databazy */
void VytvorSpojenie()
{
    Pgsql *pg = new Pgsql;
    conn = pg->Get();
}

void VacuumAnalyze()
{
    string query = "VACUUM ANALYZE pvs_all_01";
        PQsendQuery(conn,query.c_str());
    query = "VACUUM ANALYZE pvs_all_02";
        PQsendQuery(conn,query.c_str());
    query = "VACUUM ANALYZE pvs_all_03";
        PQsendQuery(conn,query.c_str());
    query = "VACUUM ANALYZE pvs_all_04";
        PQsendQuery(conn,query.c_str());
    query = "VACUUM ANALYZE pvs_all_05";
        PQsendQuery(conn,query.c_str());
    
}

/** Nahraj account_id do fronty */
void GetAccountId() 
{
    PGresult *result;
    string query    = "SELECT account_id FROM cz_players";
    result          = PQexec(conn, query.c_str());
         if (PQresultStatus(result) != PGRES_TUPLES_OK)
                {cout << "GetPlayers: " <<  PQresultErrorMessage(result) << endl;}

    riadkov     = PQntuples(result);

    for(int i = 0; i < riadkov; i++)
    {
        account_id.push(stoi(PQgetvalue(result,i,0)));
        
    }
    PQclear(result);
}

void GetDataFromSever()
{       
    string json;
    int x = 1;
    chrono::seconds dura(3); // pausa 3 sec

    if(account_id.size() > 0)
    {
        json_lock.lock();
        int id = account_id.front();
        account_id.pop();
        json_lock.unlock();

        string post_data = field+"&account_id="+to_string(id);
        
        SendCurl send;
        do{
            try {
                json = send.SendWOT(method, post_data);
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

        json_lock.lock();
        json_map[id] = json;
        json_lock.unlock();

    }
}

/** Ziskanie surovych dat a ulozenie do kontainera, 10 hracov naraz */
void GetJson()
{
        thread t1(GetDataFromSever);
        thread t2(GetDataFromSever);
        thread t3(GetDataFromSever);
        thread t4(GetDataFromSever);
        thread t5(GetDataFromSever);
        thread t6(GetDataFromSever);
        thread t7(GetDataFromSever);
        thread t8(GetDataFromSever);
        thread t9(GetDataFromSever);
        thread t10(GetDataFromSever);
    
        t1.join();t2.join();t3.join();t4.join();t5.join();t6.join();t7.join();t8.join();t9.join();t10.join();
}

PGresult *Database(int id, string table)
{
    PGresult *result;

    string query    = "SELECT * FROM "+table+" WHERE account_id= "+to_string(id) ;
    result          = PQexec(conn, query.c_str());
    if (PQresultStatus(result) != PGRES_TUPLES_OK)
          {cout << "Data z tabulku "+table+" neprisli" <<  PQresultErrorMessage(result) << endl;}

    return result;
}

void GetDatabaseData()
{
    PGresult *Database(int id, string table);
    void UrobPvsAll(string json);
    PGresult *result;
    tank_data tdata;
    map<int,string>::iterator it;
    string json;
    
    int id,riadkov,i;

    for(it = json_map.begin(); it != json_map.end(); it++ ) 
    {
        id = it->first;

        /** Tabulka pvs_all */
        result = Database(id, "pvs_all");
        riadkov = PQntuples(result);

        for(i = 0; i < riadkov; i++) {
            tdata.battles   = stoi(PQgetvalue(result,i,2));
            tdata.wins      = stoi(PQgetvalue(result,i,3));
            tdata.spot      = stoi(PQgetvalue(result,i,4));
            tdata.dmg       = stoi(PQgetvalue(result,i,5));
            tdata.dcp       = stoi(PQgetvalue(result,i,6));

            all[stoi(PQgetvalue(result,i,1))]  = tdata;
            
        }
        PQclear(result);

        /** Tabulka pvs_skirmish */
        result = Database(id, "pvs_skirmish");
        riadkov = PQntuples(result);

        for(i = 0; i < riadkov; i++) {
            tdata.battles   = stoi(PQgetvalue(result,i,2));
            tdata.wins      = stoi(PQgetvalue(result,i,3));
            tdata.spot      = stoi(PQgetvalue(result,i,4));
            tdata.dmg       = stoi(PQgetvalue(result,i,5));
            tdata.dcp       = stoi(PQgetvalue(result,i,6));

            skirmish[stoi(PQgetvalue(result,i,1))]  = tdata;
            
        }
        PQclear(result);

        /** Tabulka pvs_defense */
        result = Database(id, "pvs_defense");
        riadkov = PQntuples(result);

        for(i = 0; i < riadkov; i++) {
            tdata.battles   = stoi(PQgetvalue(result,i,2));
            tdata.wins      = stoi(PQgetvalue(result,i,3));
            tdata.spot      = stoi(PQgetvalue(result,i,4));
            tdata.dmg       = stoi(PQgetvalue(result,i,5));
            tdata.dcp       = stoi(PQgetvalue(result,i,6));

            defense[stoi(PQgetvalue(result,i,1))]  = tdata;
            
        }
        PQclear(result);

        /** Tabulka pvs_globalmap */
        result = Database(id, "pvs_globalmap");
        riadkov = PQntuples(result);

        for(i = 0; i < riadkov; i++) {
            tdata.battles   = stoi(PQgetvalue(result,i,2));
            tdata.wins      = stoi(PQgetvalue(result,i,3));
            tdata.spot      = stoi(PQgetvalue(result,i,4));
            tdata.dmg       = stoi(PQgetvalue(result,i,5));
            tdata.dcp       = stoi(PQgetvalue(result,i,6));

            globalmap[stoi(PQgetvalue(result,i,1))]  = tdata;
            
        }
        PQclear(result);
        
        json = it->second;
        
        UrobPvsAll(json);

        
    }


}




void UrobPvsAll(string json_data)
{
       
    using json = nlohmann::json;
    string insert_pvs_all="";
    string insert_pvs_all_history = "";
    json j,c;
    string account_id;
    json js = json::parse(json_data); json_data.clear();

    js      = js["data"];
    
    for (auto& x : json::iterator_wrapper(js))
    {
        account_id = x.key();
        cout << account_id << endl;
        j = x.value();
    }   
       
    for (auto& y : json::iterator_wrapper(j))
    {
          c = y.value();  
          int tank_id = c["tank_id"].get<int>();
        
          if(all.count(tank_id) == 1 ) // ak najde taky tank_id v kontajnery
          {
            if(all[tank_id].battles != c["all"]["battles"].get<int>() )
            {
                cout << c["all"]["battles"].get<int>() - all[tank_id].battles << endl;;
            }
          }
          else // ak nenajde tanky tak ich da na pridanie
          {
            cout << c << endl; 
            insert_pvs_all += "("+account_id+"," 
                                    + to_string(c["tank_id"].get<int>()) +","
                                    + to_string(c["all"]["battles"].get<int>())+ ","
                                    + to_string(c["all"]["wins"].get<int>())+ ","
                                    + to_string(c["all"]["spotted"].get<int>())+ ","
                                    + to_string(c["all"]["damage_dealt"].get<int>())+ ","
                                    + to_string(c["all"]["frags"].get<int>())+ ","
                                    + to_string(c["all"]["dropped_capture_points"].get<int>())+ "),";         
          }
          
    }
       
    cout << insert_pvs_all << endl;
        
    
    
}

int main()
{
    VytvorSpojenie();
    GetAccountId();
    
    while(account_id.size() > 0)
    {
        GetJson();
        GetDatabaseData();

        return 0;

    }
    

    cout << json_map.size() << endl;
    return 0;
}