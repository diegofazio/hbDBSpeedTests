/*	---------------------------------------------------------
	File.......: rdbms_mysql.prg
	Description: Conexión a Bases de Datos MySql 
	Author.....: Carles Aubia Floresvi
	Date:......: 26/07/2019
	--------------------------------------------------------- */
#include "hbdyn.ch"

   #xcommand TRY  => BEGIN SEQUENCE WITH {| oErr | Break( oErr ) }
   #xcommand CATCH [<!oErr!>] => RECOVER [USING <oErr>] <-oErr->
   #xcommand FINALLY => ALWAYS
   #xtranslate Throw( <oErr> ) => ( Eval( ErrorBlock(), <oErr> ), Break( <oErr> )

#define VERSION_RDBMS_MYSQL				'WDO 1.0g'
#define HB_VERSION_BITWIDTH  				17
#define NULL  								0  

	
CLASS RDBMS_MySql FROM RDBMS

	DATA pLib
	DATA hMySql
	DATA hConnection
	
	DATA lConnect								INIT .F.
	DATA lLog									INIT .F.
	DATA lWeb									INIT .T.
	DATA nAffected_Rows						INIT 0
	DATA cError 								INIT ''
	DATA aFields 								INIT {}
	DATA aLog 									INIT {}
	DATA bError									INIT {|cError| AP_RPuts( '<br>' + cError ), .t. }
	DATA nFields								INIT 0
	DATA nSysCallConv
	DATA nSysLong
	DATA nTypePos
	
	METHOD New() 								CONSTRUCTOR
		
	METHOD Query( cSql )	
	
	METHOD Prepare( hRow )
	METHOD Escape( hRow )
	
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
	
	METHOD d()
	
	DESTRUCTOR  Exit()
					

ENDCLASS

METHOD New( cServer, cUsername, cPassword, cDatabase, nPort, cType, lLog, bError ) CLASS RDBMS_MySql

	local cDll ,lAjax
	local nTry := 0

	hb_default( @cServer, '' )
	hb_default( @cUserName, '' )
	hb_default( @cPassword, '' )
	hb_default( @cDatabase, '' )
	hb_default( @nPort, 3306 )
	hb_default( @cType, 'MYSQL' )
	hb_default( @lLog, .F. )
	

	if valtype( bError ) == 'B'	
		::bError := bError
	else
		::bError := {|cError| AP_RPuts( '<br>' + cError ), .t. }
	endif
		
	::cServer		:= cServer
	::cUserName	:= cUserName
	::cPassword 	:= cPassword
	::cDatabase 	:= cDatabase
	::nPort 		:= nPort	
	::lLog 			:= lLog 

	
	if( ::lLog, ::d( 'WDO log activated' ), nil )	
	
	lAjax := lower( AP_GetEnv('HTTP_X_REQUESTED_WITH') ) == 'xmlhttprequest'
	
	if( ::lLog, ::d( if(lAjax, 'AJAX Yes', 'AJAX No' ) ), nil )		
	
	//	Cargamos lib mysql
	
		IF cType == 'MYSQL'
			cDll 	:= hb_SysMySQL() 
		ELSE
			cDll 	:= hb_SysMariaDb() 
		ENDIF	
	
		::pLib 	:= hb_LibLoad( cDll )	

		If ValType( ::pLib ) <> "P" 
			::cError := "Error (MySQL library not found)" 
			
			if( ::lLog, ::d( ::cError + ' ' + cDll ), nil )
			
			IF Valtype( ::bError ) == 'B'
				if Eval( ::bError, ::cError )
					quit
				endif
			ENDIF
			
			RETU Self
		ENDIF
		
	//	Inicializamos Variables 
	
		::nSysCallConv 	:= hb_SysCallConv()
		::nSysLong 		:= hb_SysLong()
		::nTypePos 		:= hb_SysMyTypePos()

	
	//	Inicializamos mysql
	
		
		::hMySQL = ::mysql_init()

		while ::hMySql == 0 .and. nTry < 3
		
			nTry++
			if( ::lLog, ::d( 'MySQL library failed to initialize. Try: ' + str(nTry)), nil )
			inkey(0.1)	
			::hMySQL = ::mysql_init()			
			
		end 
		
	
		IF ::hMySQL == 0 
			::cError := "hMySQL = 0 (MySQL library failed to initialize)"

			IF Valtype( ::bError ) == 'B'
			
				if( ::lLog, ::d( ::cError ), nil )
				
				if Eval( ::bError, ::cError )
					quit
				endif 
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
			
				if( ::lLog, ::d( ::cError ), nil )
				
				if Eval( ::bError, ::cError )					
					quit
				endif 
			ENDIF			
			
			RETU Self
		ENDIF
		
		::lConnect := .T.
		
