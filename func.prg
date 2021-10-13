#include "hbclass.ch"
#include 'hboo.ch'

// ----------------------------------------------------------------//

FUNCTION ObjToChar( o )

   LOCAL hObj := { => }, aDatas := __objGetMsgList( o, .T. )
   LOCAL hPairs := { => }, aParents := __clsGetAncestors( o:ClassH )

   AEval( aParents, {| h, n | aParents[ n ] := __className( h ) } )

   hObj[ "CLASS" ] = o:ClassName()
   hObj[ "FROM" ]  = aParents

   AEval( aDatas, {| cData | hPairs[ cData ] := __objSendMsg( o, cData ) } )
   hObj[ "DATAs" ]   = hPairs
   hObj[ "METHODs" ] = __objGetMsgList( o, .F. )

RETURN ValToChar( hObj )

// ----------------------------------------------------------------//

FUNCTION ValToChar( u )

   LOCAL cType := ValType( u )
   LOCAL cResult

   DO CASE
   CASE cType == "C" .OR. cType == "M"
      cResult = u

   CASE cType == "D"
      cResult = DToC( u )

   CASE cType == "L"
      cResult = If( u, ".T.", ".F." )

   CASE cType == "N"
      cResult = AllTrim( Str( u ) )

   CASE cType == "A"
      cResult = hb_ValToExp( u )

   CASE cType == "O"
      cResult = ObjToChar( u )

   CASE cType == "P"
      cResult = "(P)"

   CASE cType == "S"
      cResult = "(Symbol)"

   CASE cType == "H"
      cResult = hb_jsonEncode( u, .T. )
      IF Left( cResult, 2 ) == "{}"
         cResult = StrTran( cResult, "{}", "{=>}" )
      ENDIF

   CASE cType == "U"
      cResult = "nil"

   OTHERWISE
      cResult = "type not supported yet in function ValToChar()"
   ENDCASE

RETURN cResult



#pragma BEGINDUMP

#include <hbapi.h>
#include <hbapiitm.h>
#include <hbapierr.h>

HB_FUNC( PTRTOSTR )
{
   #ifdef HB_ARCH_32BIT
      const char * * pStrs = ( const char * * ) hb_parnl( 1 );
   #else
      const char * * pStrs = ( const char * * ) hb_parnll( 1 );
   #endif

   hb_retc( * ( pStrs + hb_parnl( 2 ) ) );
}

//----------------------------------------------------------------//

HB_FUNC( PTRTOUI )
{
   #ifdef HB_ARCH_32BIT
      unsigned int * pNums = ( unsigned int * ) hb_parnl( 1 );
   #else
      unsigned int * pNums = ( unsigned int * ) hb_parnll( 1 );
   #endif

   hb_retnl( * ( pNums + hb_parnl( 2 ) ) );
}


#pragma ENDDUMP
