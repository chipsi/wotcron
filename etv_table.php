<?php

class Database {

    const DSN = "pgsql:host=2a03:3b40:100::1:143;port=5432;dbname=clan;user=deamon;password=sedemtri";    
    protected $conn;
    protected $sthSelect;
    protected $sthInsert;
    protected $sthUpdate;

    function __construct () {
        $this->Connection();  // Vytvor spojenie do databazy
        $this->prepareSelect();
        $this->prepareInsert();
        $this->prepareUpdate();
    }

    function Connection () {
        try {
            $this->conn = new PDO(self::DSN);
        } catch (PDOException $e) {
            print "Chyba pripojenia k databaze: ". $e->getMessage(). "\n";
            die();
        }
    }

    private function prepareSelect() {
        $sql = 'SELECT count(*) FROM expected_tank_value WHERE tank_id = ?';
        $this->sthSelect = $this->conn->prepare($sql);
    }

    private function prepareInsert() {
        $sql = 'INSERT INTO expected_tank_value (tank_id,frag,dmg,spot,def,win) VALUES (:tank_id,:frag,:dmg,:spot,:def,:win)' ;
        try {
            $this->sthInsert = $this->conn->prepare($sql);
        }   catch (PDOException $e) {
            print "Chyba prepare Insert: ". $e->getMessage(). "\n";
            
        }
        
    }

    private function prepareUpdate() {
        $sql = 'UPDATE expected_tank_value SET frag=:frag,dmg=:dmg,spot=:spot,def=:def,win=:win WHERE tank_id = :tank_id';
        try{
            $this->sthUpdate = $this->conn->prepare($sql);
        } catch (PDOException $e) {
            print "Chyba prepare Update: ". $e->getMessage(). "\n";            
        }
        
    }

    public function execSelect (int $id ) : int {
        $this->sthSelect->execute(array($id));        
        $result = $this->sthSelect->fetch();
        $riadkov = $result['count'];

        return $riadkov;
    }

    public function execInsert( array $data ) {
        
        try {
            $this->sthInsert->execute($data);
        } catch (PDOException $e) {
            print "Chyba execute Insert: ". $e->getMessage(). "\n";            
        }
        
    }

    public function execUpdate( array $data ) {
        try{
            $this->sthUpdate->execute($data);    
        } catch (PDOException $e) {
            print "Chyba execute Update: ". $e->getMessage(). "\n";            
        }
        
    }

    function __destruct() {
        $this->conn = null;
        $this->sthSelect = null;
        $this->sthUpdate = null;
        $this->sthInsert = null;
    }
}

class Json {
    const URL = "https://static.modxvm.com/wn8-data-exp/json/wn8exp.json";

    function GetEtvTable () {        
        $json = file_get_contents(self::URL);
        $json = json_decode($json);
        return  $json;
    }
}

$database    = new Database;
$json       = new Json;

$json = $json->GetEtvTable();

$data = $json->data;
$ins = 0;
$upd = 0;

foreach ($data as $value ) {
    $stav = $database->execSelect($value->IDNum);    

    $array = array(':tank_id' => $value->IDNum, ':frag' => $value->expFrag, ':dmg'=>$value->expDamage, ':spot'=> $value->expSpot, ':def' => $value->expDef, ':win' => $value->expWinRate );
    
    if($stav === 0) {
        $database->execInsert($array);
        $ins ++ ;
    }
    if($stav === 1) {
        $database->execUpdate($array);
        $upd ++ ;

    }
    $stav = null;
    $array = null;

}
$trvanie = microtime(true) - $_SERVER["REQUEST_TIME_FLOAT"];
echo "Datum zaciatku scriptu: \t\t", date('Y-m-d H:i:s',$_SERVER["REQUEST_TIME_FLOAT"])."\n";
echo "Skript sa vykonaval:\t\t\t", $trvanie, "\n";
echo "Pocet insertov \t\t\t\t".$ins."\n";
echo "Pocet updatov \t\t\t\t".$upd."\n";    

    