RETU SELF

/*	Ejecutamos un hb_addslashes sobre las variables a salvar: 
	l'aliga -> l\'aliga
	Hello "World" -> Hello \"Wold\"	
*/
METHOD Prepare( hRow ) CLASS RDBMS_MySql
	
	local n, aPair, nLen 
	local cType := valtype( hRow )
	
	do case 
		case cType == 'H'
		
			nLen := len( hRow )
	
			for n := 1 to nLen 
			
				aPair := HB_HPairAt( hRow, n )
				
				if valtype( aPair[2] ) == 'C' .or. valtype( aPair[2] ) == 'M' 								
					hRow[ aPair[1] ] := hb_addslashes( aPair[2] )
				endif

			next 
			
		case cType == 'A'
	
			nLen := len( hRow )
			
			for n := 1 to nLen 							
				
				if valtype(  hRow[ n ] ) == 'C' .or. valtype(  hRow[ n ] ) == 'M' 								
					hRow[ n ] := hb_addslashes( hRow[ n ] )
				endif

			next 	

		case cType == 'C'								
			
 
			if valtype(  hRow ) == 'C' .or. valtype( hRow ) == 'M' 								
				hRow := hb_addslashes( hRow )					
			endif
			
	endcase 

retu hRow 


METHOD Escape( hRow ) CLASS RDBMS_MySql
/*	
	local n, aPair, nLen := len( hRow )
	
	for n := 1 to nLen 
	
		aPair := HB_HPairAt( hRow, n )
		
		hRow[ aPair[1] ] := //hb_addslashes( aPair[2] )

	next 
*/	

retu hRow 




METHOD Query( cQuery ) CLASS RDBMS_MySql

	LOCAL nRetVal
	LOCAL hRes			:= 0
	local cSql 		:= ''	

    IF ::hConnection == 0
		RETU NIL
	ENDIF	

	//cSql := StrTran( cQuery, "'", "\'" )  

	if ( ::lLog	, ::d( cQuery ), nil )	
	
	::nFields 	:= 0		
	
    nRetVal 	:= ::mysql_query( cQuery )	

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
     
    ::nFields := ::FCount( hRes ) 
    ::aFields := Array( ::nFields )
	
	
    FOR n = 1 to ::nFields
	
        hField := ::mysql_fetch_field( hRes )
		
        if hField != 0
		
			::aFields[ n ] = Array( 4 )
            ::aFields[ n ][ 1 ] = PtrToStr( hField, 0 )
			
            do case              
               case AScan( { 253, 254, 12 }, PtrToUI( hField, ::nTypePos ) ) != 0
                    ::aFields[ n ][ 2 ] = "C"

               case AScan( { 1, 3, 4, 5, 8, 9, 246 }, PtrToUI( hField, ::nTypePos ) ) != 0
                    ::aFields[ n ][ 2 ] = "N"

               case AScan( { 10 }, PtrToUI( hField, ::nTypePos ) ) != 0
                    ::aFields[ n ][ 2 ] = "D"

               case AScan( { 250, 252 }, PtrToUI( hField, ::nTypePos ) ) != 0
                    ::aFields[ n ][ 2 ] = "M"
            endcase 
			
        endif   
		 
	NEXT 

	  
RETU NIL

METHOD Fetch( hRes, aNoEscape ) CLASS RDBMS_MySql

	LOCAL hRow
	LOCAL aReg
	LOCAL m, f
	
	hb_default( @aNoEscape, {} )

	if ( hRow := ::mysql_fetch_row( hRes ) ) != 0	
	
		aReg 	:= array( ::nFields )		
	
		if len( aNoEscape ) == 0 
		
			if ::lWeb
			
				for m = 1 to ::nFields	
					aReg[ m ] := hb_HtmlEncode( PtrToStr( hRow, m - 1 ) )														
				next
				
			else 
			
				for m = 1 to ::nFields					
					aReg[ m ] := PtrToStr( hRow, m - 1 ) 
				next				
			
			endif
		
		else 
		
			if ::lWeb
		
				for m = 1 to ::nFields
				
					if Ascan( aNoEscape, ::aFields[m][1] ) > 0
						aReg[ m ] := PtrToStr( hRow, m - 1 ) 
					else
						aReg[ m ] := hb_HtmlEncode( PtrToStr( hRow, m - 1 ) )
					endif
				next
			
			else 
			
				for m = 1 to ::nFields
				
					if Ascan( aNoEscape, ::aFields[m][1] ) > 0
						aReg[ m ] := PtrToStr( hRow, m - 1 ) 
					else
						aReg[ m ] := PtrToStr( hRow, m - 1 )
					endif
				next						
			
			endif 
		
		endif
		
	endif

	//if hRes > 0			//	Casca al liberar la memoria !!!!!  IMPORTANT
	//	::mysql_free_result( hRes )		
	//endif

