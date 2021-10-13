//	-----------------------------------------------------------	//

CLASS TDataset 

	DATA oDb 
	DATA cTable 
	DATA cIndex 
	DATA cFocus
	DATA cPath 							INIT ''
	DATA aFields 						INIT {=>}
	DATA hRow 							INIT {=>}
	
	DATA hCfg							INIT { 'wdo' => 'DBF' }


	METHOD  New() 						CONSTRUCTOR
	METHOD  Open()
	
	METHOD  ConfigDb( hCfg )
	
	METHOD  AddField( cField ) 				INLINE ::aFields[ cField ] := {}
   
	METHOD  RecCount()
	METHOD  GetId( cId )
	
	METHOD  Next()						
	METHOD  Prev()
	METHOD  Row()							INLINE ::hRow
	METHOD  Get( cField )					INLINE ::hRow[ cField ]
	
	METHOD  Focus( cTag )					INLINE  ::oDb:Focus( cTag )
	METHOD  Seek( uValue )					INLINE  IF( ::oDb:Seek( uValue ), ::Load(), ::Blank() )
	METHOD  Eof()							INLINE  ::oDb:Eof()
	
	METHOD  Load()
	METHOD  Blank()
	METHOD  LoadPage( nRecs, nRecnoStart )
	

    ERROR HANDLER OnError( uParam1 )	
   
ENDCLASS

METHOD New( cTable, cIndex ) CLASS TDataset

	__defaultNIL( @cTable, '' )
	__defaultNIL( @cIndex, '' )
	
	::cTable := cTable
	::cIndex := cIndex		
	
	//::hCfg[ 'wdo' ] := 'DBF'

RETU SELF

METHOD Open( lExclusive ) CLASS TDataset


	__defaultNIL( @lExclusive, .F. )

	
	IF ::hCfg[ 'wdo' ] == 'DBF'
	
		IF ValType( ::oDb ) == 'O' .AND. ::oDb:lOpen
			::hRow := ::Load()	
			RETU NIL
		ENDIF

		::oDb := WDO():Dbf( ::cTable, ::cIndex , .F. )
			::oDb:lExclusive := lExclusive
			
		IF !empty( ::cPath )
			::oDb:cPath := ::cPath
		ENDIF
		
		::oDb:Open()

		::hRow := ::Load()	
	
	ELSE
	
		::oDb := WDO():Rdbms( ::hCfg[ 'wdo' ],;
								::hCfg[ 'server' ],;
								::hCfg[ 'user'] ,;
								::hCfg[ 'password' ],;
								::hCfg[ 'database' ],;
								::hCfg[ 'port' ] )
								

								
	
	ENDIF

RETU ::oDb:lOpen

METHOD ConfigDb( hCfg ) CLASS TDataset

	::hCfg := hCfg 

RETU NIL

METHOD RecCount() CLASS TDataset

	LOCAL cSql, oRs, hRes, o
	LOCAL nTotal 

	IF ::hCfg[ 'wdo' ] == 'DBF'
	
		::nTotal := ::oDb:Count()
		
	ELSE
	
		cSql 	:= 'SELECT count(*) as total FROM ' + ::cTable 
		
		hRes 	:= ::oDb:Query( cSql )
		oRs  	:= ::oDb:Fetch_Assoc( hRes ) 
		
		nTotal	:= oRs[ 'total' ]	
		
	ENDIF
	
RETU nTotal


METHOD getId( cId ) CLASS TDataset

	LOCAL hReg 
	LOCAL lFound := .F.
	LOCAL cSql, hRes

	IF ::hCfg[ 'wdo' ] == 'DBF'	
		
		::oDb:Focus( ::cFocus )
		
		IF ( lFound := ::oDb:Seek( cId ) )
			::hRow := ::Load()
		ELSE
			::hRow := ::Blank()
		ENDIF	

	ELSE

		cSql 	:= 'SELECT * FROM ' + ::cTable + ' WHERE ' + ::cFocus + ' = ' + valtochar( cId )

		hRes 	:= ::oDb:Query( cSql )
		
		lFound	:= IF( ::oDb:Count( hRes ) > 0 , .T., .F. )
	
		IF lFound 
			::hRow 	:= ::oDb:Fetch_Assoc( hRes )
		ELSE
			::hRow	:= ''		
		ENDIF		
		
	ENDIF
	
RETU lFound

METHOD Load( lAssoc ) CLASS TDataset

	LOCAL nI, cField 
	
	__defaultNIL( @lAssoc, .T. )
	
	IF lAssoc 		
		::hRow := {=>}			
	ELSE			
		::hRow := {}		
	ENDIF	

	FOR nI := 1 TO Len( ::aFields )
		
		cField := HB_HKeyAt( ::aFields, nI ) 	

		IF lAssoc 

			::hRow[ cField ] :=  ::oDb:FieldGet( cField )
			
		ELSE

			Aadd( ::hRow, ::oDb:FieldGet( cField ) )
		
		ENDIF
		
	NEXT
	


RETU ::hRow

METHOD Blank( lAssoc ) CLASS TDataset

	LOCAL nI, cField 
	
	__defaultNIL( @lAssoc, .T. )	
	
	::oDb:Last()
	::oDb:next()		//	EOF() + 1 

	::hRow := ::Load( lAssoc )

RETU ::hRow

METHOD Next() CLASS TDataset

	::oDb:next()
	::Load()

RETU NIL

METHOD Prev() CLASS TDataset

	::oDb:prev()
	::Load()

RETU NIL

METHOD LoadPage( nRecs, nRecnoStart, lAssoc ) CLASS TDataset

	LOCAL nI		:= 0
	LOCAL aRows	:= {}
	LOCAL cSql, hRes

	__defaultNIL( @nRecs, 10 )
	__defaultNIL( @nRecnoStart, 0 )
	__defaultNIL( @lAssoc, .F. )	
	
	IF ::hCfg[ 'wdo' ] == 'DBF'	
	
		IF nRecnoStart > 0 
			
			::oDb:GoTo( nRecnoStart )
		
		ENDIF
		
		WHILE !::oDb:Eof() .AND. nI < nRecs 
		
			Aadd( aRows, ::Load( lAssoc ) )
			
			::oDb:Next()
			
			nI++
		
		END	

	ELSE
	
		cSql	:= 'Select * from ' + ::cTable + ' limit ' + str(nRecnoStart) + ', ' + str(nRecs)
	
		hRes	:= ::oDb:Query( cSql )		
		aRows	:= ::oDb:FetchAll( hRes )
	
	ENDIF

RETU aRows


METHOD OnError( uParam1 ) CLASS TDataset
/*
    LOCAL nVar
    LOCAL cMsg   := __GetMessage()
    LOCAL nError := If( SubStr( cMsg, 1, 1 ) == "_", 1005, 1004 )

    cMsg := Upper( cMsg )

    IF SubStr( cMsg, 1, 1 ) == "_"    // SET

//     En aquesta clase no dixarem fer una assignacio

        _ClsSetError( _GenError( nError, ::ClassName(), 'No se puede asignar a ningun miembro de la clase' ) )

     ELSE                           // GET

       nVar := AScan( ::hRow, { |x| x[ DATA_ID ] == cMsg } )

       IF nVar > 0

          RETU ::aBuffer[ nVar ][ DATA_RDD ]

         ELSE
           MsgBeep()
           MsgStop( "Taula no definida: " + cMsg , 'ZDb () - Get')
           _ClsSetError( _GenError( nError, ::ClassName(), 'Taula no definida ' + cMsg ) )
       ENDIF

    ENDIF
*/
RETU NIL