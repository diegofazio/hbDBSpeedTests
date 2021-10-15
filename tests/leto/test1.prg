FUNCTION Main()

   LOCAL cIp := "127.0.0.1"
   LOCAL cPort := "2812"
   LOCAL cPathData := '//' + cIp + ":" + cPort + "/"
   LOCAL nCount := 0
   LOCAL a

   ? "Connecting DBF..."
   a =  hb_MilliSeconds()
   
   IF ( leto_Connect( "//" + cIp + ":" + cPort + "/" ) ) == -1
      ?? "Server not found"
      QUIT
   ENDIF

   dbSelectArea( 1 )
   dbUseArea( .F., "LETO", cPathData + 'dbf/db.dbf',, .T. )

   ?? 'OK ', hb_MilliSeconds() - a, 'ms' 
   ? "Getting data..."
   a =  hb_MilliSeconds()

   index on KAR_TIPO to kar for KAR_TIPO == 1

   ?? 'OK'
   ? "Total time:", hb_MilliSeconds() - a, "ms"
   ? "Result: ", ordKeyCount()

RETURN NIL
