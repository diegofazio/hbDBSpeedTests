//-----------------------------------------------------------------------------

REQUEST RDLMYSQLN   // Importante: solicitud para enlazar la RDL usada

//------------------------------------------------------------------------------

#include "hdo.ch" // Fichero include de hdo

//------------------------------------------------------------------------------

FUNCTION Main( cServer )

   LOCAL o, a, aData

   cServer := if( empty( cServer  ), "localhost", AllTrim( cServer ) )

   cls
   ? "Ejecutando Test3 HDO"
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
   aData := o:queryDirect( "SELECT * FROM db WHERE kar_numero = '31415926'" ) //, FETCH_HASH )

   ?? 'OK'
   ? "Total time:", hb_MilliSeconds() - a, "ms"
   ? "Result: ", aData[1][5]

   o:free()

RETURN NIL
