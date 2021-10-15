/*	---------------------------------------------------------
	File.......: rdbms_mysql.prg
	Description: Conexi�n a Bases de Datos MySql 
	Author.....: Carles Aubia Floresvi
	Date:......: 26/07/2019
	--------------------------------------------------------- */
#include "hbdyn.ch"

   #xcommand TRY  => BEGIN SEQUENCE WITH {| oErr | Break( oErr ) }
   #xcommand CATCH [<!oErr!>] => RECOVER [USING <oErr>] <-oErr->
   #xcommand FINALLY => ALWAYS
   #xtranslate Throw( <oErr> ) => ( Eval( ErrorBlock(), <oErr> ), Break( <oErr> )

#define VERSION_RDBMS_MYSQL				'0.1c'
#define HB_VERSION_BITWIDTH  				17
#define NULL  								0  

	
CLASS RDBMS_MySql FROM RDBMS

	DATA pLib
	DATA hMySql
	DATA hConnection
	
	DATA lConnect								INIT .F.
	DATA nAffected_Rows						INIT 0
	DATA cError 								INIT ''
	DATA aFields 								INIT {}
	DATA aLog 									INIT {}
	DATA bError									INIT {|cError| qOut ( cError ) }
	
	METHOD New() 								CONSTRUCTOR
		
	METHOD Query( cSql )	
	METHOD Row_Count()	
	METHOD Last_Insert_Id()	
	
	METHOD Count( hRes )						INLINE ::mysql_num_rows( hRes )
	METHOD FCount( hRes )						INLINE ::mysql_num_fields( hRes )
	METHOD LoadStruct()					
	METHOD DbStruct()							INLINE ::aFields
	METHOD Fetch( hRes )	
	METHOD Fetch_Assoc( hRes )		
	METHOD FetchAll( hRes, lAssociative )
	METHOD Free_Result( hRes )				INLINE ::mysql_free_result( hRes )	
	
	
	
	//	Wrappers (Antonio Linares)
	
	METHOD mysql_init()
	METHOD mysql_Close()
	METHOD mysql_get_server_info()
	METHOD mysql_real_connect( cServer, cUserName, cPassword, cDataBaseName, nPort )
	METHOD mysql_error()
	METHOD mysql_query( cQuery )
	METHOD mysql_store_result()
	METHOD mysql_num_rows( hRes )
	METHOD mysql_num_fields( hRes ) 
	METHOD mysql_fetch_field( hRes )
	METHOD mysql_fetch_row( hRes )
	METHOD mysql_free_result( hRes )
	METHOD mysql_real_escape_string_quote( cQuery )	

	
	METHOD Version()							INLINE VERSION_RDBMS_MYSQL

		
	
	DESTRUCTOR  Exit()
					

ENDCLASS

METHOD New( cServer, cUsername, cPassword, cDatabase, nPort, cType ) CLASS RDBMS_MySql

	hb_default( @cServer, '' )
	hb_default( @cUserName, '' )
	hb_default( @cPassword, '' )
	hb_default( @cDatabase, '' )
	hb_default( @nPort, 3306 )
	hb_default( @cType, 'MYSQL' )
	
	::cServer		:= cServer
	::cUserName	:= cUserName
	::cPassword 	:= cPassword
	::cDatabase 	:= cDatabase
	::nPort 		:= nPort	
	
	//	Cargamos lib mysql
	
		IF cType == 'MYSQL'
			::pLib 	:= hb_LibLoad( hb_SysMySQL() )	
		ELSE
			::pLib 	:= hb_LibLoad( hb_SysMariaDb() )	
		ENDIF

		If ValType( ::pLib ) <> "P" 
			::cError := "Error (MySQL library not found)" 
			
			IF Valtype( ::bError ) == 'B'
				Eval( ::bError, ::cError )
			ENDIF
			
			RETU Self
		ENDIF
	
	//	Inicializamos mysql

		::hMySQL = ::mysql_init()
	
		IF ::hMySQL = 0 
			::cError := "hMySQL = " + Str( ::hMySQL ) + " (MySQL library failed to initialize)"
			
			IF Valtype( ::bError ) == 'B'
				Eval( ::bError, ::cError )
			ENDIF			
			
			RETU Self
		ENDIF
		
	//	Server Info
	
		// "MySQL version: " + ::mysql_get_server_info()  	

		
	//	Conexion a Base de datos	
		
		::hConnection := ::mysql_real_connect( ::cServer, ::cUserName, ::cPassword, ::cDatabase, ::nPort )
		
		IF  ::hConnection != ::hMySQL
			::cError := "Connection = (Failed connection) " + ::mysql_error()
			
			IF Valtype( ::bError ) == 'B'
				Eval( ::bError, ::cError )
			ENDIF			
			
			RETU Self
		ENDIF
		
		::lConnect := .T.
		
