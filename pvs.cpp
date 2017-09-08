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

/** Pocitadla pre dotazy */
int insert_all_count = 0; int insert_all_h_count = 0;
int insert_skirmish_count = 0; int insert_skirmish_h_count = 0;
int insert_defense_count = 0;   int insert_defense_h_count = 0;
int insert_map_count = 0; int insert_map_h_count = 0;

int counter[] = {0,0,0,0,0,0,0,0};

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
    void UrobPvsAll(string json, string *i, string *ih, string *u);
    void UrobPvsskirmish(string json, string *i, string *ih, string *u);
    void UrobPvsdefense(string json, string *i, string *ih, string *u);
    void UrobPvsMap(string json, string *i, string *ih, string *u);

    void InsertInTable(string *data, string table);
    void UpdateTable(string *data, string table);
    
    PGresult *result;
    tank_data tdata;
    map<int,string>::iterator it;
    string json;

    /** Dotazy do tabuliek pvs_all Random */
    string insert_pvs_all=""; string *ipa; ipa = &insert_pvs_all;
    string insert_pvs_all_history = ""; string *ipah; ipah = &insert_pvs_all_history;
    string update_pvs_all = ""; string *upa; upa = &update_pvs_all; 

    string insert_pvs_skirmish=""; string *ips; ips = &insert_pvs_skirmish;
    string insert_pvs_skirmish_history = ""; string *ipsh; ipsh = &insert_pvs_skirmish_history;
    string update_pvs_skirmish = ""; string *ups; ups = &update_pvs_skirmish;
    
    string insert_pvs_defense=""; string *ipd; ipd = &insert_pvs_defense;
    string insert_pvs_defense_history = ""; string *ipdh; ipdh = &insert_pvs_defense_history;
    string update_pvs_defense = ""; string *upd; upd = &update_pvs_defense;

    string insert_pvs_globalmap=""; string *ipm; ipm = &insert_pvs_globalmap;
    string insert_pvs_globalmap_history = ""; string *ipmh; ipmh = &insert_pvs_globalmap_history;
    string update_pvs_globalmap = ""; string *upm; upm = &update_pvs_globalmap; 

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
            tdata.frags     = stoi(PQgetvalue(result,i,6));
            tdata.dcp       = stoi(PQgetvalue(result,i,7));

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
            tdata.frags     = stoi(PQgetvalue(result,i,6));
            tdata.dcp       = stoi(PQgetvalue(result,i,7));

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
            tdata.frags     = stoi(PQgetvalue(result,i,6));
            tdata.dcp       = stoi(PQgetvalue(result,i,7));

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
            tdata.frags     = stoi(PQgetvalue(result,i,6));
            tdata.dcp       = stoi(PQgetvalue(result,i,7));

            globalmap[stoi(PQgetvalue(result,i,1))]  = tdata;
            
        }
        PQclear(result);
        
        json = it->second;
        
        UrobPvsAll(json, ipa, ipah, upa);
        UrobPvsskirmish(json, ips, ipsh, ups);
        UrobPvsdefense(json, ipd, ipdh, upd);
        UrobPvsMap(json, ipm, ipmh, upm);
       
        all.clear();skirmish.clear();globalmap.clear();defense.clear();
        json_map.clear();json.clear();
    }
   
    /** Odosielanie pripravenych dotazov */
    
    /** INSERT */
    if(insert_all_count > 0) {
        InsertInTable(ipa,"pvs_all");
    }
    if(insert_all_h_count > 0) {
        InsertInTable(ipah,"pvs_all_history");
        UpdateTable(upa,"pvs_all");
    }
    if(insert_skirmish_count > 0) {
        InsertInTable(ips,"pvs_skirmish");
    }
    if(insert_skirmish_h_count > 0) {
        InsertInTable(ipsh,"pvs_skirmish_history");
        UpdateTable(ups,"pvs_skirmish");
    }
    if(insert_defense_count > 0) {
        InsertInTable(ipd,"pvs_defense");
    }
    if(insert_defense_h_count > 0) {
        InsertInTable(ipdh,"pvs_defense_history");
        UpdateTable(upd,"pvs_defense");
    }
    if(insert_map_count > 0) {
        InsertInTable(ipm,"pvs_globalmap");
    }
    if(insert_map_h_count > 0) {
        InsertInTable(ipmh,"pvs_globalmap_history");
        UpdateTable(upm,"pvs_all");
    }

    counter[0]  += insert_all_count ;
    counter[1]  += insert_all_h_count;
    counter[2]  += insert_skirmish_count;
    counter[3]  += insert_skirmish_h_count;
    counter[4]  += insert_defense_count;
    counter[5]  += insert_defense_h_count;
    counter[6]  += insert_map_count;
    counter[7]  += insert_map_h_count;

    /** Mazanie pocitadiel */
    insert_all_count = insert_all_h_count = insert_skirmish_count = insert_skirmish_h_count = 0;
    insert_defense_count = insert_defense_h_count = insert_map_count = insert_map_h_count = 0;

    /** Mazanie stringov */
    insert_pvs_all.clear();insert_pvs_all_history.clear(); update_pvs_all.clear();
    insert_pvs_skirmish.clear();insert_pvs_skirmish_history.clear();update_pvs_skirmish.clear();
    insert_pvs_defense.clear();insert_pvs_defense_history.clear();update_pvs_defense.clear();
    insert_pvs_globalmap.clear();insert_pvs_globalmap_history.clear();update_pvs_globalmap.clear();

    /** Mazanie map */
    all.clear();skirmish.clear();defense.clear();globalmap.clear();

}

