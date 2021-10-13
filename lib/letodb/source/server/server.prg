/*  $Id$  */

/*
 * Harbour Project source code:
 * Leto db server
 *
 * Copyright 2008 Alexander S. Kresin <alex / at / belacy.belgorod.su>
 * www - http://www.harbour-project.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA (or visit the web site http://www.gnu.org/).
 *
 * As a special exception, the Harbour Project gives permission for
 * additional uses of the text contained in its release of Harbour.
 *
 * The exception is that, if you link the Harbour libraries with other
 * files to produce an executable, this does not by itself cause the
 * resulting executable to be covered by the GNU General Public License.
 * Your use of that executable is in no way restricted on account of
 * linking the Harbour library code into it.
 *
 * This exception does not however invalidate any other reasons why
 * the executable file might be covered by the GNU General Public License.
 *
 * This exception applies only to the code released by the Harbour
 * Project under the name Harbour.  If you copy code from other
 * Harbour Project or Free Software Foundation releases into a copy of
 * Harbour, as the General Public License permits, the exception does
 * not apply to the code that you add in this way.  To avoid misleading
 * anyone as to the status of such modified files, you must delete
 * this exception notice from them.
 *
 * If you write modifications of your own for Harbour, it is your choice
 * whether to permit this exception to apply to your modifications.
 * If you do not wish that, delete this exception notice.
 *
 */

#include "hbclass.ch"
#include "dbstruct.ch"
#include "rddsys.ch"
#include "common.ch"
#include "dbinfo.ch"
#include "fileio.ch"
#include "error.ch"
#include "rddleto.ch"

#ifndef HB_HRB_BIND_DEFAULT
   #define HB_HRB_BIND_DEFAULT 0x0 
#endif
#define SOCKET_BUFFER_SIZE  8192

   MEMVAR oApp

#ifdef __LINUX__
   ANNOUNCE HB_GTSYS
   REQUEST HB_GT_STD_DEFAULT
   #define DEF_SEP      '/'
   #define DEF_CH_SEP   '\'
#else
#ifndef __CONSOLE__
   ANNOUNCE HB_GTSYS
   REQUEST HB_GT_GUI_DEFAULT
#endif
   #define DEF_SEP      '\'
   #define DEF_CH_SEP   '/'
#endif

   REQUEST ABS, ALLTRIM, AT, CHR, CTOD, DATE, DAY, DELETED, DESCEND, DTOC, DTOS, ;
      EMPTY, I2BIN, L2BIN, LEFT, LEN, LOWER, LTRIM, MAX, MIN, MONTH, OS, PAD, PADC, ;
      PADL, PADR, RAT, RECNO, RIGHT, ROUND, RTRIM, SPACE, STOD, STR, STRZERO, ;
      SUBSTR, REPLICATE, TIME, TRANSFORM, TRIM, UPPER, VAL, YEAR, ;
      hb_ATokens, hb_tokenGet, hb_tokenCount, hb_WildMatch, hb_DiskSpace
   REQUEST TIME, HB_DATETIME, HB_DTOT, HB_TTOD, HB_NTOT, HB_TTON, HB_CTOT, HB_TTOC, ;
      HB_TTOS, HB_STOT, HB_HOUR, HB_MINUTE, HB_SEC, HB_VALTOEXP

   REQUEST FieldPos, FieldGet, FieldPut, Deleted, hb_FieldType, hb_FieldLen, hb_FieldDec
   REQUEST dbGoTop, dbGoBottom, dbSkip, dbGoto, dbSeek, Bof, Eof, dbEval, dbInfo
   REQUEST dbSetFilter, dbClearFilter
   REQUEST dbAppend, dbCommit, RLock, FLock, dbUnlock, dbDelete, dbRecall
   REQUEST ordKeyVal, dbOrderInfo, Alias, Select, dbSelectArea
   REQUEST hb_Hash, hb_HAutoAdd, hb_HHasKey, hb_HPos, hb_HGet, hb_HSet, hb_HDel, hb_HKeyAt,;
      hb_HValueAt, hb_HFill, hb_HClone, hb_HCopy, hb_HMerge, hb_HScan
   REQUEST HB_BITAND, HB_BITOR, HB_BITTEST, HB_BITXOR, HB_BITNOT, HB_BITSET, HB_BITRESET, HB_BITSHIFT

   REQUEST LETO_VARSET, LETO_VARGET, LETO_VARINCR, LETO_VARDECR, LETO_VARDEL, LETO_VARGETLIST

   EXTERNAL leto_SelectArea, leto_Alias, leto_AreaID, leto_ClientID
   EXTERNAL leto_RecLock, leto_RecLockList, leto_RecUnlock
   EXTERNAL leto_TableLock, Leto_TableUnlock
   EXTERNAL letoCloseArea, leto_Use, leto_dbEval

#ifdef __HB_EXT_CDP__
   #include "hbextcdp.ch"
#else
   #include "letocdp.ch"
#endif

   STATIC cDirBase
   STATIC pHrb

PROCEDURE Main( cCommand )
   LOCAL nRes

   cDirBase := hb_dirBase()
   leto_setDirBase( cDirBase )

   IF cCommand != NIL .AND. Lower( cCommand ) == "stop"

      // connect and send QUIT
      PUBLIC oApp := HApp():New()

      IF leto_SendMessage( oApp:nPort, "stop", oApp:ip )
#ifdef __CONSOLE__
         ? "Send STOP to server..."
#else
         WrLog( "Send STOP to server..." )
#endif
      ELSE
#ifdef __CONSOLE__
         ? "Can't STOP the server (not started?)..."
#else
         WrLog( "Can't STOP the server (not started?)..." )
