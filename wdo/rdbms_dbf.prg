/*	---------------------------------------------------------
	File.......: rdbms_dbf.prg
	Description: Conexión a Dbf
	Author.....: Carles Aubia Floresvi
	Date:......: 26/07/2019
	--------------------------------------------------------- */ 

	
#define VERSION_RDBMS_DBF			'0.1b'

//#define _SET_AUTOPEN          45
	
CLASS RDBMS_Dbf FROM RDBMS


	DATA cAlias 							INIT ''
	DATA cError 							INIT ''
	DATA cPath 								INIT ''
	DATA cRdd 								INIT ''
	DATA lExclusive						INIT .F.
	DATA lRead								INIT .F.
	DATA lOpen								INIT .F.
	DATA lConnect							INIT .F.
	DATA cError 							INIT ''	
	DATA bExit								//INIT {|| AP_RPUTS( '<h3>Destructor Class...</h3>' )}
	
		
	CLASSDATA cDefaultPath 				INIT hb_getenv( 'PRGPATH' )
	CLASSDATA cDefaultRdd 					INIT 'DBFCDX'
	CLASSDATA nTime							INIT 10
	
	METHOD New() 							CONSTRUCTOR
	
	//	Common methods

	METHOD Open()
	METHOD Close()	
	METHOD Reset()	
	
	METHOD Count()  						INLINE IF ( ::lOpen, (::cAlias)->( RecCount() ), 0 )
	METHOD FieldPos( n )  				INLINE IF ( ::lOpen, (::cAlias)->( FieldPos( n ) ), '' )
	METHOD FieldName( n )  				INLINE IF ( ::lOpen, (::cAlias)->( FieldName( n ) ), '' )
	METHOD FieldGet( ncField )  			INLINE IF ( ::lOpen, (::cAlias)->( FieldGet( If( ValType( ncField ) == "C", ::FieldPos( ncField ), ncField ) ) ), '' )
    METHOD FieldPut( ncField, uValue )	
	
    METHOD Next( n )  						INLINE IF ( ::lOpen, (::cAlias)->( DbSkip( 1 ) ), NIL )
    METHOD Prev( n )  						INLINE IF ( ::lOpen, (::cAlias)->( DbSkip( -1 ) ), NIL )
    METHOD First() 						INLINE IF ( ::lOpen, (::cAlias)->( DbGoTop() ), NIL )
    METHOD Last() 							INLINE IF ( ::lOpen, (::cAlias)->( DbGoBottom() ), NIL )
	
	METHOD SetError( cError )		
	METHOD Version()						INLINE VERSION_RDBMS_DBF
	
	//	Particular methods...
	
    METHOD GoTo( n ) 						INLINE IF ( ::lOpen, (::cAlias)->( DbGoTo( n ) ), NIL )	
    METHOD Recno() 						INLINE IF ( ::lOpen, (::cAlias)->( Recno() ), -1 )	
    METHOD Focus( cFocus ) 	
    METHOD Seek( cSeek, lSoftSeek ) 
    METHOD BOF()							INLINE IF ( ::lOpen, (::cAlias)->( Bof() ), NIL )	
    METHOD EOF()							INLINE IF ( ::lOpen, (::cAlias)->( Eof() ), NIL )		
    METHOD Deleted()						INLINE IF ( ::lOpen, (::cAlias)->( Deleted() ), NIL )		
    METHOD Delete()						INLINE IF ( ::lOpen, (::cAlias)->( DbDelete() ), NIL )		
    METHOD Recall()						INLINE IF ( ::lOpen, (::cAlias)->( DbRecall() ), NIL )		
    METHOD Append()	
    METHOD Rlock()							
    METHOD Unlock()						INLINE IF ( ::lOpen, (::cAlias)->( DbUnlock() ), NIL )							
    METHOD Zap()							INLINE IF ( ::lOpen, (::cAlias)->( __DbZap() ), NIL )							
    METHOD Pack()							INLINE IF ( ::lOpen, (::cAlias)->( __DbPack() ), NIL )							
   

	//	Extended func
	
    METHOD Info()							
	
	
//	DESTRUCTOR  Exit()					

ENDCLASS

METHOD New( cDbf, cIndex, lOpen ) CLASS RDBMS_Dbf

	hb_default( @cDbf, '' )
	hb_default( @cIndex, '' )
	hb_default( @lOpen, .T. )
	
	::cDbf		:= cDbf
	::cIndex	:= cIndex	
	
	::cPath 	:= ::cDefaultPath
	::cRdd 		:= ::cDefaultRdd	
	
	IF lOpen .AND. !empty( ::cDbf )
	
		::Open()
	
	ENDIF	