void InsertInTable(string *data, string table)
{
    string values = *data;
    values.pop_back();
    PGresult *result;

    string query = "INSERT INTO "+table+" VALUES " + values ;
    
    result = PQexec(conn, query.c_str());
    if (PQresultStatus(result) != PGRES_COMMAND_OK)
    {cout << "Chyba insertu do tabulky " + table <<  PQresultErrorMessage(result) << endl;}

    PQclear(result);

}

void UpdateTable(string *data, string table)
{
    string values = *data;
    values.pop_back();
    PGresult *result;

    string query = "UPDATE "+table+" as p SET battles = u.battles, wins = u.wins, spotted = u.spotted, damage_dealt = u.damage_dealt, frags = u.frags, dropped_capture_points = u.dropped_capture_points FROM (VALUES " + values + ") as u(account_id,tank_id,battles,wins,spotted,damage_dealt,frags,dropped_capture_points) WHERE p.account_id = u.account_id AND p.tank_id = u.tank_id";

    result = PQexec(conn, query.c_str());
    if (PQresultStatus(result) != PGRES_COMMAND_OK)
    {cout << "Chyba update tabulky "+table <<  PQresultErrorMessage(result) << endl;}

    PQclear(result);
}


void UrobPvsAll(string json_data,string *insert_pvs_all, string *insert_pvs_all_history, string *update_pvs_all)
{
       
    using json = nlohmann::json;
    
    json j,c;
    string account_id;
    json js = json::parse(json_data); json_data.clear();

    js      = js["data"];
    
    for (auto& x : json::iterator_wrapper(js))
    {
        account_id = x.key();
        j = x.value();
    }   
       
    for (auto& y : json::iterator_wrapper(j))
    {
          c = y.value();  
          int tank_id = c["tank_id"].get<int>();
          
          if(all.count(tank_id) > 0 ) // ak najde taky tank_id v kontajnery
          {
            
            if(all[tank_id].battles != c["all"]["battles"].get<int>() ) // Ak su rozdielne bitky tak to zaznamenaj
            {
                /** Vloz data do databazy do pvs_all_history */
                *insert_pvs_all_history +=  "("  +account_id+"," 
                                                + to_string(c["tank_id"].get<int>()) +","
                                                + to_string(c["all"]["battles"].get<int>() - all[tank_id].battles)+ ","
                                                + to_string(c["all"]["wins"].get<int>() - all[tank_id].wins  )+ ","
                                                + "current_date - interval '1 days',"
                                                + to_string(c["all"]["spotted"].get<int>()  - all[tank_id].spot )+ ","
                                                + to_string(c["all"]["damage_dealt"].get<int>() - all[tank_id].dmg )+ ","
                                                + to_string(c["all"]["frags"].get<int>() - all[tank_id].frags )+ ","
                                                + to_string(c["all"]["dropped_capture_points"].get<int>() - all[tank_id].dcp )+ "),";  
                
                /** A update data v databaze v pvs_all */
                *update_pvs_all += "("           +account_id+"," 
                                                + to_string(c["tank_id"].get<int>()) +","
                                                + to_string(c["all"]["battles"].get<int>())+ ","
                                                + to_string(c["all"]["wins"].get<int>())+ ","
                                                + to_string(c["all"]["spotted"].get<int>())+ ","
                                                + to_string(c["all"]["damage_dealt"].get<int>())+ ","
                                                + to_string(c["all"]["frags"].get<int>())+ ","
                                                + to_string(c["all"]["dropped_capture_points"].get<int>())+ "),";

                insert_all_h_count ++; /** update pvs_all a insertov do pvs_all_historie musi by rovnako, tak ich nemusim pocitat zvlast */
            }      
          }
          else // ak nenajde tanky tak ich da na pridanie
          {
            *insert_pvs_all += "("+account_id+"," 
                                    + to_string(c["tank_id"].get<int>()) +","
                                    + to_string(c["all"]["battles"].get<int>())+ ","
                                    + to_string(c["all"]["wins"].get<int>())+ ","
                                    + to_string(c["all"]["spotted"].get<int>())+ ","
                                    + to_string(c["all"]["damage_dealt"].get<int>())+ ","
                                    + to_string(c["all"]["frags"].get<int>())+ ","
                                    + to_string(c["all"]["dropped_capture_points"].get<int>())+ "),";         
            insert_all_count ++;
          }
          
    }
    
}

