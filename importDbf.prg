FUNCTION Main()

   LOCAL o, cSql

   hb_SetEnv( 'WDO_PATH_MYSQL', "c:/xampp/apache/bin/" )

   ? "Connecting DBF..."
   a =  hb_MilliSeconds()
   
   dbSelectArea( 1 )
   dbUseArea( .F., "DBFCDX", './data/dbf/db.dbf',, .T. )

   ?? 'OK ', hb_MilliSeconds() - a, 'ms' 
   
   ? "Connecting MySQL..."
   a =  hb_MilliSeconds()
   
   o := WDO():Rdbms( 'MYSQL', "localhost", "harbour", "", "harbourdb", 3306 )

   IF !o:lConnect

      ? o:cError
      QUIT

   ENDIF

   ?? 'OK ', hb_MilliSeconds() - a, 'ms' 
   
   dbGoTop()

   ? "Importing DBF..."
   a =  hb_MilliSeconds()
   
   DO WHILE !Eof()

      cSql := 'INSERT INTO db (KAR_RUBRO,KAR_FECHA,KAR_CLIE,KAR_TIPO,KAR_NUMERO,'
      cSql += 'KAR_DEPO,KAR_CANT,KAR_PRECIO,KAR_DESCTO,KAR_VENDED,KAR_PIEZAS,'
      cSql += 'KAR_ENTSAL,KAR_ARTIC ) VALUES ('
      cSql += '"' + AllTrim( valtochar( Field->KAR_RUBRO ) ) + '",'
      cSql += '"' + AllTrim( valtochar( hb_DtoC(Field->KAR_FECHA,'YYYY-MM-DD') ) )  + '",'
      cSql += '"' + AllTrim( valtochar( Field->KAR_CLIE ) )   + '",'
      cSql += '"' + AllTrim( valtochar( Field->KAR_TIPO ) )   + '",'
      cSql += '"' + AllTrim( valtochar( Field->KAR_NUMERO ) ) + '",'
      cSql += '"' + AllTrim( valtochar( Field->KAR_DEPO ) )   + '",'
      cSql += '"' + AllTrim( valtochar( Field->KAR_CANT ) )   + '",'
      cSql += '"' + AllTrim( valtochar( Field->KAR_PRECIO ) ) + '",'
      cSql += '"' + AllTrim( valtochar( Field->KAR_DESCTO ) ) + '",'
      cSql += '"' + AllTrim( valtochar( Field->KAR_VENDED ) ) + '",'
      cSql += '"' + AllTrim( valtochar( Field->KAR_PIEZAS ) ) + '",'
      cSql += '"' + AllTrim( valtochar( Field->KAR_ENTSAL ) ) + '",'
      cSql += '"' + AllTrim( valtochar( Field->KAR_ARTIC )  ) + '")'
      o:Query( cSql )

      dbSkip()

   END DO

   ?? 'OK '
   ? "Total Importing time:" , hb_MilliSeconds() - a, "ms"
 
   ? "Counting records with 'Select * from db' "
   a =  hb_MilliSeconds()
   
   IF !Empty( hRes := o:Query( "select * from db " ) )

      ? 'Count(): ', o:Count( hRes )
      ? "Total Counting time:" , hb_MilliSeconds() - a, "ms"

   ELSE

      ? "ERROR"

   ENDIF

RETURN NIL