RETU SELF


METHOD Open() CLASS RDBMS_Dbf

	LOCAL oError
	LOCAL cError 	 	:= ''
    LOCAL nIni  		:= 0
    LOCAL nLapsus  	:= ::nTime
    LOCAL bError   	:= Errorblock({ |o| ErrorHandler(o) })	
	LOCAL cFileDbf 	:= ''
	LOCAL cFileCdx 	:= ''
	LOCAL lAutoOpen	:= Set( _SET_AUTOPEN, .F. )	//	SET AUTOPEN OFF			
	
	//	Check files...

		IF !empty( ::cDbf )
		
			cFileDbf := ::cPath + '/' + ::cDbf

			IF !File( cFileDbf )
			   ::SetError( 'Tabla de datos no existe: ' + ::cDbf )
			   RETU .F.
			ENDIF
			
		ELSE
		
			RETU .F.
			
		ENDIF
		
		IF !empty( ::cIndex ) 
		
			cFileCdx := ::cPath + '/' + ::cIndex

			IF !File( cFileCdx )
				::SetError( 'Indice de datos no existe: ' + ::cIndex )
				RETU .F.
			ENDIF
			
		ENDIF
		
	//	Open table dbf...
	
		nIni  		:= Seconds()
		

		BEGIN SEQUENCE


			  WHILE nLapsus >= 0

				 IF Empty( ::cAlias )
					::cAlias := NewAlias()
				 ENDIF


				 DbUseArea( .T., ::cRDD, cFileDbf, ::cAlias, ! ::lExclusive, ::lRead )
		
				 IF !Neterr() .OR. ( nLapsus == 0 )
					 EXIT
				 ENDIF


				 //SysWait( 0.1 )

				 nLapsus := ::nTime - ( Seconds() - nIni )

				 //SysRefresh()

			  END

		
			  IF NetErr()
				 ::SetError( 'Error de apertura de: ' + cFileDbf )
				ELSE
				 ::cAlias := Alias()
				 ::lOpen  := .t.
 				 
				 IF !empty( cFileCdx )
					SET INDEX TO (cFileCdx )			 			 
				 ENDIF
				 
			  ENDIF
	

		   RECOVER USING oError	

				cError += if( ValType( oError:SubSystem   ) == "C", oError:SubSystem, "???" ) 
				cError += if( ValType( oError:SubCode     ) == "N", " " + ltrim(str(oError:SubCode )), "/???" ) 
				cError += if( ValType( oError:Description ) == "C", " " + oError:Description, "" )		
			
				::SetError( cError )			

	   END SEQUENCE	
	
	   
	// Restore handler 		   

		ErrorBlock( bError )   
		Set( _SET_AUTOPEN, lAutoOpen )		
	
RETU NIL


METHOD Reset() CLASS RDBMS_Dbf

	::cAlias := ''
	::lOpen  := .f.
	
RETU NIL

METHOD Close() CLASS RDBMS_Dbf

    IF ::lOpen
       IF Select( ::cAlias ) > 0
         (::cAlias)->( DbCloseArea() )
       ENDIF
	   
	   ::Reset()	   	   
    ENDIF

RETU NIL


METHOD SetError( cError ) CLASS RDBMS_Dbf

	::cError := cError
	
	IF ::lShowError			
		? '<h3>Error', ::cDbf, ' => ', ::cError, '</h3>'
	ENDIF
	
RETU NIL



METHOD FieldPut( ncField, uValue ) CLASS RDBMS_Dbf

	LOCAL lUpdated := .F.
	LOCAL cField
	
	IF !::lOpen	
		RETU .F.
	ENDIF				
	

	If ValType( ncField ) == "C"

		//cField := ::FieldPos( ncField )
		cField := (::cAlias)->(FieldPos( ncField ) )
	ELSE

		cField := ncField 
	ENDIF				

	(::cAlias)->( FieldPut( cField, uValue ) )

	lUpdated := .T.

RETU lUpdated

METHOD Focus( cTag ) CLASS RDBMS_Dbf

	IF ::lOpen	
		( ::cAlias )->( OrdSetFocus( cTag ) )	
	ENDIF
	
RETU NIL

METHOD Seek( cSeek, lSoftSeek ) CLASS RDBMS_Dbf

	LOCAL lFound := .F.
	
	__defaultNIL( @lSoftSeek, .F. )	

	IF ! ::lOpen	
		RETU .F.
	ENDIF
	
	lFound 	:= (::cAlias)->( DbSeek( cSeek, lSoftSeek ) )

