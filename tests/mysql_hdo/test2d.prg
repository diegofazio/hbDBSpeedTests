//-----------------------------------------------------------------------------

REQUEST RDLMYSQLN   // Importante: solicitud para enlazar la RDL usada

//------------------------------------------------------------------------------

#include "hdo.ch" // Fichero include de hdo

//------------------------------------------------------------------------------

FUNCTION Main( cServer )

   LOCAL o, a, aData

   cServer := if( empty( cServer  ), "localhost", AllTrim( cServer ) )

   cls
   ? "Ejecutando Test2 HDO"
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
   aData := o:queryDirect( "SELECT * FROM db WHERE kar_fecha BETWEEN '2004-01-01' AND '2004-12-31' ORDER BY kar_fecha ASC" )
   //ALTER TABLE `db` ADD INDEX `index1` (`KAR_FECHA`)
   //DROP INDEX `index1` ON db

//1000100004075
   ?? 'OK'
   ? "Total time:", hb_MilliSeconds() - a, "ms"
   ? "Result: ", Len( aData )

   o:free()
   
RETURN NIL