void UrobPvsskirmish(string json_data,string *insert_pvs_skirmish, string *insert_pvs_skirmish_history, string *update_pvs_skirmish)
{
       
    using json = nlohmann::json;
    json j,c;
    string account_id;
    json js = json::parse(json_data); json_data.clear();

    js      = js["data"];
    
    for (auto& x : json::iterator_wrapper(js))
    {
        account_id = x.key();
        j = x.value();
    }   
       
    for (auto& y : json::iterator_wrapper(j))
    {
          c = y.value();  
          int tank_id = c["tank_id"].get<int>();
          
          if(skirmish.count(tank_id) > 0 ) // ak najde taky tank_id v kontajnery
          {
            
            if(skirmish[tank_id].battles != c["stronghold_skirmish"]["battles"].get<int>() ) // Ak su rozdielne bitky tak to zaznamenaj
            {
                /** Vloz data do databazy do pvs_skirmish_history */
                *insert_pvs_skirmish_history +=  "("  +account_id+"," 
                                                + to_string(c["tank_id"].get<int>()) +","
                                                + to_string(c["stronghold_skirmish"]["battles"].get<int>() - skirmish[tank_id].battles)+ ","
                                                + to_string(c["stronghold_skirmish"]["wins"].get<int>() - skirmish[tank_id].wins  )+ ","
                                                + "current_date - interval '1 days',"
                                                + to_string(c["stronghold_skirmish"]["spotted"].get<int>()  - skirmish[tank_id].spot )+ ","
                                                + to_string(c["stronghold_skirmish"]["damage_dealt"].get<int>() - skirmish[tank_id].dmg )+ ","
                                                + to_string(c["stronghold_skirmish"]["frags"].get<int>() - skirmish[tank_id].frags )+ ","
                                                + to_string(c["stronghold_skirmish"]["dropped_capture_points"].get<int>() - skirmish[tank_id].dcp )+ "),";  
                
                /** A update data v databaze v pvs_skirmish */
                *update_pvs_skirmish += "("           +account_id+"," 
                                                + to_string(c["tank_id"].get<int>()) +","
                                                + to_string(c["stronghold_skirmish"]["battles"].get<int>())+ ","
                                                + to_string(c["stronghold_skirmish"]["wins"].get<int>())+ ","
                                                + to_string(c["stronghold_skirmish"]["spotted"].get<int>())+ ","
                                                + to_string(c["stronghold_skirmish"]["damage_dealt"].get<int>())+ ","
                                                + to_string(c["stronghold_skirmish"]["frags"].get<int>())+ ","
                                                + to_string(c["stronghold_skirmish"]["dropped_capture_points"].get<int>())+ "),";

                insert_skirmish_h_count ++; /** update pvs_skirmish a insertov do pvs_skirmish_historie musi by rovnako, tak ich nemusim pocitat zvlast */
            }      
          }
          else // ak nenajde tanky tak ich da na pridanie
          {
            *insert_pvs_skirmish += "("+account_id+"," 
                                    + to_string(c["tank_id"].get<int>()) +","
                                    + to_string(c["stronghold_skirmish"]["battles"].get<int>())+ ","
                                    + to_string(c["stronghold_skirmish"]["wins"].get<int>())+ ","
                                    + to_string(c["stronghold_skirmish"]["spotted"].get<int>())+ ","
                                    + to_string(c["stronghold_skirmish"]["damage_dealt"].get<int>())+ ","
                                    + to_string(c["stronghold_skirmish"]["frags"].get<int>())+ ","
                                    + to_string(c["stronghold_skirmish"]["dropped_capture_points"].get<int>())+ "),";         
            insert_skirmish_count ++;
          }
          
    }
    
}

