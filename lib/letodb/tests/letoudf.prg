/*  $Id$  */

#include "dbinfo.ch"
#include "set.ch"

#ifdef __LINUX__
   #define DEF_SEP      '/'
   #define DEF_CH_SEP   '\'
#else
   #define DEF_SEP      '\'
   #define DEF_CH_SEP   '/'
#endif

/* 
 * File version
 */
FUNCTION UDF_Version
   RETURN "1.08"

FUNCTION UDF_Init
/*
 * This function called immediately after loading letoudf.hrb, if exist
 */
   SET AUTORDER TO 1
   RETURN Nil

/*
 * This sample function demonstrates how to use udf function on Letodb server
 *
 * Function Call from client:
 *
 * cRecBuf := Leto_Udf("UDF_AppendRec", <cFieldName>, [<cOrder>|<nOrder>], [<xMin>])
 *
 * The function return buffer of appended record. After call of Leto_ParseRec
 *
 * Leto_ParseRec( cRecBuf )
 * 
 * Internal data of rddleto is filled from record buffer.
 */

FUNCTION UDF_AppendRec( nUserStru, cFieldName, xOrder, xMin )
   LOCAL nPos := FieldPos( cFieldName )
   LOCAL xKey, lApp, lOver := .F.

   IF ! Empty( xOrder )
      OrdSetFocus( xOrder )
   ENDIF
   IF leto_TableLock( nUserStru, 1 )

      dbGoBottom()
      xKey := FieldGet( nPos )
      IF Empty(xKey) .and. ! Empty(xMin)
         xKey := xMin
      ENDIF

      IF ValType( xKey ) == "N"
         xKey ++
         IF hb_FieldType(nPos) $ 'NF'
            lOver := xKey > Val( Replicate( "9", hb_FieldLen( nPos ) ) )
         ELSEIF hb_FieldLen( nPos ) == 2
            lOver := xKey > 0x7FFF
         ELSEIF hb_FieldLen( nPos ) == 4
            lOver := xKey > 0x7FFFFFFF
         ENDIF
      ELSEIF ValType( xKey ) == "C"
         xKey := StrZero( Val(xKey) + 1, Len(xKey) )
         lOver := (xKey = '*')
      ENDIF

      IF lOver
         lApp := .F.
      ELSE
         lApp := ( UDF_Append( nUserStru ) != Nil )
      ENDIF

      IF lApp
         FieldPut( nPos, xKey )
         dbCommit()
      ENDIF
      leto_TableUnLock( nUserStru, 1 )
   ELSE
      lApp := .F.
   ENDIF

   RETURN if( lApp, leto_rec( nUserStru ), Nil )

FUNCTION UDF_Append( nUserStru )
   LOCAL lApp, lSetDel

   lSetDel := Set( _SET_DELETED, .f. )
   dbGoTop()
   Set( _SET_DELETED, lSetDel )
   IF Deleted() .and. Empty( OrdKeyVal() )
      IF( lApp := leto_RecLock( nUserStru ) )
         dbRecall()
      ENDIF
   ELSE
      dbAppend()
      IF ( lApp := ! NetErr() )
         leto_RecLock( nUserStru, RecNo() )
      ENDIF
   ENDIF
   RETURN if( lApp, leto_rec( nUserStru ), Nil )

/*
 * This sample function delete records on scope xScope, xScopeBottom and filter <cFilter>
 */
FUNCTION UDF_DeleteRecs( nUserStru, xScope, xScopeBottom, xOrder, cFilter, lDeleted )
   LOCAL aRecs := {}, n

   leto_SetEnv( xScope, xScopeBottom, xOrder, cFilter, lDeleted )
   dbEval({|| AADD(aRecs, RecNo())})
   leto_ClearEnv( xScope, xScopeBottom, cFilter )

   FOR EACH n in aRecs
      dbGoto(n)
      IF leto_RecLock( nUserStru, n )
         ClearRec()
         leto_RecUnlock( nUserStru, n )
      ENDIF
   NEXT
   dbCommit()

   RETURN leto_rec( nUserStru )

