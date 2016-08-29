#include <iostream>
#include <thread>
#include <libpq-fe.h>
#include <sys/time.h>
#include "./lib/SendCurl.h"
#include "./lib/Pgsql.h"
#include <unordered_map>

/* Meranie casu */
typedef unsigned long long timestamp_t;
typedef unordered_map<int,int> container;


static timestamp_t get_timestamp ()
{
      struct timeval now;
      gettimeofday (&now, NULL);
      return  now.tv_usec + (timestamp_t)now.tv_sec * 2000000;
}

using namespace std;

class Players 
{
    public:
        PGconn *conn;
        Players()
        {
            Pgsql *pgsql    = new Pgsql;
            this->conn      = pgsql->Get();
            delete pgsql;
        }

        // Ziskam pocet hracov v databaze
        int GetPocet()
        {
            PGresult *result;
            int i; string res;
           
            string query = "SELECT count(*) FROM players_all";
            result = PQexec(this->conn, query.c_str());
            res = PQgetvalue(result,0,0); 
            PQclear(result);
            i   = stoi(res); 
            return i;
        }

        // Ziskavanie account_id po 600ks 
        void GetPlayers(int offset, container *account_ids)
        {
            PGresult *result;
            string query    = "SELECT account_id FROM players_all OFFSET " + to_string(offset) + " LIMIT 600";
            result          = PQexec(this->conn, query.c_str());
            
            int riadkov     = PQntuples(result);
            
            int i;
            
            for(i = 0; i < riadkov; i++)
            {
                (*account_ids)[i]  = stoi (PQgetvalue(result,i,0));
                
            }
            PQclear(result);
        }

        
        

};

// Rozsekam int account_id na jeden string so 200 account_id oddeleny ciarkami pre posielanie dotazov
void GetAccountId(int stovka, container *account_ids, string *ids)
{
    
    for(int i = stovka-100; i < stovka; i++)
    {
        *ids += to_string((*account_ids)[i]) + ",";
    }

    ids->erase(ids->end()-1); // Vymaze poslednu ciarku
}


int main()
{
    timestamp_t t0 = get_timestamp();
    /* Informacia o case zacatia programu */
    time_t start, stop;
    time(&start);
    cout << endl << "Program zacal pracovat: " << ctime(&start) << endl;
   
    Players players;
    // Najprv zistim kolko hracov budem spracovavat
    int riadkov = players.GetPocet();

    /*  Sem si deklarujem potrebne premenne aby sa v cykle nehromazdili */
    container a_ids; container *p_a_ids; p_a_ids = &a_ids; // unordered_map na account_id
    
    // string so 100 account_id
    string aids1;string *p_aids1; p_aids1 = &aids1;
    string aids2;string *p_aids2; p_aids2 = &aids2;
    string aids3;string *p_aids3; p_aids3 = &aids3;
    string aids4;string *p_aids4; p_aids4 = &aids4;
    string aids5;string *p_aids5; p_aids5 = &aids5;
    string aids6;string *p_aids6; p_aids6 = &aids6;

    for(int i = 0; i < riadkov; i += 600) // Tu si nastavim po kolko hracov naraz budem spracovavat
    {
        players.GetPlayers(i, p_a_ids); // ziskam do unordered_map int account_id
        
        // premenim na to na string so 100 account_id oddeleny ciarkami
        thread string1(GetAccountId,100,p_a_ids,p_aids1); 
        thread string2(GetAccountId,200,p_a_ids,p_aids2);
        thread string3(GetAccountId,300,p_a_ids,p_aids3);
        thread string4(GetAccountId,400,p_a_ids,p_aids4);
        thread string5(GetAccountId,500,p_a_ids,p_aids5);
        thread string6(GetAccountId,600,p_a_ids,p_aids6);

        string1.join();string2.join();string3.join();string4.join();string5.join();string6.join();

        cout << aids6 << endl;

        break;
    }

   

    











    timestamp_t t2 = get_timestamp();
    double secs = (t2 - t0) / 2000000.0L;
    time(&stop);
    cout << "Cas:\t\t" << secs << endl;
    cout << "Program skoncil pracovat: " << ctime(&stop) << endl;
    return 0;
}