#endif
      ENDIF
      RETURN

   ELSEIF cCommand != NIL .AND. Lower( cCommand ) == "reload"

      // send message to reload letoudf.hrb
      PUBLIC oApp := HApp():New()
      IF ! leto_SendMessage( oApp:nPort, "udf_rel", oApp:ip )
#ifdef __CONSOLE__
         ? "Can't reload letoudf.hrb"
#else
         WrLog( "Can't reload letoudf.hrb" )
#endif
      ENDIF
      RETURN

   ELSE

#ifdef __CONSOLE__

      CLS
      @ 1, 5 SAY "Server listening ..."
      @ 2, 5 SAY "Press [ESC] to terminate the program"
      StartServer()

#endif

#ifdef __WIN_DAEMON__

      StartServer()

#endif

#ifdef __WIN_SERVICE__

      IF cCommand != NIL
         IF Lower( cCommand ) == "install"
            IF leto_serviceInstall()
               WrLog( "LetoDB service has been successfully installed" )
            ELSE
               WrLog( "Error installing LetoDB service: " + Str( letowin_GetLastError() ) )
            ENDIF
            RETURN
         ELSEIF Lower( cCommand ) == "uninstall"
            IF leto_serviceDelete()
              WrLog( "LetoDB service has been deleted" )
            ELSE
              WrLog( "Error deleting LetoDB service: " + Str( letowin_GetLastError() )  )
            ENDIF
            RETURN
         ELSEIF Lower( cCommand ) == "test"
            StartServer()
            RETURN
         ELSE
            ? "LetoDB_mt { install | uninstall }"
         ENDIF
         RETURN
      ENDIF

      IF ! leto_serviceStart( "StartServer" )
         WrLog( "LetoDB service has had some problems: " + Str( letowin_GetLastError() ) )
      ENDIF

//!!!!!!!!!!!!!!!   DO WHILE win_serviceGetStatus() == WIN_SERVICE_RUNNING

#endif

#ifdef __LINUX_DAEMON__

      IF !leto_Daemon()
         WrLog( "Can't become a daemon" )
         RETURN
      ENDIF

      StartServer()

#endif

   ENDIF

   RETURN

PROCEDURE StartServer()
   PUBLIC oApp := HApp():New()

   REQUEST DBFNTX
   REQUEST DBFCDX
#ifdef __BM
   REQUEST BMDBFNTX
   REQUEST BMDBFCDX
#endif

   WrLog( "Leto DB Server has been started." )
   leto_InitSet()

   leto_CreateData()

   leto_HrbLoad()
/*
   IF ! EMPTY( oApp:cTrigger )
      HB_RddInfo( RDDI_TRIGGER, oApp:cTrigger, leto_Driver( oApp:nDriver ) )
   ENDIF
   IF ! EMPTY( oApp:cPendingTrigger )
      HB_RddInfo( RDDI_PENDINGTRIGGER, oApp:cPendingTrigger, leto_Driver( oApp:nDriver ) )
   ENDIF
*/
   IF ! leto_Server( oApp:nPort, oApp:nTimeOut, oApp:ip )
#if __HARBOUR__ > 0x020100 
      WrLog( "Socket error " + hb_socketErrorString() )
#else
      WrLog( "Socket error " )
#endif
   ENDIF

   WrLog( "Server has been closed." )

   RETURN

#if __HARBOUR__ < 0x030000

#xtranslate hb_FNameExt([<n,...>])          => GetExten(<n>)
#xtranslate hb_FNameDir([<n,...>])          => FilePath(<n>)
#xtranslate hb_FNameName([<n,...>])         => CutPath(CutExten(<n>))