RETU aReg


METHOD Fetch_Assoc( hRes, aNoEscape ) CLASS RDBMS_MySql

	LOCAL hRow
	LOCAL hReg		:= {=>}
	LOCAL m, f 
	
	hb_default( @aNoEscape, {} )
	
	if ( hRow := ::mysql_fetch_row( hRes ) ) != 0
		
		if len( aNoEscape ) == 0 

			if ::lWeb		

				for m = 1 to ::nFields					
					hReg[ ::aFields[m][1] ] :=  hb_HtmlEncode( PtrToStr( hRow, m - 1 ) )			
				next	

			else
			
				for m = 1 to ::nFields					
					hReg[ ::aFields[m][1] ] :=  PtrToStr( hRow, m - 1 ) 
				next				
			
			endif

		else 
			
			for m = 1 to ::nFields
			
				if Ascan( aNoEscape, ::aFields[m][1]  ) > 0		
					hReg[ ::aFields[m][1] ] :=  PtrToStr( hRow, m - 1 ) 
				else
					hReg[ ::aFields[m][1] ] :=  hb_HtmlEncode( PtrToStr( hRow, m - 1 ) )
				endif
			
			next
			
		endif
			
		
	endif
	
	//if hRes > 0	//	Casca al liberar la memoria !!!!!  IMPORTANT
	//	::mysql_free_result( hRes )		
	//endif

RETU hReg

METHOD FetchAll( hRes, lAssociative, aNoEscape ) CLASS RDBMS_MySql

	LOCAL oRs
	LOCAL aData := {}
	
	__defaultNIL( @lAssociative, .f. )
	__defaultNIL( @aNoEscape, {} )


	
	IF lAssociative 
	
		WHILE ( !empty( oRs := ::Fetch_Assoc( hRes, aNoEscape ) ) )
	
			Aadd( aData, oRs )
		
		END
	
				
	ELSE
	
		WHILE ( !empty( oRs := ::Fetch( hRes, aNoEscape  ) ) )
		
			Aadd( aData, oRs )
			
		END
		
	ENDIF

	
	
RETU aData


//	Wrappers...

METHOD mysql_num_rows( hRes ) CLASS RDBMS_MySql	

return hb_DynCall( { "mysql_num_rows", ::pLib, hb_bitOr( ::nSysLong,;
                  ::nSysCallConv ), ::nSysLong }, hRes )




METHOD mysql_Init() CLASS RDBMS_MySql

RETU hb_DynCall( { "mysql_init", ::pLib, hb_bitOr( ::nSysLong, ::nSysCallConv ) }, NULL )



METHOD mysql_Close() CLASS RDBMS_MySql

RETU hb_DynCall( { "mysql_close", ::pLib, ::nSysCallConv, ::nSysLong }, ::hMySQL )

				   
METHOD mysql_get_server_info() CLASS RDBMS_MySql	

RETU hb_DynCall( { "mysql_get_server_info", ::pLib, hb_bitOr( HB_DYN_CTYPE_CHAR_PTR,;
                   ::nSysCallConv ), ::nSysLong }, ::hMySql )			   


				   
METHOD mysql_real_connect( cServer, cUserName, cPassword, cDataBaseName, nPort ) CLASS RDBMS_MySql	

    if nPort == nil
       nPort = 3306
    endif   

RETU hb_DynCall( { "mysql_real_connect", ::pLib, hb_bitOr( ::nSysLong,;
                     ::nSysCallConv ), ::nSysLong,;
                     HB_DYN_CTYPE_CHAR_PTR, HB_DYN_CTYPE_CHAR_PTR, HB_DYN_CTYPE_CHAR_PTR, HB_DYN_CTYPE_CHAR_PTR,;
                     HB_DYN_CTYPE_LONG, HB_DYN_CTYPE_LONG, HB_DYN_CTYPE_LONG },;
                     ::hMySQL, cServer, cUserName, cPassword, cDataBaseName, nPort, 0, 0 )
                     				   
				   

METHOD mysql_error() CLASS RDBMS_MySql	

RETU hb_DynCall( { "mysql_error", ::pLib, hb_bitOr( HB_DYN_CTYPE_CHAR_PTR,;
                   ::nSysCallConv ), ::nSysLong }, ::hMySql )

				   

