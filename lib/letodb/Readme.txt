      /* $Id$ */

      Leto DB Server is a multiplatform database server or a database 
 management system, chiefly intended for client programs, written on Harbour,
 be able to work with dbf/cdx files, located on a remote server.

Contents
--------

1. Directory structure
2. Building binaries
   2.1 Borland Win32 C compiler
   2.2 Linux GNU C compiler
   2.3 xHarbour Builder
   2.4 Mingw C compiler
   2.5 Building letodb with hbmk
   2.6 Addition functions support
3. Running and stopping server
4. Server configuration
   4.1 letodb.ini
   4.2 Authentication
5. Features of work with the letodb server
   5.1 Connecting to the server from client programs
   5.2 Filters
6. Variables management
7. Functions list
   7.1 Connection management functions
   7.2 Transaction functions
   7.3 Additional functions for current workarea
   7.4 Additional rdd functions
   7.5 Setting client paramenter
   7.6 File functions
   7.7 Management functions
   7.8 User account management functions
   7.9 Server variable management functions
   7.10 Calling udf-functions on the server
   7.11 Functions for bitmap filters
8. Management utility
9. Server-side functions


      1. Directory structure

      bin/          -    server executable file
      doc/          -    documentation
      include/      -    source header files
      lib/          -    rdd library
      source/
          client/   -    rdd sources
          common/   -    some common source files
          client/   -    server sources
      tests/        -    test programs, samples
      utils/
          manage/   -    server management utilities


      2. Building binaries

      The letodb server can be compiled only by the Harbour compiler, and client
 library - both Harbour, and xHarbour. For OS Windows the letodb server can be
 compiled as Windows service (the macro by __WIN_SERVICE__ should be initialized),
 or as the daemon (process) for what it's necessary to set a macro __WIN_DAEMON__.
 For Linux it is necessary to set a macro __LINUX_DAEMON__.
      The server and the client library can be built with the support of the driver
 BMDBFCDX/BMDBFNTX.  In this case, the basic rdd letodb server will be used instead
 DBFCDX/DBFNTX driver BMDBFCDX/BMDBFNTX, and supported the same functionality that
 BMDBF*. To build on this mode, in build scripts (letodb.hbp, rddleto.hbp for hbmk2
 and makefile.* for other compilers) it's need to set a macro __BM.
 You can pass option -env for hbmk2 utility:
 hbmk2 -env:__BM=yes letodb.hbp
 hbmk2 -env:__BM=yes rddleto.hbp
      The server can be built with the support of functions leto_Zip and leto_UnZip.
 To build on this mode, in build script (letodb.hbp) it's need to set a macro __ZIP:
 hbmk2 -env:__ZIP=yes letodb.hbp

      2.1 Borland Win32 C compiler

      An environment variable HB_PATH, which defines a path to the Harbour
 directory, must be set before compiling. This can be done, for example, 
 by adding a line in a make_b32.bat:

          SET HB_PATH=c:\harbour

 If you use xHarbour, uncomment a line 'XHARBOUR = yes' in makefile.bc.
 Then run the make_b32.bat and you will get server executable file letodb.exe in a bin/
 directory and rdd library rddleto.lib in a lib/ directory.

      2.2 Linux GNU C compiler

      An environment variable HB_ROOT, which defines a path to the Harbour
 directory, must be set before compiling. Or just change the value of HRB_DIR
 in the Makefile.linux.

 Then run the make_linux.sh and you will get server executable file letodb in a bin/
 directory and rdd library librddleto.a in a lib/ directory.

      2.3 xHarbour Builder

      Run the make_xhb.bat to build binaries with this compiler. Probably,
  you will need to change the path to your xHarbour Builder copy in the
  make_xhb.bat. Default value is:

          set XHB_PATH=c:\xhb

      2.4 Mingw C compiler

      An environment variable HB_PATH, which defines a path to the Harbour
 directory, must be set before compiling. This can be done, for example, 
 by adding a line in a make_mingw.bat:

          SET HB_PATH=c:\harbour

 If you use xHarbour, uncomment a line 'XHARBOUR = yes' in makefile.gcc.
 Then run the make_mingw.bat and you will get server executable file letodb.exe in a bin/
 directory and rdd library librddleto.a in a lib/ directory.

      2.5 Building letodb with hbmk

      Now there is a possibility to build letodb with a Harbour make utility, provided by
 Viktor Szakats. The command line syntax is:

      hbmk2 [-hb10|-xhb] rddleto.hbp letodb.hbp [-target=tests/test_ta.prg]

 Optional parameters: -hb10 - Harbour version is 1.0 or 1.0.1,
                      -xhb - xHarbour,
                      -target="blank"\test_ta.prg - build the test, too.

      2.6 Addition functions support

      LetoDB use Harbour expression engine for evaluating index expressions and filters,
 and allows a call of the following functions:

      ABS, ALLTRIM, AT, CHR, CTOD, DATE, DAY, DELETED, DESCEND, DTOC, DTOS,
      EMPTY, I2BIN, L2BIN, LEFT, LEN, LOWER, LTRIM, MAX, MIN, MONTH, PAD, PADC,
      PADL, PADR, RAT, RECNO, RIGHT, ROUND, RTRIM, SPACE, STOD, STR, STRZERO,
      SUBSTR, REPLICATE, TIME, TRANSFORM, TRIM, UPPER, VAL, YEAR,
      hb_ATokens, hb_WildMatch

      If it's necessary to add this set of functions, in the source/server.prg module