void UrobPvsdefense(string json_data,string *insert_pvs_defense, string *insert_pvs_defense_history, string *update_pvs_defense)
{
       
    using json = nlohmann::json;
    json j,c;
    string account_id;
    json js = json::parse(json_data); json_data.clear();

    js      = js["data"];
    for (auto& x : json::iterator_wrapper(js))
    {
        account_id = x.key();
        j = x.value();
    }   
       
    for (auto& y : json::iterator_wrapper(j))
    {
          c = y.value();  
          int tank_id = c["tank_id"].get<int>();
      
          
          if(defense.count(tank_id) > 0 ) // ak najde taky tank_id v kontajnery
          {
            
            
            if(defense[tank_id].battles != c["stronghold_defense"]["battles"].get<int>() ) // Ak su rozdielne bitky tak to zaznamenaj
            {
                /** Vloz data do databazy do pvs_defense_history */
                *insert_pvs_defense_history +=  "("  +account_id+"," 
                                                + to_string(c["tank_id"].get<int>()) +","
                                                + to_string(c["stronghold_defense"]["battles"].get<int>() - defense[tank_id].battles)+ ","
                                                + to_string(c["stronghold_defense"]["wins"].get<int>() - defense[tank_id].wins  )+ ","
                                                + "current_date - interval '1 days',"
                                                + to_string(c["stronghold_defense"]["spotted"].get<int>()  - defense[tank_id].spot )+ ","
                                                + to_string(c["stronghold_defense"]["damage_dealt"].get<int>() - defense[tank_id].dmg )+ ","
                                                + to_string(c["stronghold_defense"]["frags"].get<int>() - defense[tank_id].frags )+ ","
                                                + to_string(c["stronghold_defense"]["dropped_capture_points"].get<int>() - defense[tank_id].dcp )+ "),";  
                
                /** A update data v databaze v pvs_defense */
                *update_pvs_defense += "("           +account_id+"," 
                                                + to_string(c["tank_id"].get<int>()) +","
                                                + to_string(c["stronghold_defense"]["battles"].get<int>())+ ","
                                                + to_string(c["stronghold_defense"]["wins"].get<int>())+ ","
                                                + to_string(c["stronghold_defense"]["spotted"].get<int>())+ ","
                                                + to_string(c["stronghold_defense"]["damage_dealt"].get<int>())+ ","
                                                + to_string(c["stronghold_defense"]["frags"].get<int>())+ ","
                                                + to_string(c["stronghold_defense"]["dropped_capture_points"].get<int>())+ "),";

                insert_defense_h_count ++; /** update pvs_defense a insertov do pvs_defense_historie musi by rovnako, tak ich nemusim pocitat zvlast */
            }      
          }
          else // ak nenajde tanky tak ich da na pridanie
          {
            *insert_pvs_defense += "("+account_id+"," 
                                    + to_string(c["tank_id"].get<int>()) +","
                                    + to_string(c["stronghold_defense"]["battles"].get<int>())+ ","
                                    + to_string(c["stronghold_defense"]["wins"].get<int>())+ ","
                                    + to_string(c["stronghold_defense"]["spotted"].get<int>())+ ","
                                    + to_string(c["stronghold_defense"]["damage_dealt"].get<int>())+ ","
                                    + to_string(c["stronghold_defense"]["frags"].get<int>())+ ","
                                    + to_string(c["stronghold_defense"]["dropped_capture_points"].get<int>())+ "),";         
            insert_defense_count ++;
          }
          
    }
    
}