METHOD mysql_query( cQuery ) CLASS RDBMS_MySql	

	local u
	
	local bNewError := {|oError| ErrorHandler(oError,.T.) }
    local bOldError := Errorblock(bNewError)

	BEGIN SEQUENCE
		u := hb_DynCall( { "mysql_query", ::pLib, hb_bitOr( HB_DYN_CTYPE_INT,;
						   ::nSysCallConv ), ::nSysLong, HB_DYN_CTYPE_CHAR_PTR },;
						   ::hConnection, cQuery )
   
    RECOVER
      ap_rputs( ::mysql_error() )
      return nil
    END SEQUENCE
	
	Errorblock(bOldError)
				   
RETU u 

METHOD mysql_real_escape_string_quote( cQuery ) CLASS RDBMS_MySql	

	cQuery := StrTran( cQuery, "'", "\'" )

retu cQuery

/*
RETU hb_DynCall( { "mysql_real_escape_string", hb_bitOr( HB_DYN_CTYPE_INT,;
                   ::nSysCallConv ), ::nSysLong, HB_DYN_CTYPE_CHAR_PTR, HB_DYN_CTYPE_CHAR_PTR, HB_DYN_CTYPE_LONG, HB_DYN_CTYPE_CHAR_PTR },;
				   ::hConnection, @cQuery, cQuery, Len(cQuery), "\'")	
*/

/*
RETU hb_DynCall( { "mysql_real_escape_string", ::pLib, hb_bitOr( ::nSysLong,;
::nSysCallConv ), ::nSysLong, HB_DYN_CTYPE_CHAR_PTR, HB_DYN_CTYPE_CHAR_PTR, HB_DYN_CTYPE_LONG, HB_DYN_CTYPE_CHAR_PTR },;
::hConnection, @cQuery, cQuery,  Len(cQuery), "\'")				   
*/				   


METHOD mysql_store_result() CLASS RDBMS_MySql	

RETU hb_DynCall( { "mysql_store_result", ::pLib, hb_bitOr( ::nSysLong,;
                   ::nSysCallConv ), ::nSysLong }, ::hMySQL )



METHOD mysql_num_fields( hRes ) CLASS RDBMS_MySql	

RETU hb_DynCall( { "mysql_num_fields", ::pLib, hb_bitOr( HB_DYN_CTYPE_LONG_UNSIGNED,;
                   ::nSysCallConv ), ::nSysLong }, hRes )				   
				   
				   
METHOD mysql_fetch_field( hRes ) CLASS RDBMS_MySql	

RETU hb_DynCall( { "mysql_fetch_field", ::pLib, hb_bitOr( ::nSysLong,;
                   ::nSysCallConv ), ::nSysLong }, hRes )	
				   

	   
METHOD mysql_fetch_row( hRes ) CLASS RDBMS_MySql	

RETU hb_DynCall( { "mysql_fetch_row", ::pLib, hb_bitOr( ::nSysLong,;
                   ::nSysCallConv ), ::nSysLong }, hRes )	  



METHOD mysql_free_result( hRes ) CLASS RDBMS_MySql	

RETU hb_DynCall( { "mysql_free_result", ::pLib,;
                   ::nSysCallConv, ::nSysLong }, hRes )


METHOD Exit() CLASS RDBMS_MySql

    IF ValType( ::pLib ) == "P"
	
		//? "MySQL library properly freed: ", HB_LibFree( ::pLib )
		
		//? 'Exit Method ', ::pLib, ::hMySql
		::MySql_Close()
		
		HB_LibFree( ::pLib )
		
		::pLib := NIL
		::hMySql := NIL
		
		//? 'Exit Method 2', ::pLib, ::hMySql
    ENDIF 
	
RETU NIL


//	------------------------------------------------------------

METHOD d( c ) CLASS RDBMS_MySql

	hb_default( @c, '' )
	
	WAPI_OUTPUTDEBUGSTRING( valtochar(c) + chr(13) + chr(10) )
	
retu nil 


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

function hb_addslashes( c )

	c := StrTran( c, '\', '\\' )
	c := StrTran( c, "'", "\'" )
	c := StrTran( c, '"', '\"' )
	
	//	Pending byte NULL
	
retu c	

function hb_htmlencode( cString )

   local cChar, cRet := "" 

   for each cChar in cString
		do case
			case cChar == "<"	; cChar := "&lt;"
			case cChar == '>'	; cChar := "&gt;"     				
			case cChar == "&"	; cChar := "&amp;"     
			case cChar == '"'	; cChar := "&quot;" 
			case cChar == "'"	; cChar := "&apos;"   			          
		endcase
		
		cRet += cChar 
   next
	
RETURN cRet