STATIC FUNCTION ClearRec
   LOCAL nCount := FCount(), nLoop, xValue

   dbDelete()
   FOR nLoop := 1 to nCount
      xValue := Nil
      SWITCH HB_FIELDTYPE( nLoop )
         CASE "C"
         CASE "M"
            xValue := ""
            EXIT
         CASE "N"
         CASE "F"
         CASE "I"
         CASE "Y"
         CASE "Z"
         CASE "2"
         CASE "4"
         CASE "8"
         CASE "B"
            xValue := 0
            EXIT
         CASE "L"
            xValue := .F.
            EXIT
         CASE "D"
         CASE "T"
            xValue := CTOD( "" )
            EXIT
      ENDSWITCH
      IF xValue != Nil
         FieldPut( nLoop, xValue )
      ENDIF
   NEXT
   RETURN Nil

/*
 * UDF_UpdCascade - cascade update key fields in main and relation table
 * Parameters:
 *   nRecNo       - record number in the main table
 *   cKeyField    - field name in the main table (primary key)
 *   xKeyNew      - new value of key field
 *   cClientAlias - client alias of the relation table
 *   cKeyField2   - field name in the relation table (foreign key)
 *   xOrder       - order name or order number in the relation table
 *
 * This function return array of record buffer in two tables
 * Call from client:
 *
 * aRecBuf := Leto_Udf("UDF_UpdCascade", ... )
 * (table1)->( leto_ParseRec( aRecBuf[1] ) )
 * (table2)->( leto_ParseRec( aRecBuf[2] ) )
 */
FUNCTION UDF_UpdCascade( nUserStru, nRecNo, cKeyField, xKeyNew, cClientAlias, cKeyField2, xOrder )
   LOCAL xKeyOld, cLetoAlias, cArea := Alias()
   LOCAL nPos := FieldPos( cKeyField ), nPos2
   LOCAL cRecBuf1, cRecBuf2

   dbGoto( nRecNo )
   xKeyOld := FieldGet( nPos )
   IF xKeyOld != xKeyNew .and. leto_RecLock( nUserStru, nRecNo )
      FieldPut( nPos, xKeyNew )
      leto_RecUnlock( nUserStru, nRecNo )
      cRecBuf1 := leto_rec( nUserStru )

      cLetoAlias := leto_Alias( nUserStru, cClientAlias )
      dbSelectArea( cLetoAlias )
      IF Empty( cKeyField2 )
         cKeyField2 := cKeyField
      ENDIF
      nPos2 := FieldPos( cKeyField2 )
      IF ! Empty( xOrder )
         ordSetFocus( xOrder )
      ENDIF
      WHILE dbSeek( xKeyOld )
         IF leto_RecLock( nUserStru, RecNo() )
            FieldPut( nPos2, xKeyNew )
            leto_RecUnlock( nUserStru, RecNo() )
         ELSE
            EXIT
         ENDIF
      ENDDO
      dbSeek( xKeyNew )
      cRecBuf2 := leto_rec( nUserStru )

      dbSelectArea( cArea )

   ENDIF
   RETURN { cRecBuf1, cRecBuf2 }

/*
 * UDF_FilesExist - check files existence at the specified path
 * Parameters:
 * cPaths - list of directories, delimited with comma
 * aFiles - array of filenames without path to check
 *
 * This function return array of path for each file or "-" symbol,
 * if file doesn't exist
 */
FUNCTION UDF_FilesExist( nUserStru, cPaths, aFiles)
   LOCAL aRet := {}, cFile, lFound, cPath
   LOCAL cDataPath := leto_GetAppOptions( 1 )
   LOCAL aPath := leto_getPath( cPaths )

   for each cFile in aFiles
      lFound := .f.
      for each cPath in aPath
         if File( StrTran( cDataPath + cPath + cFile, DEF_CH_SEP, DEF_SEP ) )
            AADD( aRet, cPath )
            lFound := .t.
            exit
         endif
      next
      if ! lFound
         AADD( aRet, "-" )
      endif
   next

   RETURN aRet

STATIC FUNCTION leto_getPath( cPaths )
   LOCAL aPath := hb_ATokens( cPaths, "," ), nI
   for nI := 1 to len( aPath )
      if ! ( Right( aPath[ nI ], 1) $ DEF_CH_SEP + DEF_SEP )
         aPath[ nI ] += DEF_CH_SEP
      endif
   next
   RETURN aPath

/*
 * UDF_Locate function locate record on scope xScope, xScopeBottom and filter <cFilter>
   If lLast parameter specified, function locate the last occurence of record.
   If record isn't found, UDF_Locate returns eof() value.
 */
