/*	---------------------------------------------------------
	File.......: wdo.prg
	Description: Base WDO. Conexión a Bases de Datos. 
	Author.....: Carles Aubia Floresvi
	Date:......: 26/07/2019
	--------------------------------------------------------- */
	
CLASS RDBMS	

	DATA  cType
	
	DATA  cRdbms
	DATA  cServer
	DATA  cUsername
	DATA  cDatabase
	DATA  cPassword
	DATA  nPort
	DATA  hConnection 	
	
	DATA  cRdd		
	DATA  cDbf		
	DATA  cIndex	

	DATA  lShowError						INIT .T.	

	METHOD New() 						CONSTRUCTOR
	METHOD View( aSt, aData )

ENDCLASS

METHOD New() CLASS RDBMS

	? 'RDBMS New()'

RETU SELF


METHOD View( aSt, aData ) CLASS RDBMS

	LOCAL nFields 	:= len( aSt )
	LOCAL cHtml 	:= ''
	LOCAL n, j, nLen
	
	cHtml := '<h3>View table</h3>'	
	
	cHtml += '<style>'
	cHtml += '#mytable tr:hover {background-color: #ddd;}'
	cHtml += '#mytable tr:nth-child(even){background-color: #e0e6ff;}'
	cHtml += '#mytable { font-family: "Trebuchet MS", Arial, Helvetica, sans-serif;border-collapse: collapse; width: 100%; }'
	cHtml += '#mytable thead { background-color: #425ecf;color: white;}'
	cHtml += '</style>'
	cHtml += '<table id="mytable" border="1" cellpadding="3" >'
	
	cHtml += '<thead><tr>'
	
	FOR n := 1 TO nFields
	
		cHtml += '<td>' + aSt[n][1] + '</td>'
		
	NEXT
	
	cHtml += '</tr></thead>'
	
	nLen := len( aData )
	
	cHtml += '<tbody>'
	
	? cHtml 
	
	FOR n := 1 to nLen 
	
		cHtml := '<tr>'
		
		FOR j := 1 to nFields

			cHtml += '<td>' + valtochar( aData[n][j] ) + '</td>'
		
		NEXT
		
		cHtml += '</tr>'
		
		?? cHtml
	
	NEXT
	
	?? '</tbody></table><hr>'

RETU NIL