STATIC FUNCTION FilePath( fname )
   LOCAL i

   RETURN iif( ( i := RAt( '\', fname ) ) = 0, ;
      iif( ( i := RAt( '/', fname ) ) = 0, "", Left( fname, i ) ), ;
      Left( fname, i ) )

STATIC FUNCTION GetExten( fname )
   LOCAL i

   IF ( i := RAt( '.', fname ) ) == 0
      RETURN ""
   ELSEIF Max( RAt( '/', fname ), RAt( '\', fname ) ) > i
      RETURN ""
   ENDIF

   RETURN Lower( SubStr( fname, i + 1 ) )

FUNCTION CutPath( fname )

   LOCAL i

   RETURN iif( ( i := RAt( '\', fname ) ) = 0, ;
      iif( ( i := RAt( '/', fname ) ) = 0, fname, SubStr( fname, i + 1 ) ), ;
      SubStr( fname, i + 1 ) )

FUNCTION CutExten( fname )

   LOCAL i

   RETURN iif( ( i := RAt( '.', fname ) ) = 0, fname, SubStr( fname, 1, i - 1 ) )

#endif

STATIC FUNCTION leto_hrbLoad
   LOCAL cHrbName := cDirBase + "letoudf.hrb", pInit
   IF File( cHrbName )
      pHrb := hb_HrbLoad(HB_HRB_BIND_DEFAULT, cHrbName )
      IF ! Empty(pHrb)
         WrLog( cHrbName + " has been loaded." )

         IF ! Empty( pInit := hb_hrbGetFunSym(pHrb, 'UDF_Init') )
            hb_ExecFromArray( pInit )
         ENDIF
      ENDIF
   ENDIF
   RETURN Nil

FUNCTION hs_UdfReload()
   IF ! Empty( pHrb )
      hb_hrbUnload( pHrb )
      pHrb := nil
      WrLog( "letoudf.hrb has been unloaded." )
   ENDIF
   leto_hrbLoad()
   RETURN Nil

FUNCTION letoUseArea( nUserStru, cFileName, cAlias, lShared, lReadOnly, cdp )
   LOCAL nAreaID := 0, nLen
   LOCAL cReply

   IF ! Empty( cFileName )

      cReply := leto_Use( nUserStru, cFileName, cAlias, lShared, lReadOnly, cdp )

      nLen := Asc( Left( cReply, 1 ) )
      IF Substr( cReply, nLen + 2, 1) == "+"
         nAreaID := Val( Substr( cReply, nLen + 3 ) )
      ENDIF
   ENDIF

   RETURN nAreaId

FUNCTION leto_Use( nUserStru, cFileName, cAlias, lShared, lReadOnly, cdp )

   IF Left( cFileName, 1 ) != "\" .and. Left( cFileName, 1 ) != "/"
      cFileName := "/" + cFileName
   ENDIF
   IF Empty( cAlias )
      cAlias := cFileName
   ENDIF
   IF lShared == Nil
      lShared := .T.
   ENDIF
   IF lReadOnly == Nil
      lReadOnly := .F.
   ENDIF
   IF cdp == Nil
      cdp := ""
   ENDIF

   RETURN hs_opentable( nUserStru, cFileName + ";" + cAlias + ";" + ;
                        IIF(lShared, "T", "F") + ;
                        IIF(lReadOnly, "T", "F") + ";" + ;
                        cdp + ";" + Chr(13) + Chr(10) )

FUNCTION letoOrdCreate( nUserStru, cBagName, cKey, cTagName, lUnique, cFor,;
   cWhile, lAll, nRecNo, nNext, lRest, lDesc, lCustom, lAdditive )

   IF ! Empty( cKey ) .and. ( ! Empty( cBagName ) .or. ! Empty( cTagName ) )

      IF lUnique == Nil
         lUnique := .F.
      ENDIF
      IF cFor == Nil
         cFor := ""
      ENDIF
      IF cWhile == Nil
         cWhile := ""
      ENDIF
      IF lAll == Nil
         lAll := .F.
      ENDIF
      IF lRest == Nil
         lRest := .F.
      ENDIF
      IF lDesc == Nil
         lDesc := .F.
      ENDIF
      IF lCustom == Nil
         lCustom := .F.
      ENDIF
      IF lAdditive == Nil
         lAdditive := .F.
      ENDIF

      hs_createindex( nUserStru, cBagName + ";" + cTagName + ";" + cKey + ";" + ;
 IIF( lUnique, "T", "F") + ";" + cFor + ";" +  cWhile + ";" + ;
 IIF( lAll, "T", "F" ) + ";" + IIF( Empty( nRecNo ), "", LTrim( Str( nRecNo ) )) + ";" +;
 IIF( Empty( nNext ) , "", LTrim( Str( nNext ) ) ) + ";;" + ;
 IIF( lRest, "T", "F" ) + ";" + IIF( lDesc, "T", "F" ) + ";" + ;
 IIF( lCustom, "T", "F" ) + ";" + IIF( lAdditive, "T", "F" ) + ";" )

   ENDIF

   RETURN Nil

FUNCTION letoOrdListAdd( nUserStru, cBagName )
   IF ! Empty( cBagName )
      hs_openindex( nUserStru, cBagName + ";" )
   ENDIF
   RETURN Nil

FUNCTION hs_opentable( nUserStru, cCommand )                                  // lNoSaveWA!!!
   LOCAL nPos, cReply, cLen, cName, cFileName, cAlias, cRealAlias, cFlags
   LOCAL aStru, i
   LOCAL nTableStru, lShared, lReadonly, cdp
   LOCAL lres := .T. , oError, nDriver, lNoSaveWA := leto_GetAppOptions( 11 )
   LOCAL cDataPath, lShareTables
   LOCAL nAreaID, cRecData

   IF Empty( cName := GetCmdItem( cCommand,1,@nPos ) ) .OR. nPos == 0
      RETURN "-002"
   ENDIF
   cAlias := GetCmdItem( cCommand, nPos + 1, @nPos )
   IF nPos == 0; RETURN "-002"; ENDIF
   cFlags := GetCmdItem( cCommand, nPos + 1, @nPos )
   IF nPos == 0; RETURN "-002"; ENDIF

   lShared := ( Left( cFlags,1 ) == "T" )
   lReadonly := ( SubStr( cFlags,2,1 ) == "T" )

   cdp := GetCmdItem( cCommand, nPos + 1, @nPos )

   cName := StrTran( cName, DEF_CH_SEP, DEF_SEP )
   IF Empty( hb_FNameExt( cName ) )
      cName += ".dbf"
   ENDIF

   IF ( nDriver := leto_getDriver( hb_FNameDir(cName) ) ) == Nil
      nDriver := leto_GetAppOptions( LETO_CDX )
   ENDIF

   IF lNoSaveWA .OR. ( nTableStru := leto_FindTable( cName, @nAreaID ) ) < 0
      nAreaID := leto_CreateAreaID()
      cRealAlias := leto_MakeAlias( nAreaID )
      lShareTables := leto_GetAppOptions( 10 )
      IF Lower(cName) = "/mem:"
         cDataPath := ""
         cName := Substr(cName, 2)
      ELSE
         cDataPath := leto_GetAppOptions( 1 )
      ENDIF
      cFileName := cDataPath + cName

      BEGIN SEQUENCE WITH { |e|break( e ) }
         leto_SetUserEnv( nUserStru )
         dbUseArea( .T. , leto_Driver( nDriver ), ;
            cFileName, cRealAlias, ;
            ( lShareTables .AND. lShared ),   ;
            ( lShareTables .AND. lReadOnly ), ;
            Iif( !Empty(cdp ),cdp,Nil ) )
      RECOVER USING oError
         lres := .F.
      END SEQUENCE
      IF ! lres .AND. ( !lShareTables .AND. lReadonly )
          // file read only
          lres := .T.
          BEGIN SEQUENCE WITH { |e|break( e ) }
             dbUseArea( .T. , leto_Driver( nDriver ), ;
                cFileName, cRealAlias, ;
                ( lShared ),   ;
                ( lReadOnly ), ;
                Iif( !Empty(cdp ),cdp,Nil ) )
          RECOVER USING oError
             lres := .F.
          END SEQUENCE
      ENDIF

      IF !lres
         leto_DelAreaID( nAreaID )
         RETURN ErrorStr(Iif( oError:genCode==EG_OPEN .AND. oError:osCode==32, "-004:","-003:" ), oError, cFileName)
      ENDIF
      IF ( nTableStru := leto_InitTable( nAreaID, cName, nDriver, lShared ) ) < 0
         RETURN "-004"
      ENDIF
      IF leto_InitArea( nUserStru, nTableStru, nAreaID, cAlias, .F. ) < 0
         RETURN "-004"
      ENDIF
   ELSE
      IF !leto_SetShared( nTableStru ) .OR. !lShared
         // The table is already opened exclusively by another user
         RETURN "-004:21-1023-0-0" + Chr(9) + cName
      ENDIF
      IF leto_InitArea( nUserStru, nTableStru, nAreaID, cAlias, .T. ) < 0
         RETURN "-004"
      ENDIF
      BEGIN SEQUENCE WITH { |e|break( e ) }
        IF OrdCount() >= SET( _SET_AUTORDER )
           OrdSetFocus( SET( _SET_AUTORDER ) )
        ENDIF
        dbGoTop()
      RECOVER USING oError
         lres := .F.
      END SEQUENCE
      IF !lres
         RETURN ErrorStr("-004:", oError, cName)
      ENDIF
   ENDIF

   aStru := dbStruct()
   cReply := "+" + LTrim( Str( nAreaID ) ) + ";" + ;
      iif( nDriver == LETO_NTX, "1;", "0;" ) + ;
      LetoMemoInfo() + Leto_LastUpdate( nUserStru ) + ;
      LTrim( Str( Len(aStru ) ) ) + ";"
   FOR i := 1 TO Len( aStru )
      cReply += aStru[i,DBS_NAME] + ";" + aStru[i,DBS_TYPE] + ";" + ;
         LTrim( Str( aStru[i,DBS_LEN] ) ) + ";" + LTrim( Str( aStru[i,DBS_DEC] ) ) + ";"
   NEXT
   cReply += leto_IndexesInfo( nUserStru )
   cRecData := leto_rec( nUserStru )

   IF LEN( cRecData ) <= 0
      RETURN "-004"
   ENDIF

   cReply += cRecData
   cLen := leto_N2B( Len( cReply ) )

   RETURN Chr( Len( cLen ) ) + cLen + cReply

FUNCTION hs_openindex( nUserStru, cCommand )
   LOCAL cReply, cBagName, cBagClean, nPos
   LOCAL oError, lres := .T.
   LOCAL cDataPath, cRecData

   IF Empty( cBagName := GetCmdItem( cCommand, 1, @nPos ) ) .OR. nPos == 0
      RETURN "-002"
   ENDIF

   cBagName  := StrTran( cBagName, DEF_CH_SEP, DEF_SEP )
   IF Lower(cBagName) = "/mem:"
      cDataPath := ""
      cBagName := Substr(cBagName, 2)
   ELSE
      cDataPath := leto_GetAppOptions( 1 )
   ENDIF
   IF DEF_SEP $ cBagName
      IF !EMPTY(cDataPath)
         cBagName := cDataPath + iif( Left( cBagName,1 ) $ DEF_SEP, "", DEF_SEP ) + cBagName
      ENDIF
   ELSE
      cBagName := cDataPath + hb_FNameDir( leto_TableName( nUserStru ) ) + cBagName
   ENDIF
   //hb_FNameSplit( cBagName, Nil, @cBagClean, Nil )
   //cBagClean := Lower( cBagClean )
   cBagClean := Lower( hb_FNameName(cBagName) )
   BEGIN SEQUENCE WITH { |e|break( e ) }
      OrdListAdd( cBagName )
      IF leto_GetAppOptions( LETO_CDX ) == LETO_NTX
      /* WRONG! for second++ users : OrdSetFocus( OrdCount() ) */
         nPos := 0
         DO WHILE nPos < OrdCount()
            IF Lower( OrdBagName( ++nPos ) ) == cBagClean
               EXIT
            ENDIF
         ENDDO
         OrdSetFocus( nPos )
         cBagName := OrdBagName( nPos )
      ELSE /* ToDo: logic check for CDX, with more than one CDX file */
         OrdSetFocus( OrdCount() )
         cBagName := OrdBagName()
      ENDIF
   RECOVER USING oError
      lres := .F.
   END SEQUENCE

   IF !lres
      RETURN ErrorStr("-003:", oError, cBagName)
   ENDIF

   cReply := "+" + leto_IndexesInfo( nUserStru, cBagName )

   cRecData := leto_rec( nUserStru )

   IF LEN( cRecData ) <= 0
      RETURN "-003"
   ENDIF

   RETURN cReply + cRecData

FUNCTION hs_createtable( nUserStru, cCommand )
   LOCAL cName, cAlias, cFileName, cExt, i, nLen, cLen, nPos, aStru, cReply, nAreaID
   LOCAL oError, lres := .T. , nDriver, nTableStru, cRealAlias, nArea
   LOCAL lAnyExt := leto_GetAppOptions( 4 ), cDataPath
   LOCAL cRecData

   IF Empty( cName := GetCmdItem( cCommand,1,@nPos ) ) .OR. nPos == 0
      RETURN "-002"
   ENDIF
   cAlias := GetCmdItem( cCommand,nPos + 1,@nPos )
   IF nPos == 0
      RETURN "-002"
   ENDIF
   IF Empty( nLen := Val( GetCmdItem( cCommand,nPos + 1,@nPos ) ) )
      RETURN "-003"
   ENDIF

   IF !lAnyExt .AND. !Empty( cExt := hb_FNameExt( cName ) ) .AND. Lower( cExt ) != ".dbf"
      RETURN "-004"
   ENDIF

   aStru := Array( nLen, 4 )
   FOR i := 1 TO nLen
      aStru[i,1] := GetCmdItem( cCommand, nPos + 1, @nPos )
      aStru[i,2] := GetCmdItem( cCommand, nPos + 1, @nPos )
      aStru[i,3] := Val( GetCmdItem( cCommand,nPos + 1,@nPos ) )
      aStru[i,4] := Val( GetCmdItem( cCommand,nPos + 1,@nPos ) )
   NEXT

   cName := StrTran( cName, DEF_CH_SEP, DEF_SEP )
   IF Empty( hb_FNameExt( cName ) )
      cName += ".dbf"
   ENDIF

   IF ( nDriver := leto_getDriver( hb_FNameDir(cName) ) ) == Nil
      nDriver := leto_GetAppOptions( LETO_CDX )
   ENDIF

   nAreaID := leto_CreateAreaID()
   cRealAlias := leto_MakeAlias( nAreaID )

   IF Lower(cName) = "/mem:"
      cDataPath := ""
      cName := Substr(cName, 2)
   ELSE
      cDataPath := leto_GetAppOptions( 1 )
   ENDIF
   cFileName := cDataPath + cName
   BEGIN SEQUENCE WITH { |e|break( e ) }
      leto_SetUserEnv( nUserStru )
      dbCreate( cFileName, aStru, leto_Driver( nDriver ), .T. , cRealAlias )
   RECOVER USING oError
      lres := .F.
   END SEQUENCE

   IF !lres
      leto_DelAreaID( nAreaID )
      RETURN ErrorStr("-011:", oError, cFileName)
   ENDIF

   IF ( nTableStru := leto_InitTable( nAreaID, cName, nDriver, .F. ) ) < 0
      RETURN "-004"
   ENDIF

   IF leto_InitArea( nUserStru, nTableStru, nAreaID, cAlias, .F. ) < 0
      RETURN "-004"
   ENDIF

   cReply := "+" + LTrim( Str( nAreaID ) ) + ";" + ;
      iif( nDriver == LETO_NTX, "1;", "0;" )

   cRecData := leto_rec( nUserStru )

   IF LEN( cRecData ) <= 0
      RETURN "-004"
   ENDIF

   cReply += cRecData
   cReply += letoMemoInfo()
   cLen := leto_N2B( Len( cReply ) )

   RETURN Chr( Len( cLen ) ) + cLen + cReply

FUNCTION hs_createindex( nUserStru, cCommand )
   LOCAL nPos
   LOCAL cBagName, cTagName, cKey, lUnique, cFor, cWhile
   LOCAL oError, lres := .T. , nIndexStru
   LOCAL cTemp
   LOCAL lAll, nRecno, nNext, nRecord, lRest, lDescend, lCustom, lAdditive
   LOCAL lMemory, lFilter
   LOCAL lAnyExt := leto_GetAppOptions( 4 ), cDataPath
   LOCAL lUseCur, cOrder

   cBagName := GetCmdItem( cCommand, 1, @nPos )
   cTagName := GetCmdItem( cCommand, nPos + 1, @nPos )
   IF Empty( cBagName ) .AND. Empty( cTagName )
      RETURN "-002"
   ENDIF
   IF Empty( cKey := GetCmdItem( cCommand,nPos + 1,@nPos ) )
      RETURN "-002"
   ENDIF
   lUnique := ( GetCmdItem( cCommand,nPos + 1,@nPos ) == "T" )
   cFor := GetCmdItem( cCommand, nPos + 1, @nPos )
   cWhile := GetCmdItem( cCommand, nPos + 1, @nPos )

   IF !Empty( cTemp := GetCmdItem( cCommand,nPos + 1,@nPos ) )
      lAll := ( cTemp == "T" )
   ENDIF
   IF !Empty( cTemp := GetCmdItem( cCommand,nPos + 1,@nPos ) )
      nRecno := Val( cTemp )
   ENDIF
   IF !Empty( cTemp := GetCmdItem( cCommand,nPos + 1,@nPos ) )
      nNext := Val( cTemp )
   ENDIF
   IF !Empty( cTemp := GetCmdItem( cCommand,nPos + 1,@nPos ) )
      nRecord := Val( cTemp )
   ENDIF
   IF !Empty( cTemp := GetCmdItem( cCommand,nPos + 1,@nPos ) )
      lRest := ( cTemp == "T" )
   ENDIF
   IF !Empty( cTemp := GetCmdItem( cCommand,nPos + 1,@nPos ) )
      lDescend := ( cTemp == "T" )
   ENDIF
   IF !Empty( cTemp := GetCmdItem( cCommand,nPos + 1,@nPos ) )
      lCustom := ( cTemp == "T" )
   ENDIF
   IF nPos != 0 .AND. !Empty( cTemp := GetCmdItem( cCommand,nPos + 1,@nPos ) )
      lAdditive := ( cTemp == "T" )
   ENDIF
   IF nPos != 0 .AND. !Empty( cTemp := GetCmdItem( cCommand,nPos + 1,@nPos ) )
      lMemory := ( cTemp == "T" )
   ENDIF
   IF nPos != 0 .AND. !Empty( cTemp := GetCmdItem( cCommand,nPos + 1,@nPos ) )
      lFilter := ( cTemp == "T" )
   ENDIF
   IF nPos != 0 .AND. !Empty( cOrder := GetCmdItem( cCommand,nPos + 1,@nPos ) )
      OrdSetFocus( cOrder )
      lUseCur := .T.
      leto_SetAreaEnv( nUserStru, cOrder )
   ENDIF

   IF !lAnyExt .AND. !Empty( cBagName ) .AND. ;
         !Empty( cTemp := hb_FNameExt( cBagName ) ) .AND. ;
         ( Len( cTemp ) < 4 .OR. !( Lower( Substr( cTemp, 2 ) ) $ "cdx;idx;ntx" ) )
      RETURN "-004"
   ENDIF

   IF !Empty( cBagName )
      IF Empty( cTagName ) .AND. !( Lower(IndexExt())==".ntx" )
         cTagName := hb_FNameName( cBagName )
      ENDIF
      cBagName := StrTran( cBagName, DEF_CH_SEP, DEF_SEP )
      IF Lower(cBagName) = "/mem:"
         cDataPath := ""
         cBagName := Substr(cBagName, 2)
      ELSE
         cDataPath := leto_GetAppOptions( 1 )
      ENDIF
      IF DEF_SEP $ cBagName
         IF ! Empty( cDataPath ) .AND. cBagName <> DEF_SEP
            cBagName := DEF_SEP + cBagName
         ENDIF
         cBagName := cDataPath + iif( Left( cBagName,1 ) $ DEF_SEP, "", DEF_SEP ) + cBagName
      ELSE
         cBagName := cDataPath + hb_FNameDir( leto_TableName( nUserStru ) ) + cBagName
      ENDIF
   ENDIF

   BEGIN SEQUENCE WITH { |e|break( e ) }
      IF ! Empty( cFor ) .OR. ! Empty( cWhile ) .OR. lDescend != Nil .OR. ;
            lAll != Nil .OR. nRecno != Nil .OR. nNext != Nil .OR. nRecord != Nil .OR. ;
            lRest != Nil .OR. lDescend != Nil .OR. lAdditive != Nil .OR. ;
            lCustom != Nil .OR. lMemory != Nil .OR. lFilter != Nil .OR. lUseCur != Nil
         ordCondSet( ;
            iif( !Empty( cFor ), cFor, ), ;
            iif( !Empty( cFor ), &( "{||" + cFor + "}" ), ), ;
            iif( lAll != Nil, lAll, ), ;
            iif( !Empty( cWhile ), &( "{||" + cWhile + "}" ), ), , , ;
            iif( !Empty( nRecno ), nRecno, ), ;
            iif( !Empty( nNext ), nNext, ), ;
            iif( !Empty( nRecord ), nRecord, ), ;
            iif( lRest != Nil, lRest, ), ;
            iif( lDescend != Nil, lDescend, ), , ;
            iif( lAdditive != Nil, lAdditive, ), ;
            iif( lUseCur != Nil, lUseCur, ), ;
            iif( lCustom != Nil, lCustom, ), , ;
            iif( ! Empty( cWhile ), cWhile, ), ;
            iif( lMemory != Nil, lMemory, ), ;
            iif( lFilter != Nil, lFilter, ), )
      ENDIF
      leto_SetUserEnv( nUserStru )
      OrdCreate( cBagName, cTagName, cKey, &( "{||" + cKey + "}" ), lUnique )
   RECOVER USING oError
      lres := .F.
   END SEQUENCE

   IF ( lFilter != Nil .AND. lFilter ) .OR. ( lUseCur != Nil .AND. lUseCur )
      leto_ClearAreaEnv( nUserStru, cOrder )
   ENDIF

   IF !lres
      RETURN ErrorStr("-003:", oError, cBagName)
   ENDIF

   nIndexStru := leto_InitIndex( nUserStru, OrdBagName( OrdCount() ), cBagName )
   IF !Empty( cTagName )
      leto_addTag( nUserStru, nIndexStru, cTagName )
   ENDIF

   RETURN "++;" + leto_rec( nUserStru )

#define EF_CANRETRY                     1
#define EF_CANSUBSTITUTE                2
#define EF_CANDEFAULT                   4

STATIC FUNCTION ErrorStr(cSign, oError, cFileName)
   LOCAL nFlags := 0
   IF oError:canDefault
      nFlags += EF_CANDEFAULT
   ENDIF
   IF oError:canRetry
      nFlags += EF_CANRETRY
   ENDIF
   IF oError:canSubstitute
      nFlags += EF_CANSUBSTITUTE
   ENDIF

   WrLog( Leto_ErrorMessage( oError ) )

   RETURN cSign ;
          + Iif( ISNUMBER(oError:genCode), LTrim( Str( oError:genCode ) ), "0" ) ;
          + "-" + Iif( ISNUMBER(oError:subCode), LTrim(Str(oError:subCode)), "0" ) ;
          + "-" + Iif( ISNUMBER(oError:osCode), LTrim( Str( oError:osCode ) ), "0" ) ;
          + "-" + LTrim( Str( nFlags ) ) ;
          + Chr(9) + cFileName

EXIT PROCEDURE EXITP

   LOCAL pExit

   IF !Empty( pHrb ) .AND. !Empty( pExit := hb_hrbGetFunSym(pHrb, 'UDF_Exit') )
      hb_ExecFromArray( pExit )
   ENDIF

   leto_ReleaseData()

   RETURN

CLASS HApp

   DATA nPort     INIT 2812
   DATA ip
   DATA nTimeOut  INIT -1
   DATA DataPath  INIT ""
   DATA LogFile   INIT ""
   DATA lLower    INIT .F.
   DATA lFileFunc INIT .F.
   DATA lAnyExt   INIT .F.
   DATA lShare    INIT .F.      // .T. - new mode, which allows share tables with other processes
   DATA lNoSaveWA INIT .F.      // .T. - new mode, which forces dbUseArea() each time "open table" is demanded
   DATA nDriver   INIT 0
   DATA lPass4M   INIT .F.
   DATA lPass4L   INIT .F.
   DATA lPass4D   INIT .F.
   DATA cPassName INIT "leto_users"
   DATA lCryptTraffic INIT .F.
   DATA cTrigger
   DATA cPendingTrigger

   METHOD New()

ENDCLASS

METHOD New() CLASS HApp
   LOCAL cIniName := "letodb.ini"
   LOCAL aIni, i, j, cTemp, cPath, nDriver
   LOCAL nPort
   LOCAL nMaxVars, nMaxVarSize
   LOCAL nCacheRecords := 10
   LOCAL nTables_max := NIL
   LOCAL nUsers_max := NIL
   LOCAL nDebugMode := 0
   LOCAL lOptimize := .F.
   LOCAL nAutOrder
   LOCAL nMemoType
   LOCAL lForceOpt := .F.
   LOCAL lSetTrigger := .F.
   LOCAL nUDF := 1

#ifdef __LINUX__

   IF File( cDirBase + cIniName )
      aIni := rdIni( cDirBase + cIniName )
   ELSEIF File( "/etc/" + cIniName )
      aIni := rdIni( "/etc/" + cIniName )
   ENDIF

#else

   IF File( cDirBase + cIniName )
      aIni := rdIni( cDirBase + cIniName )
   ENDIF

#endif

   IF !Empty( aIni )
      FOR i := 1 TO Len( aIni )
         IF aIni[i,1] == "MAIN"
            FOR j := 1 TO Len( aIni[i,2] )
               IF aIni[i,2,j,1] == "PORT"
                  IF ( nPort := Val( aIni[i,2,j,2] ) ) >= 2000
                     ::nPort := nPort
                  ENDIF
               ELSEIF aIni[i,2,j,1] == "IP"
                  ::ip := aIni[i,2,j,2]
               ELSEIF aIni[i,2,j,1] == "TIMEOUT"
                  ::nTimeOut := Val( aIni[i,2,j,2] )
               ELSEIF aIni[i,2,j,1] == "DATAPATH"
                  ::DataPath := StrTran( aIni[i,2,j,2], DEF_CH_SEP, DEF_SEP )
                  IF Right( ::DataPath, 1 ) $ DEF_SEP
                     ::DataPath := Left( ::DataPath, Len( ::DataPath ) - 1 )
                  ENDIF
               ELSEIF aIni[i,2,j,1] == "LOGPATH"
                  ::LogFile := StrTran( aIni[i,2,j,2], DEF_CH_SEP, DEF_SEP )
                  IF !EMPTY( ::LogFile )
                     IF Right( ::LogFile, 1 ) != DEF_SEP
                        ::LogFile += DEF_SEP
                     ENDIF
                     leto_setDirBase( ::LogFile )
                  ENDIF
               ELSEIF aIni[i,2,j,1] == "LOWER_PATH"
                  ::lLower := ( aIni[i,2,j,2] == '1' )
               ELSEIF aIni[i,2,j,1] == "ENABLEFILEFUNC"
                  ::lFileFunc := ( aIni[i,2,j,2] == '1' )
               ELSEIF aIni[i,2,j,1] == "ENABLEANYEXT"
                  ::lAnyExt := ( aIni[i,2,j,2] == '1' )
               ELSEIF aIni[i,2,j,1] == "ENABLEUDF" .and. Val(aIni[i,2,j,2]) >= 0 .and. Val(aIni[i,2,j,2]) <= 2
                  nUDF := Val( aIni[i,2,j,2] )
               ELSEIF aIni[i,2,j,1] == "SHARE_TABLES"
                  ::lShare := ( aIni[i,2,j,2] == '1' )
               ELSEIF aIni[i,2,j,1] == "NO_SAVE_WA"
                  ::lNoSaveWA := ( aIni[i,2,j,2] == '1' )
               ELSEIF aIni[i,2,j,1] == "DEFAULT_DRIVER"
                  ::nDriver := iif( Lower( aIni[i,2,j,2] ) == "ntx", LETO_NTX, 0 )
               ELSEIF aIni[i,2,j,1] == "PASS_FOR_LOGIN"
                  ::lPass4L := ( aIni[i,2,j,2] == '1' )
               ELSEIF aIni[i,2,j,1] == "PASS_FOR_MANAGE"
                  ::lPass4M := ( aIni[i,2,j,2] == '1' )
               ELSEIF aIni[i,2,j,1] == "PASS_FOR_DATA"
                  ::lPass4D := ( aIni[i,2,j,2] == '1' )
               ELSEIF aIni[i,2,j,1] == "PASS_FILE"
                  ::cPassName := aIni[i,2,j,2]
               ELSEIF aIni[i,2,j,1] == "CRYPT_TRAFFIC"
                  ::lCryptTraffic := ( aIni[i,2,j,2] == '1' )
               ELSEIF aIni[i,2,j,1] == "MAX_VARS_NUMBER"
                  nMaxVars := Val( aIni[i,2,j,2] )
               ELSEIF aIni[i,2,j,1] == "MAX_VAR_SIZE"
                  nMaxVarSize := Val( aIni[i,2,j,2] )
               ELSEIF aIni[i,2,j,1] == "CACHE_RECORDS"
                  IF ( nCacheRecords := Val( aIni[i,2,j,2] ) ) <= 0
                      nCacheRecords := 10
                  ENDIF
               ELSEIF aIni[i,2,j,1] == "TABLES_MAX"
                  IF ( nTables_max := Val( aIni[i,2,j,2] ) ) <= 100 .OR. nTables_max > 200000
                      nTables_max := NIL
                  ENDIF
               ELSEIF aIni[i,2,j,1] == "USERS_MAX"
                  IF ( nUsers_max := Val( aIni[i,2,j,2] ) ) <= 10 .OR. nUsers_max > 100000
                      nUsers_max := NIL
                  ENDIF
               ELSEIF aIni[i,2,j,1] == "DEBUG"
                  IF ( nDebugMode := Val( aIni[i,2,j,2] ) ) <= 0
                      nDebugMode := 0
                  ENDIF
               ELSEIF aIni[i,2,j,1] == "OPTIMIZE"
                  lOptimize := ( aIni[i,2,j,2] == '1' )
               ELSEIF aIni[i,2,j,1] == "AUTORDER"
                  nAutOrder := Val( aIni[i,2,j,2] )
               ELSEIF aIni[i,2,j,1] == "MEMO_TYPE"
                  IF Lower( aIni[i,2,j,2] ) = 'dbt'
                     nMemoType := DB_MEMO_DBT
                  ELSEIF Lower( aIni[i,2,j,2] ) = 'fpt'
                     nMemoType := DB_MEMO_FPT
                  ELSEIF Lower( aIni[i,2,j,2] ) = 'smt'
                     nMemoType := DB_MEMO_SMT
                  ENDIF
               ELSEIF aIni[i,2,j,1] == "FORCEOPT"
                  lForceOpt := ( aIni[i,2,j,2] == '1' )
               ELSEIF aIni[i,2,j,1] == "ENABLESETTRIGGER"
                  lSetTrigger := ( aIni[i,2,j,2] == '1' )
               ELSEIF aIni[i,2,j,1] == "TRIGGER"
                  ::cTrigger := aIni[i,2,j,2]
               ELSEIF aIni[i,2,j,1] == "PENDINGTRIGGER"
                  ::cPendingTrigger := aIni[i,2,j,2]
               ENDIF
            NEXT
         ELSEIF aIni[i,1] == "DATABASE"
            cPath := nDriver := Nil
            FOR j := 1 TO Len( aIni[i,2] )
               IF aIni[i,2,j,1] == "DATAPATH"
                  cPath := StrTran( aIni[i,2,j,2], DEF_CH_SEP, DEF_SEP )
                  IF Right( cPath, 1 ) $ DEF_SEP
                     cPath := Left( cPath, Len( cPath ) - 1 )
                  ENDIF
               ELSEIF aIni[i,2,j,1] == "DRIVER"
                  nDriver := iif( ( cTemp := Lower( aIni[i,2,j,2] ) ) == "cdx", ;
                     0, iif( cTemp == "ntx", LETO_NTX, Nil ) )
               ENDIF
            NEXT
            IF cPath != Nil
               cPath := StrTran( cPath, DEF_CH_SEP, DEF_SEP )
               IF Left( cPath,1 ) != DEF_SEP
                  cPath := DEF_SEP + cPath
               ENDIF
               IF Right( cPath,1 ) != DEF_SEP
                  cPath += DEF_SEP
               ENDIF
               leto_AddDataBase( cPath, iif( nDriver == Nil,::nDriver,nDriver ) )
            ENDIF
         ENDIF
      NEXT
   ENDIF

   IF ::lLower
      SET( _SET_FILECASE, 1 )
      SET( _SET_DIRCASE, 1 )
   ENDIF
   IF ::lNoSaveWA
      ::lShare := .T.
   ENDIF

   leto_SetAppOptions( iif( Empty(::DataPath ),Nil,::DataPath ), ::nDriver, ::lFileFunc, ;
         ::lAnyExt, ::lPass4L, ::lPass4M, ::lPass4D, ::cPassName, ::lCryptTraffic, ;
         ::lShare, ::lNoSaveWA, nMaxVars, nMaxVarSize, nCacheRecords, nTables_max, nUsers_max, ;
         nDebugMode, lOptimize, nAutOrder, nMemoType, lForceOpt, ::cTrigger, ::cPendingTrigger, lSetTrigger, nUDF )

   RETURN Self


FUNCTION leto_SetEnv( xScope, xScopeBottom, xOrder, cFilter, lDeleted )
   IF ! Empty( xOrder )
      OrdSetFocus( xOrder )
   ENDIF
   IF ValType(cFilter) == "C"
      dbSetFilter( &( "{||" + cFilter + "}" ), cFilter )
   ENDIF
   IF lDeleted != Nil
      Set( _SET_DELETED, lDeleted )
   ENDIF

   IF xScope != Nil
      dbOrderInfo(DBOI_SCOPETOP,,, xScope)
      dbOrderInfo(DBOI_SCOPEBOTTOM,,, iif( xScopeBottom == Nil, xScope, xScopeBottom ) )
   ENDIF
   RETURN Nil

FUNCTION leto_ClearEnv( xScope, xScopeBottom, cFilter )
   IF xScope != Nil
      dbOrderInfo(DBOI_SCOPETOPCLEAR)
      dbOrderInfo(DBOI_SCOPEBOTTOMCLEAR)
   ENDIF
   IF ValType(cFilter) == "C"
      dbClearFilter()
   ENDIF
   RETURN Nil
