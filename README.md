# hbDBSpeedTests
Harbour DB speed tests comparison - Registers Count: 821051

### MySql configuration( 1 or 2 ) 

1) /data/mysql/dbstru.zip  - Import structure of Database and then with importDbf.prg transfer all dbf data to mysql
2) /data/mysql/db.zip - Import all database file to mysql

### Dbf configuration

1) unpack /data/dbf/db.zip to any folder. Do not forget to set that folder in .prg tests!!

### LetoDB configuration

1) unpack /data/dbf/db.zip to any folder(Set Datapath - See step 2). 
2) /data/letodb.ini <- Set the "Datapath" with your full path to dbf folder. FIRST OF ALL!!!!
3) /data/letodb.exe install <- Install letodb Service
4) /data/letodb.exe start <- Start letodb Service(Check Windows Services if started)

/data/letodb.exe stop <- Stop letodb Service
/data/letodb.exe uninstall <- Uninstall letodb Service

### Server specifications 

- Windows 10 x64 - i7-7400 8gb Ram - SSD 480gb
- mysql  Ver 15.1 Distrib 10.4.11-MariaDB, for Win64 (AMD64)
- LetoDB x64 build. 2.17-b3 http://www.kresin.ru/en/letodb.html


## LOCALHOST TEST( in milliseconds )

### Connection time to db
| DBF    | LetoDB | Mysql  |
| ------ | ------ | ------ |
|    0   |    3   |   17   |

### Test1 - Simple record count with one search criteria - Result: 594060 regs
| DBF    | LetoDB | Mysql  |
| ------ | ------ | ------ |
|  246   |  467   |  152   |





