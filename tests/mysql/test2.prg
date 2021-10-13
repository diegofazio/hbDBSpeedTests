FUNCTION Main()

   LOCAL o, a, hRes := { => }

   hb_SetEnv( 'WDO_PATH_MYSQL', "c:/xampp/apache/bin/" )

   ? "Connecting MySQL..."

   o := WDO():Rdbms( 'MYSQL', "localhost", "harbour", "", "harbourdb", 3306 )

   IF !o:lConnect

      ? o:cError
      QUIT

   ENDIF

   ?? 'OK'

   ? "Getting data..."
   a =  hb_MilliSeconds()
   hRes := o:Query( "select * from db where KAR_FECHA >= '2004-01-01' and KAR_FECHA <= '2004-12-31'" )
   aData := o:FetchAll( hRes )

   ?? 'OK'
   ? "Total time:", hb_MilliSeconds() - a, "ms"
   ? "Result: ", Len( aData )
   ? "Result: ", o:Count( hRes )

RETURN NIL
