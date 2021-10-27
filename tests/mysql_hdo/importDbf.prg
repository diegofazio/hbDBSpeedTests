//-----------------------------------------------------------------------------

REQUEST RDLMYSQLN   // Importante: solicitud para enlazar la RDL usada
REQUEST DBFCDX

//------------------------------------------------------------------------------

#include "hdo.ch" // Fichero include de hdo

//------------------------------------------------------------------------------

FUNCTION Main( cServer )

   LOCAL o, cSql, a, n, oRS
   local cValues, cHeader

   cServer := if( empty( cServer  ), "localhost", AllTrim( cServer ) )

   cls
   ? "Importacion desde una dbf, usando variables unidas (bind)"
   ?

   ? "Connecting DBF..."
   a =  hb_MilliSeconds()

   dbSelectArea( 1 )
   dbUseArea( .F., "DBFCDX", '.\data\dbf\db.dbf',, .T. )

   ?? 'OK ', hb_MilliSeconds() - a, 'ms'

   o := THDO():new( HDO_RDL_MYSQL_NATIVE )

   ? "Connecting MySQL..."
   a =  hb_MilliSeconds()

   IF !o:connect( "harbourdb", cServer, "harbour", "", 3306 )
      o:errorStr()
      o:free()
      QUIT
   ENDIF

   ?? 'OK ', hb_MilliSeconds() - a, 'ms'

   ? "Importing DBF..."
   a =  hb_MilliSeconds()

   cHeader := 'INSERT INTO db (KAR_RUBRO,KAR_FECHA,KAR_CLIE,KAR_TIPO,KAR_NUMERO,'
   cHeader += 'KAR_DEPO,KAR_CANT,KAR_PRECIO,KAR_DESCTO,KAR_VENDED,KAR_PIEZAS,'
   cHeader += 'KAR_ENTSAL,KAR_ARTIC ) VALUES '

   DO WHILE !Eof()

      cValues := ''
      n := 0

     WHILE n < 1000 .AND. !Eof()
         cValues += '('
         cValues += '"' + AllTrim( valtochar( Field->KAR_RUBRO ) ) + '",'
         cValues += '"' + AllTrim( valtochar( hb_DToC( Field->KAR_FECHA, 'YYYY-MM-DD' ) ) )  + '",'
         cValues += '"' + AllTrim( valtochar( Field->KAR_CLIE ) )   + '",'
         cValues += '"' + AllTrim( valtochar( Field->KAR_TIPO ) )   + '",'
         cValues += '"' + AllTrim( valtochar( Field->KAR_NUMERO ) ) + '",'
         cValues += '"' + AllTrim( valtochar( Field->KAR_DEPO ) )   + '",'
         cValues += '"' + AllTrim( valtochar( Field->KAR_CANT ) )   + '",'
         cValues += '"' + AllTrim( valtochar( Field->KAR_PRECIO ) ) + '",'
         cValues += '"' + AllTrim( valtochar( Field->KAR_DESCTO ) ) + '",'
         cValues += '"' + AllTrim( valtochar( Field->KAR_VENDED ) ) + '",'
         cValues += '"' + AllTrim( valtochar( Field->KAR_PIEZAS ) ) + '",'
         cValues += '"' + AllTrim( valtochar( Field->KAR_ENTSAL ) ) + '",'
         cValues += '"' + AllTrim( valtochar( Field->KAR_ARTIC )  ) + '" '
         cValues += '),'
         n++
         dbSkip()
      END

      cValues := SubStr( cValues, 1, Len( cValues ) - 1 )

      cSql := cHeader + cValues

      o:execDirect( cSql )

   END DO

   ?? 'OK '
   ? "Total Importing time:" , hb_MilliSeconds() - a, "ms"

   ? "Counting records with 'SELECT * FROM db' "
   a =  hb_MilliSeconds()
   oRS := o:rowSet( "SELECT * FROM db" )
   oRS:load()

   ? 'Count(): ', oRS:rowCount()
   ? "Total Counting time:" , hb_MilliSeconds() - a, "ms"

   oRS:free()
   o:free()

RETURN NIL