void UrobPvsMap(string json_data,string *insert_pvs_globalmap, string *insert_pvs_globalmap_history, string *update_pvs_globalmap)
{
       
    using json = nlohmann::json;
    
    json j,c;
    string account_id;
    json js = json::parse(json_data); json_data.clear();

    js      = js["data"];
    
    for (auto& x : json::iterator_wrapper(js))
    {
        account_id = x.key();
        j = x.value();
    }   
       
    for (auto& y : json::iterator_wrapper(j))
    {
          c = y.value();  
          int tank_id = c["tank_id"].get<int>();
          
          if(globalmap.count(tank_id) > 0 ) // ak najde taky tank_id v kontajnery
          {
            
            if(globalmap[tank_id].battles != c["globalmap"]["battles"].get<int>() ) // Ak su rozdielne bitky tak to zaznamenaj
            {
                /** Vloz data do databazy do pvs_globalmap_history */
                *insert_pvs_globalmap_history +=  "("  +account_id+"," 
                                                + to_string(c["tank_id"].get<int>()) +","
                                                + to_string(c["globalmap"]["battles"].get<int>() - globalmap[tank_id].battles)+ ","
                                                + to_string(c["globalmap"]["wins"].get<int>() - globalmap[tank_id].wins  )+ ","
                                                + "current_date - interval '1 days',"
                                                + to_string(c["globalmap"]["spotted"].get<int>()  - globalmap[tank_id].spot )+ ","
                                                + to_string(c["globalmap"]["damage_dealt"].get<int>() - globalmap[tank_id].dmg )+ ","
                                                + to_string(c["globalmap"]["frags"].get<int>() - globalmap[tank_id].frags )+ ","
                                                + to_string(c["globalmap"]["dropped_capture_points"].get<int>() - globalmap[tank_id].dcp )+ "),";  
                
                /** A update data v databaze v pvs_globalmap */
                *update_pvs_globalmap += "("           +account_id+"," 
                                                + to_string(c["tank_id"].get<int>()) +","
                                                + to_string(c["globalmap"]["battles"].get<int>())+ ","
                                                + to_string(c["globalmap"]["wins"].get<int>())+ ","
                                                + to_string(c["globalmap"]["spotted"].get<int>())+ ","
                                                + to_string(c["globalmap"]["damage_dealt"].get<int>())+ ","
                                                + to_string(c["globalmap"]["frags"].get<int>())+ ","
                                                + to_string(c["globalmap"]["dropped_capture_points"].get<int>())+ "),";

                insert_map_h_count ++; /** update pvs_globalmap a insertov do pvs_globalmap_historie musi by rovnako, tak ich nemusim pocitat zvlast */
            }      
          }
          else // ak nenajde tanky tak ich da na pridanie
          {
            *insert_pvs_globalmap += "("+account_id+"," 
                                    + to_string(c["tank_id"].get<int>()) +","
                                    + to_string(c["globalmap"]["battles"].get<int>())+ ","
                                    + to_string(c["globalmap"]["wins"].get<int>())+ ","
                                    + to_string(c["globalmap"]["spotted"].get<int>())+ ","
                                    + to_string(c["globalmap"]["damage_dealt"].get<int>())+ ","
                                    + to_string(c["globalmap"]["frags"].get<int>())+ ","
                                    + to_string(c["globalmap"]["dropped_capture_points"].get<int>())+ "),";         
            insert_map_count ++;
          }
          
    }
    
}

int Spracuj()
{   
    GetDatabaseData();

    return 0;
}   


int main()
{
    time_t start, stop;
    time(&start);
    cout << "*********************************"<< endl;
    cout << endl << "Program zacal pracovat: " << ctime(&start) << endl;
    
    VytvorSpojenie();
    GetAccountId();

    
    while(account_id.size() > 0)
    {
        /** Ziskaj json */
        using namespace std::chrono;
        high_resolution_clock::time_point t1 = high_resolution_clock::now();
        GetJson();
        high_resolution_clock::time_point t2 = high_resolution_clock::now();

        /** Spracuj json a uloz do databazy*/
        high_resolution_clock::time_point d1 = high_resolution_clock::now();
        Spracuj();
        high_resolution_clock::time_point d2 = high_resolution_clock::now();

        /**
        duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
        duration<double> time_spand = duration_cast<duration<double>>(d2 - d1);
        cout << "Cas json: " << time_span.count() << endl;
        cout << "Cas spracovania: " << time_spand.count() << endl;
        cout << "Pocet dotazov: " << counter[0]+counter[1]+counter[2]+counter[3]+counter[4]+counter[5]+counter[6]+counter[7] << endl;
        cout << all.size() << endl;
        */
    }
    
    time(&stop);
    cout << "Celkovy pocer dotazov: "    << counter[0]+counter[1]+counter[2]+counter[3]+counter[4]+counter[5]+counter[6]+counter[7] << endl;
    cout << "Program skoncil pracovat: " << ctime(&stop) << endl;
    return 0;
}