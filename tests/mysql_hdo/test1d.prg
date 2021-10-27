//-----------------------------------------------------------------------------

REQUEST RDLMYSQLN   // Importante: solicitud para enlazar la RDL usada

//------------------------------------------------------------------------------

#include "hdo.ch" // Fichero include de hdo

//------------------------------------------------------------------------------

FUNCTION Main( cServer )

   LOCAL o, a, aData

   cServer := if( empty( cServer  ), "localhost", AllTrim( cServer ) )

   cls
   ? "Ejecutando Test1 HDO"
   ?

   o := THDO():new( HDO_RDL_MYSQL_NATIVE )

   ? "Connecting MySQL..."
   a =  hb_MilliSeconds()
     
   IF !o:connect( "harbourdb", cServer, "harbour", "", 3306 )
      ? o:errorStr()
      o:free()
      QUIT
   ENDIF

   ?? 'OK ', hb_MilliSeconds() - a, 'ms' 

   ? "Getting data..."
   a =  hb_MilliSeconds()
   aData := o:queryDirect( "SELECT COUNT(*) FROM db WHERE kar_tipo = 1" )

   ?? 'OK'
   ? "Total time:" , hb_MilliSeconds() - a, "ms"
   ? aData[ 1 ][ 1 ]

   o:free()

RETURN NIL
