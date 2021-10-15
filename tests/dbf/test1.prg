REQUEST DBFCDX

FUNCTION Main()

   LOCAL cPathData := 'c:\bases\dbf\'
   LOCAL nCount := 0

   ? "Connecting DBF..."
   a =  hb_MilliSeconds()
   
   dbSelectArea( 1 )
   dbUseArea( .F., "DBFCDX", cPathData + 'db.dbf',, .T. )

   if neterr()
      ?? "Error open DBF"
   endif
   
   ?? 'OK ', hb_MilliSeconds() - a, 'ms' 
   ? "Getting data..."
   a =  hb_MilliSeconds()

   index on KAR_TIPO to MEM:kar for KAR_TIPO == 1

   ?? 'OK'
   ? "Total time:", hb_MilliSeconds() - a, "ms"
   ? "Result: ", ordKeyCount()

RETURN NIL
