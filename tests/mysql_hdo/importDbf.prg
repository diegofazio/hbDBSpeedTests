//-----------------------------------------------------------------------------

REQUEST RDLMYSQLN   // Importante: solicitud para enlazar la RDL usada
REQUEST DBFCDX

//------------------------------------------------------------------------------

#include "hdo.ch" // Fichero include de hdo

//------------------------------------------------------------------------------

FUNCTION Main( cServer )

   LOCAL o, oStmt, cSql, a, oRS
   local KAR_RUBRO, KAR_FECHA, KAR_CLIE, KAR_TIPO, KAR_NUMERO, KAR_DEPO, KAR_CANT, KAR_PRECIO, KAR_DESCTO, KAR_VENDED, KAR_PIEZAS, KAR_ENTSAL, KAR_ARTIC

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
   
   dbGoTop()

   ? "Importing DBF..."
   
   cSql := 'INSERT INTO db (KAR_RUBRO,KAR_FECHA,KAR_CLIE,KAR_TIPO,KAR_NUMERO,'
   cSql += 'KAR_DEPO,KAR_CANT,KAR_PRECIO,KAR_DESCTO,KAR_VENDED,KAR_PIEZAS,'
   cSql += 'KAR_ENTSAL,KAR_ARTIC ) VALUES ( ?, ? ,?, ?, ?, ? ,?, ?, ?, ?, ?, ?, ? )'

   oStmt := o:prepare( cSql )
   
   oStmt:bindParam( 1, @KAR_RUBRO ) 
   oStmt:bindParam( 2, @KAR_FECHA )
   oStmt:bindParam( 3, @KAR_CLIE )
   oStmt:bindParam( 4, @KAR_TIPO )
   oStmt:bindParam( 5, @KAR_NUMERO ) 
   oStmt:bindParam( 6, @KAR_DEPO ) 
   oStmt:bindParam( 7, @KAR_CANT ) 
   oStmt:bindParam( 8, @KAR_PRECIO )
   oStmt:bindParam( 9, @KAR_DESCTO ) 
   oStmt:bindParam( 10, @KAR_VENDED ) 
   oStmt:bindParam( 11, @KAR_PIEZAS )
   oStmt:bindParam( 12, @KAR_ENTSAL ) 
   oStmt:bindParam( 13, @KAR_ARTIC )  

   a =  hb_MilliSeconds()

   DO WHILE !Eof()

      KAR_RUBRO := Field->KAR_RUBRO
      KAR_FECHA := Field->KAR_FECHA
      KAR_CLIE := Field->KAR_CLIE
      KAR_TIPO := Field->KAR_TIPO
      KAR_NUMERO := Field->KAR_NUMERO
      KAR_DEPO := Field->KAR_DEPO
      KAR_CANT := Field->KAR_CANT
      KAR_PRECIO := Field->KAR_PRECIO
      KAR_DESCTO := Field->KAR_DESCTO
      KAR_VENDED := Field->KAR_VENDED
      KAR_PIEZAS := Field->KAR_PIEZAS
      KAR_ENTSAL := Field->KAR_ENTSAL
      KAR_ARTIC := Field->KAR_ARTIC

      oStmt:execute()

      dbSkip()

   END DO
   
   ?? 'OK '
   ? "Total Importing time:" , hb_MilliSeconds() - a, "ms"
 
   oStmt:free()

   ? "Counting records with 'SELECT * FROM db' "
   a =  hb_MilliSeconds()
   oRS := o:rowSet( "SELECT * FROM db" )
   oRS:load()

   ? 'Count(): ', oRS:rowCount()
   ? "Total Counting time:" , hb_MilliSeconds() - a, "ms"

   oRS:free()
   o:free()

RETURN NIL