RETU SELF

METHOD Query( cQuery ) CLASS RDBMS_MySql

	LOCAL nRetVal
	LOCAL hRes			:= 0
	local cSql 		:= ''
	

    IF ::hConnection == 0
		RETU NIL
	ENDIF	

	cSql := StrTran( cQuery, "'", "\'" )  


    nRetVal := ::mysql_query( cQuery )
	

	IF nRetVal == 0 

		
		hRes = ::mysql_store_result()


		IF hRes != 0					//	Si Update/Delete hRes == 0

			::LoadStruct( hRes )
		ENDIF

		//::Row_Count()			

	ELSE

		::cError := 'Error: ' + ::mysql_error()

		IF Valtype( ::bError ) == 'B'
			Eval( ::bError, ::cError )
		ENDIF
		
	ENDIF
   
RETU hRes


METHOD Last_Insert_Id() CLASS RDBMS_MySql 

	local nId := -1
	local hRes, hRs 
	
	if !empty( hRes := ::Query( "SELECT LAST_INSERT_ID() as last_id"  ) )
	
		hRs := ::Fetch_Assoc( hRes )
	
		nId := Val( hRs['last_id'] )
	
	endif 

RETU nId 

//	Return affected rows. Executed after query

METHOD Row_Count() CLASS RDBMS_MySql 

	local hRes, hRs 
	
	::nAffected_Rows := 0
	
	if !empty( hRes := ::Query( "SELECT ROW_COUNT() as total"  ) )	
	
		hRs 	:= ::Fetch_Assoc( hRes )
	
		::nAffected_Rows	:= Val( hRs['total'] )
	
	endif 

RETU ::nAffected_Rows

METHOD LoadStruct( hRes ) CLASS RDBMS_MySql

	LOCAL n, hField	
     
    ::aFields = Array( ::FCount( hRes ) )
	
    FOR n = 1 to Len( ::aFields )
	
        hField := ::mysql_fetch_field( hRes )
		
        if hField != 0
		
			::aFields[ n ] = Array( 4 )
            ::aFields[ n ][ 1 ] = PtrToStr( hField, 0 )
			
            do case
               case AScan( { 253, 254, 12 }, PtrToUI( hField, hb_SysMyTypePos() ) ) != 0
                    ::aFields[ n ][ 2 ] = "C"

               case AScan( { 1, 3, 4, 5, 8, 9, 246 }, PtrToUI( hField, hb_SysMyTypePos() ) ) != 0
                    ::aFields[ n ][ 2 ] = "N"

               case AScan( { 10 }, PtrToUI( hField, hb_SysMyTypePos() ) ) != 0
                    ::aFields[ n ][ 2 ] = "D"

               case AScan( { 250, 252 }, PtrToUI( hField, hb_SysMyTypePos() ) ) != 0
                    ::aFields[ n ][ 2 ] = "M"
            endcase 
			
        endif   
		 
	NEXT 

	  
RETU NIL

//original
/*METHOD Fetch( hRes ) CLASS RDBMS_MySql

	LOCAL hRow
	LOCAL aReg
	LOCAL m

	if ( hRow := ::mysql_fetch_row( hRes ) ) != 0
	
		aReg := array( ::FCount( hRes ) )
	
		for m = 1 to ::FCount( hRes )
		   aReg[ m ] := PtrToStr( hRow, m - 1 )
		next
		
	endif
	
	//::mysql_free_result( hRes )

RETU aReg*/

//modified
METHOD Fetch( hRes ) CLASS RDBMS_MySql

   LOCAL hRow
   LOCAL aReg
   LOCAL m
   LOCAL f

   IF ( hRow := ::mysql_fetch_row( hRes ) ) != 0
      f := ::FCount( hRes )
      aReg := Array( f )
      FOR m = 1 TO f
         aReg[ m ] := PtrToStr( hRow, m - 1 )
      NEXT
   ENDIF
   // ::mysql_free_result( hRes )

RETURN aReg



METHOD Fetch_Assoc( hRes ) CLASS RDBMS_MySql

	LOCAL hRow
	LOCAL hReg		:= {=>}
	LOCAL m

	if ( hRow := ::mysql_fetch_row( hRes ) ) != 0
	
		for m = 1 to ::FCount( hRes )
		   hReg[ ::aFields[m][1] ] := PtrToStr( hRow, m - 1 )
		next
		
	endif
	
	//::mysql_free_result( hRes )

RETU hReg

METHOD FetchAll( hRes, lAssociative ) CLASS RDBMS_MySql

	LOCAL oRs
	LOCAL aData := {}
	
	__defaultNIL( @lAssociative, .f. )
	
	IF lAssociative 
	
		WHILE ( !empty( oRs := ::Fetch_Assoc( hRes ) ) )
		
			Aadd( aData, oRs )
		
		END
		
	ELSE
	
		WHILE ( !empty( oRs := ::Fetch( hRes ) ) )
		
			Aadd( aData, oRs )
			
		END
		
	ENDIF
	
	
