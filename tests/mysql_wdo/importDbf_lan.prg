FUNCTION Main()

   LOCAL o, cSql

   hb_SetEnv( 'WDO_PATH_MYSQL', "c:/xampp/apache/bin/" )

   ? "Connecting DBF..."
   a =  hb_MilliSeconds()

   dbSelectArea( 1 )
   dbUseArea( .F., "DBFCDX", 'db.dbf',, .T. )

   ?? 'OK ', hb_MilliSeconds() - a, 'ms'

   ? "Connecting MySQL..."
   a =  hb_MilliSeconds()

   o := WDO():Rdbms( 'MYSQL', "192.168.0.9", "harbour", "", "harbourdb", 3306 )

   IF !o:lConnect

      ? o:cError
      QUIT

   ENDIF

   ?? 'OK ', hb_MilliSeconds() - a, 'ms'

   dbGoTop()

   ? "Importing DBF..."
   a =  hb_MilliSeconds()

   cHeader := 'INSERT INTO db2 (KAR_RUBRO,KAR_FECHA,KAR_CLIE,KAR_TIPO,KAR_NUMERO,'
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

      o:Query( cSql )

   END DO

   ?? 'OK '
   ? "Total Importing time:", hb_MilliSeconds() - a, "ms"

   ? "Counting records with 'Select * from db' "
   a =  hb_MilliSeconds()

   IF !Empty( hRes := o:Query( "select * from db " ) )

      ? 'Count(): ', o:Count( hRes )
      ? "Total Counting time:", hb_MilliSeconds() - a, "ms"

   ELSE

      ? "ERROR"

   ENDIF

RETURN NIL
