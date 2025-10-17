//import {duckdb} from 'suckdbuild/libqjs-duckdb.so';
import {ngx} from 'ngx';
    function sleep(ms) {
    const start = Date.now();
    while (Date.now() - start < ms) {
        // Пустой цикл, который занимает время
    }
    }

try{

    const db = new ngx;
    db.open('test.db');
    while(true)    
     {
        sleep(1000);
     }


console.log("Начало бесконечного цикла");

while (true) {
    console.log("Это сообщение будет выводиться каждые 2 секунды");
    sleep(2000); // Синхронная задержка на 2000 миллисекунд (2 секунды)
}
    
    
}
catch(e)
{
console.log('Ошибка ' + e.name + ":" + e.message + "\n" + e.stack); 
}
