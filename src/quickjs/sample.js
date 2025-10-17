//import {duckdb} from 'suckdbuild/libqjs-duckdb.so';
import {duckdb} from 'duckdb';
try{

    const db = new duckdb;
    db.open('test.db');
    db.query('CREATE TABLE if not exists test (id INTEGER, name VARCHAR)');
    db.query("INSERT INTO test VALUES (1, 'Alice'), (2, 'Bob')");
    const result = db.query('SELECT * FROM test');
    console.log(JSON.stringify(result));
    db.close();
}
catch(e)
{
console.log('Ошибка ' + e.name + ":" + e.message + "\n" + e.stack); 
}
