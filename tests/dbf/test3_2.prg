REQUEST DBFCDX

FUNCTION Main()

   LOCAL cPathData := 'c:\bases\dbf\'
   LOCAL nCount := 0
   LOCAL dDesde := 0d20040101
   LOCAL dHasta := 0d20041231
   LOCAL hRes := { => }
   LOCAL recordset := {}

   ? "Connecting DBF..."


   dbSelectArea( 1 )
   dbUseArea( .F., "DBFCDX", cPathData + 'db.dbf',, .T. )
   ?? 'OK'
   ? "Getting data..."
   a =  hb_MilliSeconds()

   LOCATE FOR KAR_NUMERO = '31415926'

   IF Found()
      hRes := { => }
      hRes[ 'KAR_RUBRO' ] := KAR_RUBRO
      hRes[ 'KAR_FECHA' ] := KAR_FECHA
      hRes[ 'KAR_CLIE'  ] := KAR_CLIE
      hRes[ 'KAR_TIPO'  ] := KAR_TIPO
      hRes[ 'KAR_NUMERO' ] := KAR_NUMERO
      hRes[ 'KAR_DEPO'  ] := KAR_DEPO
      hRes[ 'KAR_CANT'  ] := KAR_CANT
      hRes[ 'KAR_PRECIO' ] := KAR_PRECIO
      hRes[ 'KAR_DESCTO' ] := KAR_DESCTO
      hRes[ 'KAR_VENDED' ] := KAR_VENDED
      hRes[ 'KAR_PIEZAS' ] := KAR_PIEZAS
      hRes[ 'KAR_ENTSAL' ] := KAR_ENTSAL
      hRes[ 'KAR_ARTIC' ] := KAR_ARTIC

      ?? 'OK'

   ELSE

      ?? 'NOT FOUND'

   END IF

   ? "Total time:", hb_MilliSeconds() - a, "ms"
   ? "Result: ", KAR_NUMERO

RETURN NIL