FUNCTION UDF_Locate( nUserStru, xScope, xScopeBottom, xOrder, cFilter, lDeleted, lLast )

   leto_SetEnv( xScope, xScopeBottom, xOrder, cFilter, lDeleted )

   IF lLast == Nil
      GO TOP
   else
      GO BOTTOM
   endif

   leto_ClearEnv( xScope, xScopeBottom, cFilter )

   RETURN leto_rec( nUserStru )

/*
 * UDF_dbEval function returns buffer with records by order <xOrder>, and for condition,
 * defined in <xScope>, <xScopeBottom>, <cFilter>, <lDeleted> parameters
 * Function call from client:
   
   leto_ParseRecords( leto_Udf('UDF_dbEval', <xScope>, <xScopeBottom>, <xOrder>, <cFilter>, <lDeleted> ) )
   while ! eof()
      ...
      skip
   enddo
   dbInfo( DBI_CLEARBUFFER )

 */
FUNCTION UDF_dbEval( nUserStru, xScope, xScopeBottom, xOrder, cFilter, lDeleted )
   LOCAL cRecs

   leto_SetEnv( xScope, xScopeBottom, xOrder, cFilter, lDeleted )
   GO TOP
   cRecs := leto_dbEval( nUserStru )
   leto_ClearEnv( xScope, xScopeBottom, cFilter )

   RETURN cRecs

/*
 * UDF_getFields function returns two-dimensional array with with records by order <xOrder>,
   and for condition, defined in <xScope>, <xScopeBottom>, <cFilter>, <lDeleted> parameters
   for fields or expressons, defined in comma-separated <cFields> parameter.
   aRelat - array with description of relations in format:
   { { <cRelAlias>, <cOrderName>, <cKeyExpr> }, ...}
*/
FUNCTION UDF_getFields( nUserStru, cFields, xScope, xScopeBottom, xOrder, cFilter, lDeleted, aRelat )
   LOCAL aRecs := {}, aRec, ar, cAlias, nAt, cRelAlias
   LOCAL aFields := hb_ATokens( cFields, "," ), cf
   LOCAL aPos := {}, nPos, xKey

   IF aRelat != nil
      cAlias := Alias()
      FOR EACH ar in aRelat
         cRelAlias := Leto_Alias( nUserStru, ar[1] )
         ( cRelAlias )->( ordSetFocus( ar[2] ) )
         AADD( ar, cRelAlias )
      NEXT
      dbSelectArea( cAlias )
   ENDIF
   FOR EACH cf IN aFields
      IF ( nAt := At( "->", cf ) ) != 0 .and. aRelat != nil
         cAlias := Upper( Left( cf, nAt - 1 ) )
         FOR EACH ar in aRelat
            IF Upper( ar[1] ) == cAlias
               cf := ar[4] + Substr( cf, nAt )
               EXIT
            ENDIF
         NEXT
      ENDIF
      AADD( aPos, IIF( ( nPos := FieldPos( cf ) ) != 0, nPos, cf ) )
   NEXT
   leto_SetEnv( xScope, xScopeBottom, xOrder, cFilter, lDeleted )
   GO TOP
   WHILE ! eof()
      aRec := {}
      IF aRelat != nil
         FOR EACH ar IN aRelat
            xKey := &( ar[3] )
            ( ar[4] )->( dbSeek( xKey ) )
         NEXT
      ENDIF
      FOR EACH nPos IN aPos
         AADD( aRec, IIF( ValType( nPos ) == 'N', FieldGet( nPos ), &nPos ) )
      NEXT
      AADD( aRecs, aRec )
      SKIP
   ENDDO
   leto_ClearEnv( xScope, xScopeBottom, cFilter )

   RETURN aRecs

FUNCTION UDF_Trans( nUserStru, cTo )
/*
 * UDF_Trans copy all records from current area to area with <cTo> client alias
   with conversion between numeric and character fields.
*/
   LOCAL cArea := Alias()
   LOCAL cAliasTo := leto_Alias( nUserStru, cTo )
   LOCAL lSetDel
   LOCAL lRes := .T., oError

   dbSelectArea( cArea )
   IF ! Empty( cAliasTo )
      lSetDel := Set( _SET_DELETED, .f. )
      BEGIN SEQUENCE WITH { |e|break( e ) }
         OrdSetFocus( 0 )
         GO TOP
         WHILE ! eof()
            UDF_TransRec( cAliasTo )
            SKIP
         ENDDO
         (cAliasTo)->(dbCommit())
      RECOVER USING oError
         WrLog('UDF_Trans error: ' + cArea + '-->' + cTo + ' ' +;
 oError:description + if(!Empty(oError:operation), ':' + oError:operation,'') +;
 ' recno ' + LTrim(Str(RecNo())))
         lRes := .F.
      END SEQUENCE
      Set( _SET_DELETED, lSetDel )
   ENDIF
   RETURN lRes