RETU aData


//	Wrappers...

METHOD mysql_num_rows( hRes ) CLASS RDBMS_MySql	

return hb_DynCall( { "mysql_num_rows", ::pLib, hb_bitOr( hb_SysLong(),;
                  hb_SysCallConv() ), hb_SysLong() }, hRes )




METHOD mysql_Init() CLASS RDBMS_MySql

RETU hb_DynCall( { "mysql_init", ::pLib, hb_bitOr( hb_SysLong(), hb_SysCallConv() ) }, NULL )



METHOD mysql_Close() CLASS RDBMS_MySql

RETU hb_DynCall( { "mysql_close", ::pLib, hb_SysCallConv(), hb_SysLong() }, ::hMySQL )

				   
METHOD mysql_get_server_info() CLASS RDBMS_MySql	

RETU hb_DynCall( { "mysql_get_server_info", ::pLib, hb_bitOr( HB_DYN_CTYPE_CHAR_PTR,;
                   hb_SysCallConv() ), hb_SysLong() }, ::hMySql )			   


				   
METHOD mysql_real_connect( cServer, cUserName, cPassword, cDataBaseName, nPort ) CLASS RDBMS_MySql	

    if nPort == nil
       nPort = 3306
    endif   

RETU hb_DynCall( { "mysql_real_connect", ::pLib, hb_bitOr( hb_SysLong(),;
                     hb_SysCallConv() ), hb_SysLong(),;
                     HB_DYN_CTYPE_CHAR_PTR, HB_DYN_CTYPE_CHAR_PTR, HB_DYN_CTYPE_CHAR_PTR, HB_DYN_CTYPE_CHAR_PTR,;
                     HB_DYN_CTYPE_LONG, HB_DYN_CTYPE_LONG, HB_DYN_CTYPE_LONG },;
                     ::hMySQL, cServer, cUserName, cPassword, cDataBaseName, nPort, 0, 0 )
                     				   
				   

METHOD mysql_error() CLASS RDBMS_MySql	

RETU hb_DynCall( { "mysql_error", ::pLib, hb_bitOr( HB_DYN_CTYPE_CHAR_PTR,;
                   hb_SysCallConv() ), hb_SysLong() }, ::hMySql )

				   

METHOD mysql_query( cQuery ) CLASS RDBMS_MySql	

	local u
	
	local bNewError := {|oError| ErrorHandler(oError,.T.) }
    local bOldError := Errorblock(bNewError)

	BEGIN SEQUENCE
		u := hb_DynCall( { "mysql_query", ::pLib, hb_bitOr( HB_DYN_CTYPE_INT,;
						   hb_SysCallConv() ), hb_SysLong(), HB_DYN_CTYPE_CHAR_PTR },;
						   ::hConnection, cQuery )
   
    RECOVER
      ? "Error" 
      return nil
    END SEQUENCE
	
	Errorblock(bOldError)
				   
RETU u 

METHOD mysql_real_escape_string_quote( cQuery ) CLASS RDBMS_MySql	

	cQuery := StrTran( cQuery, "'", "\'" )

retu cQuery

/*
RETU hb_DynCall( { "mysql_real_escape_string", hb_bitOr( HB_DYN_CTYPE_INT,;
                   hb_SysCallConv() ), hb_SysLong(), HB_DYN_CTYPE_CHAR_PTR, HB_DYN_CTYPE_CHAR_PTR, HB_DYN_CTYPE_LONG, HB_DYN_CTYPE_CHAR_PTR },;
				   ::hConnection, @cQuery, cQuery, Len(cQuery), "\'")	
*/

/*
RETU hb_DynCall( { "mysql_real_escape_string", ::pLib, hb_bitOr( hb_SysLong(),;
hb_SysCallConv() ), hb_SysLong(), HB_DYN_CTYPE_CHAR_PTR, HB_DYN_CTYPE_CHAR_PTR, HB_DYN_CTYPE_LONG, HB_DYN_CTYPE_CHAR_PTR },;
::hConnection, @cQuery, cQuery,  Len(cQuery), "\'")				   
*/				   


METHOD mysql_store_result() CLASS RDBMS_MySql	

RETU hb_DynCall( { "mysql_store_result", ::pLib, hb_bitOr( hb_SysLong(),;
                   hb_SysCallConv() ), hb_SysLong() }, ::hMySQL )



METHOD mysql_num_fields( hRes ) CLASS RDBMS_MySql	

RETU hb_DynCall( { "mysql_num_fields", ::pLib, hb_bitOr( HB_DYN_CTYPE_LONG_UNSIGNED,;
                   hb_SysCallConv() ), hb_SysLong() }, hRes )				   
				   
				   