RETU lFound

METHOD Append() CLASS RDBMS_Dbf

    LOCAL nlapsus       := 0	
	LOCAL nIni

	IF ! ::lOpen	
		RETU .F.
	ENDIF	
	
	nIni := Seconds()
	
    WHILE nLapsus >= 0 

       (::cAlias)->( DbAppend() )

       IF !Neterr() .or. ( nLapsus == 0 )
           EXIT
       ENDIF

       nLapsus := ::nTime - ( seconds() - nIni )

    END		
	
RETU IF( !Neterr(), .t., .f. )

METHOD RLock( xIdentidad ) CLASS RDBMS_Dbf

    LOCAL nlapsus	:= 0
	LOCAL lRlock 	:= .F.
	LOCAL nIni
	
	IF ! ::lOpen	
		RETU .F.
	ENDIF	
	
	nIni := Seconds()
	
    WHILE nLapsus >= 0 

       lRlock := (::cAlias)->( DbRlock( xIdentidad ) )

       IF !Neterr() .or. ( nLapsus == 0 )
           EXIT
       ENDIF

       nLapsus := ::nTime - ( seconds() - nIni )

    END		
	
RETU lRlock

METHOD Info() CLASS RDBMS_Dbf

	LOCAL cHtml 	:= ''
	LOCAL cKey 	:= ''
	LOCAL n 		:= 1

	cHtml := '<h3>Info Table</h3>'	
	
	cHtml := '<style>'
	cHtml += '#routes tr:hover {background-color: #ddd;}'
	cHtml += '#routes tr:nth-child(even){background-color: #e0e6ff;}'
	cHtml += '#routes { font-family: "Trebuchet MS", Arial, Helvetica, sans-serif;border-collapse: collapse; width: 100%; }'
	cHtml += '#routes thead { background-color: #425ecf;color: white;}'
	cHtml += '</style>'
	cHtml += '<table id="routes" border="1" cellpadding="3" >'
	cHtml += '<thead ><tr><td align="center">Info</td><td>Description</td></tr></thead>'
	cHtml += '<tbody >'
	
	cHtml += '<tr><td><b>' + 'Dbf' 		+ '<b></td><td>' + ::cDbf + '</td></tr>'
	cHtml += '<tr><td><b>' + 'Index' 		+ '<b></td><td>' + ::cIndex + '</td></tr>'
	cHtml += '<tr><td><b>' + 'RecCount' 	+ '<b></td><td>' + ltrim(str(::Count())) + '</td></tr>'
	
	WHILE ( ! empty( cKey := (::cAlias)->( OrdKey( n ) ) ) )
	
		cHtml += '<tr><td><b>' + 'Tag ' + ltrim(str(n))  	+ '<b></td><td>' + cKey + '</td></tr>'
	
		n++
	
	END

/*	
	FOR n := 1 TO nLen 		
		cHtml += '<tr>'
		cHtml += '<td align="center">' + ltrim(str(aMapSort[n][ MAP_ORDER ])) + '</td>'
		cHtml += '<td align="center">' + aMapSort[n][ MAP_METHOD ] + '</td>'
		cHtml += '<td>' + aMapSort[n][ MAP_ID ] + '</td>'
		cHtml += '<td>' + aMapSort[n][ MAP_ROUTE ] + '</td>'
		cHtml += '<td>' + aMapSort[n][ MAP_CONTROLLER ] + '</td>'
		cHtml += '<td>' + aMapSort[n][ MAP_QUERY ] + '</td>'
		cHtml += '<td>' + valtochar(aMapSort[n][ MAP_PARAMS ]) + '</td>'
		cHtml += '</tr>'
	NEXT		
*/	
	cHtml += '</tbody></table><hr>'
	
	?? cHtml


RETU NIL

/*
METHOD Exit() CLASS RDBMS_Dbf

	IF  ::lOpen	
	
		IF Valtype( ::bExit ) == 'B'
			Eval( ::bExit )
		ENDIF
	
		::Close() 		
		
	ENDIF
	
RETU NIL
*/

*-----------------------------------
STATIC FUNCTION ErrorHandler(oError)
*-----------------------------------

    BREAK oError

RETU NIL

*------------------
FUNCTION NewAlias()
*------------------

    LOCAL n      	:= 0
    LOCAL cAlias
	LOCAL cSeed 	:= 'ALIAS'

    cAlias  := cSeed + Ltrim(Str(n++))

    WHILE Select(cAlias) != 0
          cAlias := cSeed + Ltrim(Str(n++))
    END

RETU cAlias