STATIC FUNCTION UDF_TransRec( cAliasTo )
   LOCAL nPos1, nPos2, xVal, cFName, ct1, ct2

   (cAliasTo)->( dbAppend() )
   IF ! NetErr()
      FOR nPos1 := 1 to FCount()
         cFName := FieldName( nPos1 )
         IF (nPos2 := (cAliasTo)->(FieldPos(cFName))) # 0 .and. ! (cAliasTo)->(hb_FieldType(nPos2)) $ '+^'
            xVal := (cAliasTo)->(FieldGet(nPos2))
            ct2 := ValType( xVal )
            xVal := FieldGet( nPos1 )
            IF IIF( ct1 = "C", ! ( xVal = Space( Len( xVal ) ) ), ! Empty( xVal ) )
               ct1 := ValType( xVal )
               IF ct2 = ct1
                  IF ct1 == "C"
                     xVal := RTrim( xVal )
                  ENDIF
                  (cAliasTo)->(FieldPut( nPos2, xVal ))
               ELSEIF ct1 = 'C' .and. ct2 = 'N'
                  (cAliasTo)->(FieldPut( nPos2, Val(xVal)))
               ELSEIF ct1 = 'N' .and. ct2 = 'C'
                  (cAliasTo)->(FieldPut( nPos2, Str(xVal, hb_FieldLen(nPos2), hb_FieldDec(nPos2))))
               ENDIF
            ENDIF
         ENDIF
      NEXT
      IF Deleted()
         (cAliasTo)->( dbDelete() )
      ENDIF
   ENDIF
   RETURN Nil

FUNCTION UDF_OpenTables( nUserStru, aTables, cPaths )
/*
 * UDF_OpenTables open a tables, described in aTables array,
   and return to client an array with structure of opened tables
   Each table described with array of (at least) 1 to 5 elements:
   {<cFileName>, [<cAlias>], [<lShared>], [<lReadOnly>], [<cdp>]}
   Tables are opened on the server by one request from the client,
   the server returns information on open tables in the array,
   and then elements of array is transferred to the "use" command.
   The "use" command opens tables without the request to the server.
   The <alias> in the "use" command is mandatory parameter.

   Example of usage UDF_OpenTables from client:

   if leto_UDFExist( "UDF_OpenTables" )

      aAreas := leto_UDF( ""UDF_OpenTables"", {{"table1",, .t.}, {"table2",, .t.}, {"table3",, .t.}} )
      use (aAreas[1]) alias table1 shared new
      use (aAreas[2]) alias table2 shared new
      use (aAreas[3]) alias table3 shared new

   else

      use table1 alias table1 shared new
      use table2 alias table2 shared new
      use table3 alias table3 shared new

   endif

 */
   LOCAL aOpen := {}, aItem, nLen, cTable
   LOCAL cDataPath, aPath, cPath, cOpen

   IF cPaths != nil
      cDataPath := leto_GetAppOptions( 1 )
      aPath := leto_getPath( cPaths )
   ENDIF

   FOR EACH aItem IN aTables
      cTable := aItem[1]
      IF ! Empty( aPath )
         FOR EACH cPath IN aPath
            IF File( StrTran( cDataPath + cPath + cTable + ".dbf", DEF_CH_SEP, DEF_SEP ) )
               cTable := cPath + cTable
               exit
            ENDIF
         NEXT
      ENDIF
      nLen := len( aItem )
      cOpen := leto_Use( nUserStru, cTable,;
                         IIF( nLen >=2, aItem[2], ),;
                         IIF( nLen >=3, aItem[3], ),;
                         IIF( nLen >=4, aItem[4], ),;
                         IIF( nLen >=5, aItem[5], ) )
      AADD( aOpen, "+" + cTable + ";" + Substr( cOpen, Asc( Left( cOpen, 1 ) ) + 2 ) )
   NEXT

   RETURN aOpen
