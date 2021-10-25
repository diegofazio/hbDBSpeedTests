# hbDBSpeedTests
Harbour DB speed tests comparison - Registers Count: 821051

### MySql configuration( 1 or 2 ) 

1) /data/mysql/dbstru.zip  - Import structure of Database and then with /tests/mysql_wdo/importDbf.prg transfer all dbf data to mysql
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
- mysql  Ver 15.1 Distrib 10.4.11-MariaDB, for Win64 (AMD64) via https://github.com/carles9000/wdo and HDO( Manu Expósito )
- LetoDB x64 build. 2.17-b3 http://www.kresin.ru/en/letodb.html


## LOCALHOST TEST( in milliseconds )

### Connection time to db
| DBF    | LetoDB | Mysql(WDO)  | Mysql(HDO)  |
| ------ | ------ | ----------- | ----------- |
|    0   |    3   |   17        |  2          |

### Test1 - Simple record count with one search criteria - Result: 594060 regs
| DBF    | LetoDB | Mysql(WDO)  | Mysql(HDO)  |
| ------ | ------ | ----------- | ----------- |
|  219   |  467   |  152        |  172        |

### Test2 - Search and retrieve data(sorted by KAR_NUMERO) that match a criteria by date - Result: 39354 regs
| DBF    | LetoDB | Mysql(WDO)  | Mysql(HDO)  |
| ------ | ------ | ----------- | ----------- |
| 534    |  833   | 402         |  232        |

### Test3 - Search and retrieve a specific and unique data in the db - Result: 1reg
| DBF    | LetoDB | Mysql(WDO)  | Mysql(HDO)  |
| ------ | ------ | ----------- | ----------- |
|  301   |  705   |  153        |  154        |

### ImportDbf(DBF->mySql) - import DBF file with 821051 regs 

| Mysql(WDO)  | Mysql(HDO)  |
| ----------- | ----------- |
|  109146     |  82928      |


## LAN( gigabit ) TEST( in milliseconds )

### Connection time to db
| DBF    | LetoDB | Mysql(WDO)  | Mysql(HDO)  |
| ------ | ------ | ----------- | ----------- |
|    5   |    3   |   24        |             |

### Test1 - Simple record count with one search criteria - Result: 594060 regs
| DBF    | LetoDB | Mysql(WDO)  | Mysql(HDO)  |
| ------ | ------ | ----------- | ----------- |
|   181  |   527  |  213        |             |

### Test2 - Search and retrieve data(sorted by KAR_NUMERO) that match a criteria by date - Result: 39354 regs
| DBF    | LetoDB | Mysql(WDO)  | Mysql(HDO)  |
| ------ | ------ | ----------- | ----------- |
|   415  |  2464  |  476        |             |

### Test3 - Search and retrieve a specific and unique data in the db - Result: 1reg
| DBF    | LetoDB | Mysql(WDO)  | Mysql(HDO)  |
| ------ | ------ | ----------- | ----------- |
|   264  |   817  |  213        |             |

### ImportDbf(DBF->mySql) - import DBF file with 821051 regs 

| Mysql(WDO)  | Mysql(HDO)  |
| ----------- | ----------- |
|    3199859  |             |


