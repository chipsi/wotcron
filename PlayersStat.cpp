#include <iostream>
#include <thread>
#include <libpq-fe.h>
#include <sys/time.h>
#include "./lib/SendCurl.h"
#include "./lib/Pgsql.h"

/* Meranie casu */
typedef unsigned long long timestamp_t;

static timestamp_t get_timestamp ()
{
      struct timeval now;
      gettimeofday (&now, NULL);
      return  now.tv_usec + (timestamp_t)now.tv_sec * 1000000;
}

using namespace std;

void Players2(int *p_Array, int *p_riadkov)
{
    PGconn *conn; PGresult *result;
    Pgsql trieda;
    conn = trieda.pgsql;
    int i; string a_id;

    string sql = "SELECT account_id FROM players_all";
    result = PQexec(conn, sql.c_str());

    for(i = 0; i < *p_riadkov; i++)
    {
        a_id  = PQgetvalue(result, i, 0);
        *(p_Array+i) = stoi(a_id); 
    }
    PQclear(result);


}
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

        // Ziskavanie account_id po 1000ks 
        string GetPlayers(int offset)
        {
            PGresult *result;
            string query    = "SELECT account_id FROM players_all OFFSET " + to_string(offset) + " LIMIT 180";
            result          = PQexec(this->conn, query.c_str());
            
            int riadkov     = PQntuples(result);
            
            int i;string account_ids, a_id;
            
            for(i = 0; i < riadkov; i++)
            {
                a_id       = PQgetvalue(result,i,0);
                account_ids += a_id + ",";
            }
            PQclear(result);
            return account_ids;
        }

};

void ExtractFromString(string account_ids, int stovka, string *p_a_id) // Vyber stovku ID z 1000 ID
{
    int i ; int z = 0; int p = 0; int  k = 0; int posledny_znak;

    posledny_znak = account_ids.size(); 
    cout << "posledny znak "<<posledny_znak<< endl;
    for(i=0;i < stovka; i++)
    {
        p = account_ids.find(',',p+1);
        if(i == (stovka - 100)){ z = p-9;break;}
    }
    
    p = z;
    cout << p << endl;
    for(i = 0; i < 100; i++)
    {
        p = account_ids.find(',',p+1); 
        k = p; if(k == posledny_znak){k = k - 1;break;}
    }
     cout << "kcko" << k << endl;
    *p_a_id = account_ids.substr(z,k-z);    
}










int main()
{
    timestamp_t t0 = get_timestamp();
    /* Informacia o case zacatia programu */
    time_t start, stop;
    time(&start);
    cout << endl << "Program zacal pracovat: " << ctime(&start) << endl;
    
    Players *c_players = new Players; // Ziskam pointer na triedu pre pracu s hracmi
    int riadkov = c_players->GetPocet(); // Zistim celkovy pocet hracov na kontrolu

    //string ExtractFromString(string account_ids, int stovka, string *p_a_id);
    string a_id1,a_id2;
    string *p_a_id1, *p_a_id2;
    p_a_id1 = &a_id1; p_a_id2 = &a_id2; 


    int i;string account_ids;
    for(i = 0; i < riadkov; i = i + 1000)
    {
        
        account_ids = c_players->GetPlayers(i); // Ziskam 1000 hracov
       
        thread T1(ExtractFromString,account_ids,100,p_a_id1);
        thread T2(ExtractFromString,account_ids,200,p_a_id2);
        
        T1.join();
        T2.join();

        cout <<  "prvy : "  << endl << a_id1 << endl;
        cout <<  "druhy : " << endl << a_id2 << endl;


        break;
        
        
        
        account_ids.clear();
    }

    











    timestamp_t t1 = get_timestamp();
    double secs = (t1 - t0) / 1000000.0L;
    cout << "Cas:\t\t" << secs << endl;
    cout << "Program skoncil pracovat: " << ctime(&stop) << endl;
    return 0;
}