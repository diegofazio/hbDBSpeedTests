FUNCTION Main()

   LOCAL o, a, hRes := { => }

   hb_SetEnv( 'WDO_PATH_MYSQL', "c:/xampp/apache/bin/" )

   ? "Connecting MySQL..."
   a =  hb_MilliSeconds()
   o := WDO():Rdbms( 'MYSQL', "localhost", "harbour", "", "harbourdb", 3306 )
   o:lWeb := .F.

   IF !o:lConnect

      ? o:cError
      QUIT

   ENDIF

   ?? 'OK ', hb_MilliSeconds() - a, 'ms' 

   ? "Getting data..."
   a =  hb_MilliSeconds()
   hRes := o:Query( "select * from db where KAR_FECHA BETWEEN '2004-01-01' and '2004-12-31' ORDER BY KAR_FECHA ASC" )
   aData := o:FetchAll( hRes )
   //ALTER TABLE `db` ADD INDEX `index1` (`KAR_FECHA`)
   //DROP INDEX `index1` ON db

//1000100004075
   ?? 'OK'
   ? "Total time:", hb_MilliSeconds() - a, "ms"
   ? "Result: ", Len( aData )
   ? "Result: ", o:Count( hRes )
   hb_memowrit("test.txt", valtochar( aData ))

RETURN NIL
