/*
*   Program na spracovanie informacii o hracoch.
*   Hlada client language, global rating ....   
*
*/

#include <iostream>

/** Kontainer **/
#include <stack>
#include <map>

/**  Moje kniznice  */
#include "./lib/SendCurl.h"
#include "./lib/Pgsql.h"
#include "./json/src/json.hpp"

using namespace std;

// Co budem pozadovat od serveru
const string field = "&fields=account_id,client_language,global_rating,created_at,logout_at,last_battle_time";
const string method   = "/account/info/";
string post;
string json_data;

int insert_counter = 0;
int update_counter = 0;

struct player {
  int account_id ;
  int global_rating;
  string client_language;
  int created_at ;
  int last_battle_time;
  int logout_at;
};

player player_data;

typedef map<int,player> mymap;
mymap maps_data;

stack<int>account_ids;
PGconn *conn;

void PripojDatabase() {
    Pgsql *trieda  = new Pgsql();
    conn = trieda->Get();

    /////////// Priprav dotaz
    PGresult *result;    
    const char* query  = "SELECT account_id,client_language,global_rating,extract(epoch from logout_at),extract(epoch from created_at),extract(epoch from last_battle_time) FROM players_info WHERE account_id = $1";

    result = PQprepare(conn,"players_info",query,1,NULL);
    if (PQresultStatus(result) != PGRES_COMMAND_OK)
    {
        fprintf(stderr, "Prepared statment players_info je chybny: %s", PQerrorMessage(conn));
        PQclear(result);
    }
    PQclear(result);
}


void NaplnStack() {

    PGresult *result;    
    const char *sql = "SELECT account_id FROM players_all ORDER BY account_id DESC";

    result = PQexec(conn, sql);
    int ntuples = PQntuples(result);   

    for(int i = 0; i < ntuples; i++)
    {
        account_ids.push(stoi(PQgetvalue(result, i, 0)));        
    }
    
    cout << "Celkom hracov spracovanie: " << account_ids.size() << endl;

    PQclear(result);
}

void VaccuumAnalyze() {
    PGresult *result;    
    const char *sql = "VACUUM FULL ANALYZE players_info";

    result = PQexec(conn, sql);

    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
        fprintf(stderr, "VACUUM ANALYZE FULL players_info bolo chybne: %s", PQerrorMessage(conn));
        PQclear(result);
    }
    cout << "Prebehlo Vacuum Analyze players_info2" << endl;
    PQclear(result);
}

void GetPost() {

    string ids;
    int i = 0;   

    while(!account_ids.empty()) {
        
        ids += to_string(account_ids.top())+ ",";
        account_ids.pop();
        i ++;
        if(i == 100) break;
    }

    // Vymaze poslednu ciarku
    ids.erase(ids.end()-1);

    /* POST data pre poslanie na server */
    post = field+"&account_id="+ids; 
    ids.clear();         
}

void SendPost() {

    SendCurl *send = new SendCurl;  

    json_data = send->SendWOT(method, post);        

    post.clear();
    delete send;      
}

void ParseJson() {
    using json = nlohmann::json;
    
    json js = json::parse(json_data);    
    string status = js["status"].get<string>();

    if(status.compare("ok") == 0) {

        json udaje = js["data"];
        
        for (auto& x : json::iterator_wrapper(udaje))
        {
            if(x.value() != NULL)
            {
                json j = x.value();                
                    
                if(!j.is_null())
                {
                    player_data.account_id = j["account_id"].get<int>();
                    player_data.global_rating = j["global_rating"].get<int>();
                    player_data.client_language = j["client_language"].get<string>();
                    player_data.created_at = j["created_at"].get<int>();
                    player_data.last_battle_time = j["last_battle_time"].get<int>();
                    player_data.logout_at = j["logout_at"].get<int>();  

                    maps_data[j["account_id"].get<int>()] = player_data;
                }                
            }            
        }        
    }
    
    status.clear();js.clear();

}

void CheckData () {

    PGresult *prepared, *update_q, *insert_q;       

    const char *paramValues[1];
    int riadkov;   


    for(map<int,player>::iterator it = maps_data.begin(); it != maps_data.end(); it++) {
            
            //string a_id = to_string(it->first);
            paramValues[0] = to_string(it->first).c_str();
            
    
            prepared  = PQexecPrepared(conn,"players_info",1,paramValues,NULL,NULL,0);
                if (PQresultStatus(prepared) != PGRES_TUPLES_OK)
                    {fprintf(stderr, "Select statment players_info je chybny: %s ", PQresultErrorMessage(prepared));}

            riadkov  = PQntuples(prepared);                        

            // Ak mam zaznam UPDATE
            if(riadkov == 1) {
                int up = 0;
                
                if(it->second.client_language.compare( PQgetvalue(prepared,0,1)) != 0) {
                    up = 1;
                }
                if(it->second.global_rating != stoi(PQgetvalue(prepared,0,2))) {
                    up = 1;
                }
                if(it->second.logout_at != stoi(PQgetvalue(prepared,0,2))) {
                    up = 1;
                }
                
                if(it->second.last_battle_time != stoi(PQgetvalue(prepared,0,5))) {
                    up = 1;
                }

                if(up == 1) {
                    string sql = "UPDATE players_info SET client_language = '" + it->second.client_language + "', global_rating = " + to_string( it->second.global_rating ) + ", logout_at = to_timestamp(" + to_string(it->second.logout_at) + "), created_at = to_timestamp("+ to_string(it->second.created_at)+"),";
                    sql += " last_battle_time = to_timestamp("+ to_string(it->second.last_battle_time) +") WHERE account_id  = " + to_string(it->first);
                    
                    update_q = PQexec(conn, sql.c_str());

                    // Pocitadlo updatov
                    update_counter ++;

                        if (PQresultStatus(update_q) != PGRES_COMMAND_OK)
                            {cout << "Chyba update players_info "  <<  PQresultErrorMessage(update_q) << endl;}
                    PQclear(update_q);
                }
            }

            if(riadkov == 0 ) {
                string ins_sql = "INSERT INTO players_info (account_id,global_rating,client_language,logout_at,last_battle_time,created_at) VALUES ";
                ins_sql += "("+to_string(it->first)+","+to_string(it->second.global_rating)+",'"+it->second.client_language+"',to_timestamp("+ to_string(it->second.logout_at) +"),to_timestamp("+ to_string(it->second.last_battle_time) +"),to_timestamp("+ to_string(it->second.created_at)+"))";
                
                insert_q = PQexec(conn, ins_sql.c_str());

                // Pocitadlo insertov
                insert_counter ++;

                        if (PQresultStatus(insert_q) != PGRES_COMMAND_OK)
                            {cout << "Chyba update players_info "  <<  PQresultErrorMessage(insert_q) << endl;}

                PQclear(insert_q);


                ins_sql.clear();
            }
            PQclear(prepared);
    }    
}


int main() {

        time_t start, stop;
    
        time(&start);
        cout << "Program zacal pracovat o: " << ctime(&start) << endl;

        PripojDatabase();
        NaplnStack();

        while (!account_ids.empty()) {

            GetPost();
            SendPost();
            ParseJson();
            CheckData();            

            json_data.clear();maps_data.clear();
            post.clear();            
        }
        
        VaccuumAnalyze();
        PQfinish(conn);

        cout << "Pocet spracovanych updatov :" << update_counter << endl;
        cout << "Pocet spracovanych insertov :" << insert_counter << endl;
        time(&stop);
        cout << endl << "Program skoncil o: " << ctime(&stop) << endl << endl;
    return 0;
}