it's necessary to add a line:

      REQUEST <cFName1>[, ...]

      And then rebuild server.

      3. Running and stopping server

      Just run it:
      
      letodb.exe                    ( under Windows )
      ./letodb                      ( under Linux )

      To shutdown the server, run the same executable with a 'stop' parameter:

      letodb.exe stop               ( under Windows )
      ./letodb stop                 ( under Linux )

      To reload letoudf.hrb module, run the same executable with a 'reload' parameter:

      letodb.exe reload             ( under Windows )
      ./letodb reload               ( under Linux )

      For windows service (server should be compiled with __WIN_SERVICE__ flag):

      To install and uninstall service, you must have administrative privileges.
      To install service, run letodb with 'install' parameter:

      letodb.exe install

      To uninstall service, run letodb with 'uninstall' parameter:

      letodb.exe uninstall

      To start and stop server, run service manager.

      4. Server configuration

      4.1 letodb.ini

      You may provide configuration file letodb.ini if you isn't satisfied with
 default parameters values. Currently following parameters exists ( default
 values are designated ), they should be written in section [MAIN]:

      [MAIN]
      Port = 2812              -    server port number;
      Ip =                     -    ip address, where Letodb listens for connections;
                                    If it doesn't set, all net interfaces ( all ip addresses ),
                                    available on the computer, are used.
      TimeOut = -1             -    connection timeout;
      DataPath =               -    path to a data directory on a server;
      LogPath = letodb.log     -    path and name of a log file;
      Default_Driver = CDX     -    default RDD to open files on server ( CDX/NTX );
      Memo_Type =              -    memo type ( FPT/DBT ). Default: FPT for DBFCDX, DBT for DBFNTX;
      Lower_Path = 0           -    if 1, convert all paths to lower case;
      EnableFileFunc = 0       -    if 1, using of file functions ( leto_file(),
                                    leto_ferase(), leto_frename() is enabled;
      EnableAnyExt = 0         -    if 1, creating of data tables and indexes with
                                    any extention, other than standard ( dbf,cdx,ntx )
                                    is enabled; 
      EnableUDF = 1            -    if 1 (by default), using of udf functions with prefix "UDF_*" is enabled,
                                    if 2, using of udf functions with any names is enabled,
                                    if 0, udf calls is disabled;
      Pass_for_Login = 0       -    if 1, user authentication is necessary to
                                    login to the server;
      Pass_for_Manage = 0      -    if 1, user authentication is necessary to
                                    use management functions ( Leto_mggetinfo(), etc. );
      Pass_for_Data = 0        -    if 1, user authentication is necessary to
                                    have write access to the data;
      Pass_File = "leto_users" -    the path and name of users info file;
      Crypt_Traffic = 0        -    if 1, the data passes to the network encrypted;
      Share_Tables = 0         -    if 0 (default, this mode server was the only from the
                                    start of a letodb project), the letodb opens all
                                    tables in an exclusive mode, what allows to increase
                                    the speed. If 1 (new mode, added since June 11, 2009),
                                    tables are opened in the same mode as client
                                    applications opens them, exclusive or shared, what
                                    allows the letodb to work in coexistence with other
                                    types of applications.
      No_Save_WA = 0           -    When this mode is set, each dbUseArea() will cause
                                    a real file open operation and creating a new 
                                    workarea on the server ( in default mode each file
                                    is opened only one time and have only one real
                                    workarea for all users ).
                                    Theoretically this new mode may increase the speed
                                    in case of a big number of active users. It isn't
                                    properly tested yet, so for production use default
                                    mode is recommended.
      Cache_Records            -    The number of records to read into the cache
      Max_Vars_Number = 10000  -    Maximum number of shared variables
      Max_Var_Size = 10000     -    Maximim size of a text variable
      Trigger = <cFuncName>    -    Global function letodb RDDI_TRIGGER
      PendingTrigger = <cFuncName>- Global function letodb RDDI_PENDINGTRIGGER
      EnableSetTrigger = 0     -    if 1, allows to change trigger settings,
                                    using dbInfo( DBI_TRIGGER, ... )
      Tables_Max  = 5000       -    Number of tables
      Users_Max = 500          -    Number of users
      Debug = 0                -    Debug level
      Optimize = 0             -    if 1, SET HARDCOMMIT OFF
      AutOrder = 0             -    SET AUTORDER setting
      ForceOpt = 0             -    _SET_FORCEOPT setting

      It is possible to define [DATABASE] structure if you need to have a
 directory, where files are opened via other RDD:

      [DATABASE]
      DataPath =               -    (mandatory option)
      Driver = CDX             -    ( CDX/NTX )

      You can define as many [DATABASE] sections, as needed.

      In Windows environment the letodb.ini must be placed in a directory, from
 where server is started.
      In Linux the program looks for it in the directory from where the server
 is started and, if unsuccessfully, in the /etc directory.

      4.2 Authentication

      To turn authentication subsystem on you need to set one of the following 
 letodb.ini parameters to 1: Pass_for_Login, Pass_for_Manage, Pass_for_Data. But before
 you need to create, at least, one user with admin rights, because when authentication
 subsystem works, only authenticated users with admin rights are able to add/change users
 and passwords.
      To add a user, you need to include a call of LETO_USERADD() in your client side
 program, for example:

      LETO_USERADD( "admin", "secret:)", "YYY" )

 where "YYY" is a string, which gives rights to admin, manage and write access. You may
 also use the utils/manager/console.prg program to set or change authentication data.

 To connect to a server with an authentication data ( username and password ) you need to
 use LETO_CONNECT() function.

      5. Features of work with the letodb server

      5.1 Connecting to the server from client programs

      To be able to connect to the server you need to link the rddleto library
 to your aplication and add at start of a main source file two lines:

      REQUEST LETO
      RDDSETDEFAULT( "LETO" )

      To open a dbf file on a server, you need to write in a SET PATH TO
 statement or in the USE command directly a path to the server in a standard
 form //ip_address:port/data_path/file_name.

      If a 'DataPath' parameter of a server configuration file is set to a non
 empty value, you need to designate not the full path to a file on the server,
 but only a relative ( relatively the 'DataPath' ).
      For example, if the you need to open a file test.dbf, which is located on
 a server 192.168.5.22 in a directory /data/mydir and the 'DataPath' parameter
 value ( of a letodb.ini server configuration file ) is '/data', the syntax
 will be the following:

      USE "//192.168.5.22:2812/mydir/test"

      If the server doesn't run or you write a wrong path, you'll get open error.
 It is possible to check accessibility of a server before opening files by using
 the leto_Connect( cAddress ) function, which returns -1 in case of unsuccessful
 attempt:

      IF leto_Connect( "//192.168.5.22:2812/mydir/" ) == -1
         Alert( "Can't connect to server ..." )
      ENDIF

     At connection with the server the client send to it information about codepage,
which the server then uses for operations of indexation, a filtration and for some
other operations. The client codepage should be established before connection with
the letodb server.

      5.2 Filters

      The filter is established usually: by the SET FILTER TO command or by a call
dbSetFilter() function. The filter which can be executed on the server, is called optimized.
If the filter can't be executed on the server, it is not optimized. Such filter
is slow as from the server all records, which are all the same requested then
are filtered on the client. To set the optimized filter, it is necessary, that in logical
expression for the filter there were no variables or the functions defined on the client.
To check, whether the filter is optimized, it is necessary to caall the LETO_ISFLTOPTIM()
function.

      6. Variables management

      Letodb allows to manage variables, which are shared between applications, connected
 to the server. Variables are separated into groups and may be of logical, integer or 
 character type. You can set a variable, get it, delete or increment/decrement
 ( numerical, of course ). All operations on variables are performed consecutively by
 one thread, so the variables may work as semaphores. See the functions list for a syntax
 of appropriate functions and tests/test_var.prg for a sample.
      Letodb.ini may contain lines to determine the maximum number of variables and a
 maximum size of a text variable.

      7. Functions list

      7.1 Connection management functions

      Below is a full ( at least, for the moment I write it ) list of functions,
 available for using in client applications with RDD LETO linked.

      LETO_CONNECT( cAddress, [ cUserName ], [ cPassword ], [ nTimeOut ], [ nBufRefreshTime ] )
                                                           --> nConnection, -1 if failed
 nBufRefreshTime defines the time interval in 0.01 sec. After this 
 time is up, the records buffer must be refreshed, 100 by default (1 sec)

      LETO_CONNECT_ERR()                                   --> nError
      LETO_DISCONNECT( [ cConnString | nConnection ] )     --> nil
      LETO_SETCURRENTCONNECTION( [ cConnString | nConnection ] ) --> nConnection
      LETO_GETCURRENTCONNECTION()                          --> nConnection
      LETO_GETSERVERVERSION()                              --> cVersion
      LETO_GETLOCALIP()                                    --> IP address of client station
      LETO_ADDCDPTRANSLATE(cClientCdp, cServerCdp )        --> nil
      LETO_PATH( [<cPath>], [cConnString | nConnection] )  --> cOldPath
      LETO_PING( [ cConnString | nConnection ] )           --> lResult

      7.2 Transaction functions

      LETO_BEGINTRANSACTION( [ nBlockLen ], [ lParseTrans ] )
 Parameter <nBlockLen> can be used for set memory allocation block size.
 For big transaction it can improve transaction speed at the client side.
 if parameter <lParseTransRec> is .F., transaction buffer isn't
 processing during record parsing from server.

      LETO_ROLLBACK()

      LETO_COMMITTRANSACTION( [ lUnlockAll ] )             --> lSuccess

      LETO_INTRANSACTION()                                 --> lTransactionActive

      7.3 Additional functions for current workarea

      LETO_COMMIT()
 This function can be used for current workarea instead of calls:

 dbCommit()
 dbUnlock()

 The client sends to the server 3 packages: for record updating
 on the server, for commit record and area unlock. Leto_Commit sends
 only one package for all these operations

      LETO_SUM( <cFieldNames>|<cExpr>, [ cFilter ], [xScope], [xScopeBottom] )
                                                           --> nSumma if one field or expression passed, or
                                                               {nSumma1, nSumma2, ...} for several fields
 The first parameter of leto_sum if comma separated fields or expressions:
 leto_sum("Sum1,Sum2", cFilter, cScopeTop, cScopeBottom) return
 an array with values of sum fields Sum1 and Sum2.

 If "#" symbol passed as field name, leto_sum returns a count of
 evaluated records, f.e:
 leto_sum("Sum1,Sum2,Sum1+Sum2,#", cFilter, cScopeTop, cScopeBottom) -->
 {nSum1, nSum2, nSum3, nCount}

  If only one field name or expression is passed, leto_sum() returns a numeric value

      LETO_GROUPBY(cGroup, <cFields>|<cExpr>, [cFilter], [xScopeTop], [xScopeBottom]) --> aValues
                                                               {{xGroup1, nSumma1, nSumma2, ...}, ...}

 This function return two-dimensional array. The first element of each row is
 a value of <cGroup> field, elements from 2 - sum of comma separated fields or
 expressions, represented in <cFields>. If "#" symbol passed as field name in cFields,
 leto_groupby return a count of evaluated records in each group

      LETO_ISFLTOPTIM()                                    --> lFilterOptimized

      dbInfo( DBI_BUFREFRESHTIME[, nNewVal])               --> nOldVal
  Setting new value for area skip and seek buffers refresh time in 0.01 sec.
  If -1: using connection setting. If 0 - buffers are used anyway.

      dbInfo( DBI_CLEARBUFFER )
  This command cleared skip buffer.

      7.4 Additional rdd functions

      leto_CloseAll( [ cConnString | nConnection ] )       --> nil
 Close all workareas for specified or default connection

      7.5 Setting client paramenter

      LETO_SETSKIPBUFFER( nSkip )                          --> nCount (buffer statistic using)
 This buffer is intended for optimization of multiple calls of skip.
 This function set size in records of skip buffer for current workarea.
 By default, the size of skip buffer is 10 records. Skip buffer is bidirectional.
 Skip buffer is refreshed after BUFF_REFRESH_TIME (1 sec)
 If parameter <nSkip> is absent, function returns bufferr statictic (number of buffer shooting)

      LETO_SETSEEKBUFFER( nRecsInBuf )                     --> nCount (buffer statistic using)
 This buffer is intended for optimization of multiple calls of seek.
 This function set size in records of seek buffer for current index order.
 If record is found in seek buffer on dbSeek(), it's not loaded from server.
 Like skip buffer, seek buffer is refreshed after BUFF_REFRESH_TIME (1 sec)
 If parameter <nRecsInBuf> is absent, function returns bufferr statictic
 (number of buffer shooting)
 By default, seek buffer is turned off.

      LETO_SETFASTAPPEND( lFastAppend )                    --> lFastAppend (previous value)
 If the fast append mode isn't set, record is added on the server at once
 after a dbAppend() function call. If this mode is set, record is added
 by dbCommit() call, or after a call of functions of navigation. By default
 a mode fast append isn't set. The switch working on all workareas.

      RddInfo( RDDI_REFRESHCOUNT, <lSet>,, [nConnection] )
 By default, the RDDI_REFRESHCOUNT flag is set to true.  If this flag is set,
 RecCount() function retrieve records count from server, if doesn't set -
 use last value from server.  If other applications are appending records
 to the table, new value of records count won't be immediately received.
 If RDDI_REFRESHCOUNT flag is cleared, dbGoto(0) cleared record buffer and
 set eof and other flags instead of server request.

      RddInfo( RDDI_BUFKEYCOUNT, <lSet>,, [nConnection] )
 By default, the RDDI_BUFKEYCOUNT flag is set to false.  If this flag isn't set,
 ordKeyCount() function retrieve key count from server, if set -
 use last value from server.

      RddInfo( RDDI_BUFKEYNO, <lSet>,, [nConnection] )
 By default, the RDDI_BUFKEYNO flag is set to false.  If this flag isn't set,
 ordKeyNo() function retrieve value from server, if set - use last value from server.

      RddInfo( RDDI_CLIENTID,,, [nConnection] )
 Returns self client ID (nUserStru) number.

      7.6 File functions

 The <cFileName> parameter of all file functions can contain a connection string
 to the letodb server in a format:
 //ip_address:port/data_path/file_name.
 If the connection string is omitted, the current active connection is used:
 /data_path/file_name.
 All files is searching at the DataPath catalog on the server. The ".."
 symbols are disabled.

      LETO_FILE( cFileName )                               --> lFileExists
 Determine if file exist at the server, analog of File() function

      LETO_FERASE( cFileName )                             --> -1 if failed
 Delete a file at the server.

      LETO_FRENAME( cFileName, cFileNewName )              --> -1 if failed
 Rename a file: <cFileName> --> <cFileNewName>. <cFileNewName> should be without
 connection string.

      LETO_MEMOREAD( cFileName )                           --> cStr
 Returns the contents of file at the server as character string, analog of
 MemoRead() function.

      Leto_MemoWrite( cFileName, cBuf )                    --> lSuccess
 Writes a character string into a file at the server, analog of
 MemoWrit() function.

      LETO_MAKEDIR( cDirName )                             --> -1 if failed
 Creates a directory at the server

      LETO_DIREXIST( cDirName )                            --> lDirExists
 Determine if directory exist at the server

      LETO_DIRREMOVE( cDirName )                           --> -1 if failed
 Delete a directory at the server.

      LETO_FERROR()                                        --> nError
 Returns an error code of last file function.

      Leto_FileRead( cFileName, nStart, nLen, @cBuf )      --> -1 if failed
 Read a content of file at the server from <nStart> offset and <nLen> length

      Leto_FileWrite( cFileName, nStart, cBuf )            --> lSuccess
 Write <cBuf> character string to a file at the server from <nStart> offset

      Leto_FileSize( cFileName )                           --> -1 if failed
 Returns a length of a file at the server

      Leto_FileAttr( cFileName [, cNewAttr] )              --> cAttr
 Get or set file attributes

      Leto_Directory( cDir[, cAttr] )                      --> aDirectory
 Returns a content of directory at the server in the same format as Directory() function

      7.7 Management functions

      LETO_MGGETINFO()                                     --> aInfo
 This function returns parameters of current connection as 17-element array
 of char type values:
 1 - count of active users
 2 - max count of users
 3 - opened tables
 4 - max opened tables
 5 - time elapsed
 6 - count of operations
 7 - bytes sent
 8 - bytes read
 9 - opened indexes
 10 - max opened indexes
 11 - data path
 12 - max day wait
 13 - ulWait
 14 - count of transactions
 15 - count successfully of transactions
 16 - current memory used
 17 - max memory used

      LETO_MGGETUSERS( [<cTableId>|<nTable>] )                        --> aInfo
 Function returns two-dimensional array, each row is info about user:
 aInfo[i,1] - user number
 aInfo[i,2] - ip address
 aInfo[i,3] - net name of client
 aInfo[i,4] - program name
 aInfo[i,5] - timeout

      LETO_MGGETTABLES( [<cUserId>|<nUser>] )                         --> aInfo
 Function returns two-dimensional array, each row is info about opened tables:
 aInfo[i,1] - table number
 aInfo[i,2] - table name

      LETO_MGGETLOCKS( [<cUserId>|<nUser>] )                          --> aInfo
 Function returns two-dimensional array, each row is info about locks:
 aInfo[i,1] - table name
 aInfo[i,2] - character string with list of locked records

      LETO_MGGETTIME()                                     --> aDateTime
 Function returns array {<dDate>, <nSeconds>}:
 dDate - server date;
 nSeconds - seconds after midnight.
 Convert this values to datetime variable (Harbour):
 hb_DTOT( aDateTime[1], aDateTime[2] )

      LETO_MGKILL( <nUser>|<cUserId> )
 Kill user number <nUser>

      LETO_LOCKCONN( lOnOff )                              --> lSuccess
 After leto_lockconn( .t. ) request new connections are blocked by server, until
 leto_lockconn( .f. ) called

      LETO_LOCKLOCK( lOnOff, nSecs )                       --> lSuccess
 This function wait until any updates are closed, commit all changes and
 trying to lock server from any updates from clients.
 It returns True, if lock is succesfull.

      7.8 User account management functions

      LETO_USERADD( cUserName, cPass [, cRights ] )        --> lSuccess
      LETO_USERPASSWD( cUserName, cPass )                  --> lSuccess
      LETO_USERRIGHTS( cUserName, cRights )                --> lSuccess
      LETO_USERFLUSH()                                     --> lSuccess
      LETO_USERGETRIGHTS()                                 --> cRights

      7.9 Server variable management functions

      LETO_VARSET( cGroupName, cVarName, xValue[, nFlags[, @xRetValue]] ) --> lSuccess

 This function assign value <xValue> to variable <cVarName> from group <cGroupName>
 Optional parameter <nFlags> defines function mode:
 LETO_VCREAT    - if variable doesn't exist, it's created;
 LETO_VOWN      - own user variable (free after user disconnect);
 LETO_VDENYWR   - write deny for other users;
 LETO_VDENYRD   - read deny for other users;
 LETO_VPREVIOUS - retusn previos value to <xRetValue> parameter.

      LETO_VARGET( cGroupName, cVarName )                  --> xValue

 Function return value of variable <cVarName> from group <cGroupName>

      LETO_VARINCR( cGroupName, cVarName )                 --> nValue

 Function increment value of variable <cVarName> from group <cGroupName>

      LETO_VARDECR( cGroupName, cVarName )                 --> nValue

 Function decrement value of variable <cVarName> from group <cGroupName>

      LETO_VARDEL( cGroupName, cVarName )                  --> lSuccess

 Function delete variable <cVarName> from group <cGroupName>

      LETO_VARGETLIST( [cGroupName [, nMaxLen]] )          --> aList

 Function return two-dimensional array with variables: { {<cVarName>, <value>}, ...}

      7.10 Calling udf-functions on the server

      LETO_UDF( cSeverFunc, xParam1, ... )                 --> xResult
 This function is called from client application. A string
 <cServerFunc> contains a server connection string and name
 of udf function:
 //ip_address:port/funcname
 A <funcname> function should be defined on the letodb server.
 The first parameter of udf function is nUserStru.
 Udf function can return result (any type) to client.
 Examples of udf-functions are in the tests/letoudf.prg

      LETO_UDFEXIST( cSeverFunc )                          --> lExist
 leto_udfExist check the existance of udf-function at the letodb server.
 <cSeverFunc> parameter is the same as for leto_udf().

      LETO_PARSEREC( cRecBuf )                             --> nil
 This function is necessary for calling, if udf-function returns as
 result the buffer of record which should be current after of it
 works.

      LETO_PARSERECODRS( cRecBuf )                         --> nil
 This function is necessary for calling to fill the skip-buffer as a result
 of udf-function UDF_dbEval. After calling this function, the data from the
 skip-buffer can be received by dbSkip() calls. See sample in comments for
 UDF_dbEval() in tests/letoudf.prg

      7.11 Functions for bitmap filters

 If letodb compiled with rdd BMDBFCDX/BMDBFNTX, then there is support
 the following functions:

      LBM_DbGetFilterArray()                               --> aFilterRec
      LBM_DbSetFilterArray( aFilterRec )                   --> nil
      LBM_DbSetFilterArrayAdd( aFilterRec )                --> nil
      LBM_DbSetFilterArrayDel( aFilterRec )                --> nil

 Purpose and the parameters of these functions is the same as for the
 corresponding functions BM_*.

      LBM_DbSetFilter( [<xScope>], [<xScopeBottom>], [<cFilter>] ) -> nil
 This function set bitmap filter by current index order and for condition,
 defined in <xScope>, <xScopeBottom>, <cFilter> parameters.
 The current record after LBM_DbSetFilter() is the first record satisfying
 filter condition.

      8. Management utility

      There are two management utilities, GUI and console, the sources are in
 utils/manage directory.

      GUI utility, manage.prg, is made with the HwGUI library. If you have HwGUI,
 just write in the line 'set HWGUI_INSTALL=' in utils/manage/bld.bat a path
 to your HwGUI directory and run the bld.bat, it will build manage.exe for you.
   
      For those, who doesn't use HwGUI, there is a console mode utility,
 console.prg. Build a console.exe with a make/bat files, which you use to build
 Harbour single-file programs, you just need to add rddleto.lib to the libraries
 list. Run the console.exe with a server name or ip and port as a parameter:

      console.exe server_name:nPort
      console.exe ip_address:nPort

 The server_name and ip_address in a command line must be without leading
 slashes ( '//' ), because Clipper/Harbour interprets them in a special way.

      9. Server-side functions

      These functions can be run from the client by function leto_udf,
 and also from the functions defined in a file letoudf.hrb.
 The first parameter such function always nUserStru

      leto_Alias( nUserStru, cClientAlias )                --> cRealAlias
 This function return server alias for client alias <cClientAlias>.
 The server alias then can be user in usial rdd-operations

      leto_RecLock( nUserStru [, nRecord] )                --> lSuccess
  leto_Reclock function locked record with <nRecord> number, or a current record.

      leto_RecUnLock( nUserStru [, nRecord] )              --> Nil
  leto_RecUnlock function unlocked record with <nRecord> number, or a current
  record.

      leto_RecLockList( nUserStru, aRecNo )                --> lSuccess
  leto_ReclockList function locked records with number in the <aRecNo> array.
  If any record isn't locked, all records are unlocked, and function returns
  .F. result.
  This function can be used at the server from letoodf.prg module, or from
  client by a call leto_UDF( "leto_RecLockList", aRecNo ).

      leto_TableLock( <nUserStru>, [<nFlag>], [<Secs>])    --> lSuccess
      leto_TableUnLock( <nUserStru>, [<nFlag>])

      nFlag - parameter from 1 to 32, nSecs - seconds for wait locking (1 by default)
      These functions are intended for concurrent access to the table for
      actions except updating of the data.

      leto_SelectArea( nUserStru, nAreaId )                --> lSuccess

      leto_areaID( nUserStru )                             --> nAreaId
 Function return internal id of current workarea

      leto_ClientID()                                      --> nUserStru
 Function return client ID (nUserStru), -1 if failed

      letoUseArea( nUserStru, cFileName, cAlias, lShared, lReadOnly, cdp ) --> nAreaId
      letoOrdListAdd( nUserStru, cBagName )                --> Nil
      letoOrdCreate( nUserStru, cBagName, cKey, cTagName,
                     lUnique, cFor, cWhile, lAll, nRecNo,
                     nNext, lRest, lDesc, lCustom,
                     lAdditive )                           --> Nil
      letoCloseArea( nUserStru )                           --> Nil

 The functions letoUseArea, letoOrdListAdd, letoOrdCreate, letoCloseArea,
 leto_RecLock, leto_RecLock is intended for using in udf-functions instead
 of rdd functions: dbUseArea, OrdListAdd, OrdCreate, dbCloseArea, RLock, dbUnlock

 Server variable management functions:

      LETO_VARSET( nUserStru, cGroupName, cVarName, xValue[, nFlags )
                                                           --> lSuccess
      LETO_VARGET( nUserStru, cGroupName, cVarName )       --> xValue
      LETO_VARINCR( nUserStru, cGroupName, cVarName )      --> nValue
      LETO_VARDECR( nUserStru, cGroupName, cVarName )      --> nValue
      LETO_VARDEL( nUserStru, cGroupName, cVarName )       --> lSuccess
      LETO_VARGETLIST( nUserStru, [cGroupName, [lValue]] ) --> aList

 Zip/Unzip functions:

      leto_Zip( nUserStru, [cDirName], [acFiles], [nLevel], [lOverwrite], [cPassword], [acExclude], [lWithPath] )
                                                           --> cZip
      leto_UnZip( nUserStru, [cDirName], cZip, [cPassword], [lWithPath] )   --> lSuccess
 parameters:
 cDirName - directory relatively DataPath;
 cZip - archive body to unzip;
 other parameters is the same as parameters of hb_ZipFile/hb_UnZipFile functions.
 Calling functions from a client:
 leto_UDF("leto_Zip", [cDirName], ...)
 leto_UDF("leto_UnZip", [cDirName], cZip, ...)
