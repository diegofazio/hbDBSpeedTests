FUNCTION Main()

   LOCAL o, a, hRes := {=>}

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
   hRes := o:Query( "select count(*) from db where KAR_TIPO = 1 " )
   aData := o:FetchAll( hRes )

   ?? 'OK'
   ? "Total time:" , hb_MilliSeconds() - a, "ms"
   ? aData[1][1]

RETURN NIL
