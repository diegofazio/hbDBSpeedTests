//-----------------------------------------------------------------------------

REQUEST RDLMYSQLN   // Importante: solicitud para enlazar la RDL usada

//------------------------------------------------------------------------------

#include "hdo.ch" // Fichero include de hdo

//------------------------------------------------------------------------------

FUNCTION Main()

   LOCAL o, a, aData

   cls
   ? "Ejecutando Test1"
   ?

   o := THDO():new( HDO_RDL_MYSQL_NATIVE )

   ? "Connecting MySQL..."
   a =  hb_MilliSeconds()
     
   IF !o:connect( "harbourdb", "localhost", "harbour", "" )
      ? o:cError
      o:free()
      QUIT
   ENDIF

   ?? 'OK ', hb_MilliSeconds() - a, 'ms' 

   ? "Getting data..."
   a =  hb_MilliSeconds()
   aData := o:queryDirect( "select count(*) from db where KAR_TIPO = 1" )

   ?? 'OK'
   ? "Total time:" , hb_MilliSeconds() - a, "ms"
   ? aData[ 1 ][ 1 ]

   inkey( 100 )

RETURN NIL