METHOD mysql_fetch_field( hRes ) CLASS RDBMS_MySql	

RETU hb_DynCall( { "mysql_fetch_field", ::pLib, hb_bitOr( hb_SysLong(),;
                   hb_SysCallConv() ), hb_SysLong() }, hRes )	
				   

	   
METHOD mysql_fetch_row( hRes ) CLASS RDBMS_MySql	

RETU hb_DynCall( { "mysql_fetch_row", ::pLib, hb_bitOr( hb_SysLong(),;
                   hb_SysCallConv() ), hb_SysLong() }, hRes )	  



METHOD mysql_free_result( hRes ) CLASS RDBMS_MySql	

RETU hb_DynCall( { "mysql_free_result", ::pLib,;
                   hb_SysCallConv(), hb_SysLong() }, hRes )
				   
				   



				   
				   
METHOD Exit() CLASS RDBMS_MySql

    IF ValType( ::pLib ) == "P"
	
		//? "MySQL library properly freed: ", HB_LibFree( ::pLib )
		
		? 'Exit MEthod ', ::pLib, ::hMySql
		::MySql_Close()
		
		HB_LibFree( ::pLib )
		
		? 'Exit MEthod 2', ::pLib, ::hMySql
    ENDIF 
	
RETU NIL


//	------------------------------------------------------------
function hb_SysLong()

return If( hb_OSIS64BIT(), HB_DYN_CTYPE_LLONG_UNSIGNED, HB_DYN_CTYPE_LONG_UNSIGNED )

//----------------------------------------------------------------//

function hb_SysCallConv()

return If( ! "Windows" $ OS(), HB_DYN_CALLCONV_CDECL, HB_DYN_CALLCONV_STDCALL )

//----------------------------------------------------------------//


function hb_SysMyTypePos()

return If( hb_version( HB_VERSION_BITWIDTH ) == 64,;
       If( "Windows" $ OS(), 26, 28 ), 19 )   

//----------------------------------------------------------------//

function hb_SysMySQL()

   local cLibName

   
   if ! "Windows" $ OS()
      if "Darwin" $ OS()
         cLibName = "/usr/local/Cellar/mysql/8.0.16/lib/libmysqlclient.dylib"
      else   
         cLibName = If( hb_version( HB_VERSION_BITWIDTH ) == 64,;
                        "/usr/lib/x86_64-linux-gnu/libmysqlclient.so",; // libmysqlclient.so.20 for mariaDB
                        "/usr/lib/x86-linux-gnu/libmysqlclient.so" )
      endif                  
   else

		IF hb_version( HB_VERSION_BITWIDTH ) == 64
		
			IF !Empty( HB_GetEnv( 'WDO_PATH_MYSQL' ) )
				cLibName = HB_GetEnv( 'WDO_PATH_MYSQL' ) + 'libmysql64.dll'
			ELSE
				cLibName = "c:/Apache24/htdocs/libmysql64.dll"
			ENDIF
		
		ELSE
		
			IF !Empty( HB_GetEnv( 'WDO_PATH_MYSQL' ) )
				cLibName = HB_GetEnv( 'WDO_PATH_MYSQL' ) + 'libmysql.dll'
			ELSE
				cLibName = "c:/Apache24/htdocs/libmysql.dll"
			ENDIF		
		
		ENDIF

   endif

return cLibName 

//----------------------------------------------------------------//

function hb_SysMariaDb()

   local cLibName

   
   if ! "Windows" $ OS()
      if "Darwin" $ OS()
         cLibName = "/usr/local/Cellar/mysql/8.0.16/lib/libmysqlclient.dylib"
      else   
         cLibName = If( hb_version( HB_VERSION_BITWIDTH ) == 64,;
                        "/usr/lib/x86_64-linux-gnu/libmariadbclient.so",; // libmysqlclient.so.20 for mariaDB
                        "/usr/lib/x86-linux-gnu/libmariadbclient.so" )
      endif                  
   else

		IF hb_version( HB_VERSION_BITWIDTH ) == 64
		
			IF !Empty( HB_GetEnv( 'WDO_PATH_MYSQL' ) )
				cLibName = HB_GetEnv( 'WDO_PATH_MYSQL' ) + 'libmysql64.dll'
			ELSE
				cLibName = "c:/Apache24/htdocs/libmysql64.dll"
			ENDIF
		
		ELSE
		
			IF !Empty( HB_GetEnv( 'WDO_PATH_MYSQL' ) )
				cLibName = HB_GetEnv( 'WDO_PATH_MYSQL' ) + 'libmysql.dll'
			ELSE
				cLibName = "c:/Apache24/htdocs/libmysql.dll"
			ENDIF		
		
		ENDIF

   endif

return cLibName 
