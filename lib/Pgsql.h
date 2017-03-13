/*
*   Trieda na pripojenie k databaze clan na server postgresql
*
*   author: Boris Fekar
*/

#ifndef Pgsql_H
#define Pgsql_H
#include <libpq-fe.h>

class Pgsql
{
    private:
<<<<<<< HEAD
            const char *conninfo = "host=37.205.11.183 port=5432 dbname=clan user=deamon password=sedemtri";
=======
            const char *conninfo = "dbname=clan user=deamon password=sedemtri";
>>>>>>> d0d07bebbcc18d6fa46370aa0cc1dddf716eab42
            PGconn *Connect();
    public:
            PGconn *pgsql;
            PGconn *Get();
            PGresult *Query(const char *sql);
            Pgsql();
            ~Pgsql();


};

<<<<<<< HEAD
#endif
=======
#endif
>>>>>>> d0d07bebbcc18d6fa46370aa0cc1dddf716eab42
