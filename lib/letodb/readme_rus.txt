/* $Id$ */

      Leto DB Server - �� ����������ଥ��� �ࢥ� ��, ��� ����, 
 � �᭮���� �।�����祭�� ��� ������᪨� �ணࠬ�, ����ᠭ��� �� �몥 Harbour,
 ��� ����㯠 � 䠩��� dbf/cdx, �ᯮ������� �� 㤠������ �ࢥ�.

����ঠ���
--------

1. ������� ��⠫����
2. ���ઠ letodb
   2.1 ��������� Borland Win32 C
   2.2 ��������� Linux GNU C
   2.3 xHarbour Builder
   2.4 ��������� Mingw C
   2.5 ���ઠ letodb � ������� hbmk
   2.6 �����প� �������⥫��� �㭪権
3. ����� � ��⠭���� �ࢥ�
4. ���䨣��஢���� �ࢥ�
   4.1 letodb.ini
   4.2 ���ਧ��� ���짮��⥫��
5. �ᮡ������ ࠡ��� � �ࢥ஬
   5.1 ���������� � �ࢥ஬ ������᪨� �ணࠬ�
   5.2 ����� � 䨫��ࠬ�
6. ��ࠢ����� ��६���묨
7. ���᮪ �㭪権
   7.1 ��ࠢ����� ᮥ��������
   7.2 ����� � �࠭����ﬨ
   7.3 �������⥫�� �㭪樨 ��� ⥪�饩 ࠡ�祩 ������
   7.4 �������⥫�� �㭪樨 rdd
   7.5 ����ன�� ��ࠬ��஢ ������
   7.6 ������� �㭪樨
   7.7 �㭪樨 �ࠢ�����
   7.8 �㭪樨 �ࠢ����� ���짮��⥫ﬨ
   7.9 �㭪樨 �ࠢ����� ��६���묨 �ࢥ�
   7.10 �맮� udf-�㭪権 �� �ࢥ�
   7.11 �㭪樨 ࠡ��� � bitmap 䨫��ࠬ�.
8. �⨫�� �ࠢ����� �ࢥ஬
9. �㭪樨, �믮��塞� �� �ࢥ�


      1. ������� ��⠫����

      bin/          -    �ᯮ��塞� 䠩� �ࢥ�
      doc/          -    ���㬥����
      include/      -    䠩�� ����������
      lib/          -    ������⥪� rdd
      source/
          client/   -    ��室�� ⥪��� rdd
          common/   -    ��騥 ��室�� ⥪��� ��� �ࢥ� � rdd
          client/   -    ��室�� ⥪��� �ࢥ�
      tests/        -    ��⮢� �ணࠬ��, �ਬ���
      utils/
          manage/   -    ��室�� ⥪��� �⨫��� �ࠢ����� �ࢥ஬


      2. ���ઠ letodb

      ��ࢥ� letodb ����� ���� ᮡ࠭ ⮫쪮 ��������஬ Harbour, � ������᪠�
 ������⥪� - ��� Harbour, ⠪ � xHarbour. ��� �� Windows �ࢥ� letodb ����� ����
 ᮡ࠭  ��� �㦡� Windows (������ ���� ���樠����஢�� ����� __WIN_SERVICE__),
 ��� ��� ����� (�����), ��� 祣� ���� ��⠭����� ����� __WIN_DAEMON__.
 ��� ᡮન ���  Linux ���� ��⠭����� ����� __LINUX_DAEMON__. 
      ��ࢥ� � ������᪠� ������⥪� ����� ���� ᮡ࠭� � �����প�� �ࠩ���
 BMDBFCDX/BMDBFNTX. � �⮬ ��砥 �����묨 rdd ��� �ࢥ� letodb ���� �ᯮ�짮������
 ����� DBFCDX/DBFNTX  �ࠩ��� BMDBFCDX/BMDBFNTX, � �㤥� �����ন������ �㭪樮���쭮���,
 �������筠� BMDBF*.  ��� ᡮન � �⮬ ०��� ����室��� � �ਯ�� ᡮન
 (letodb.hbp, rddleto.hbp ��� hbmk2 � makefile.* ��� ��⠫��� ��������஢)
 ��⠭����� ����� __BM.  ��� �⮣� ����室��� �맢��� hbmk2 � ��樥� -env:
 hbmk2 -env:__BM=yes letodb.hbp
 hbmk2 -env:__BM=yes rddleto.hbp
      ��� ᡮન �ࢥ� � �����প�� �㭪権 leto_Zip � leto_Unzip ����室��� ��⠭����� 
 ����� __ZIP:
 hbmk2 -env:__ZIP=yes letodb.hbp

      2.1 ��������� Borland Win32 C

      ��६����� ���㦥��� HB_PATH, � ���ன ��⠭���������� ���� � ��⠫���
 Harbour, ������ ���� ��⠭������ ��। �������樥�. �� ����� ᤥ����,
 � �ਬ���, ������� ��ப� � make_b32.bat:

          SET HB_PATH=c:\harbour

 �᫨ �� �ᯮ���� xHarbour, ᭨��� �������਩ � ��ப� 'XHARBOUR = yes' � makefile.bc.
 ��⥬ ������� make_b32.bat, � �ᯮ��塞� 䠩� �ࢥ� letodb.exe �㤥� ᮧ��� � ��⠫��� bin/,
 � ������⥪� rdd rddleto.lib - � ��⠫��� lib/.

      2.2 ��������� Linux GNU C

      ��६����� ���㦥��� HB_ROOT, � ���ன ��⠭���������� ���� � ��⠫���
 Harbour, ������ ���� ��⠭������ ��। �������樥�. ��� ���� �������
 ���祭�� HRB_DIR � 䠩�� Makefile.linux.

 ��⥬ ������� make_linux.sh, � �ᯮ��塞� 䠩� �ࢥ� letodb �㤥� ᮧ���
 � ��⠫��� bin/, � ������⥪� rdd librddleto.a - � ��⠫��� lib/.

      2.3 xHarbour Builder

      ������� make_xhb.bat ��� ᡮન letodb �⨬ ��������஬. ��������,
  ����室��� �㤥� �������� ���� � xHarbour Builder � make_xhb.bat.
  ���祭�� ��� � xHarbour Builder �� 㬮�砭��:

          set XHB_PATH=c:\xhb

      2.4 ��������� Mingw C

      ��६����� ���㦥��� HB_PATH, � ���ன ��⠭���������� ���� � ��⠫���
 Harbour,  ������ ���� ��⠭������ ��। �������樥�. ��� ����� ᤥ����,
 ��⠢�� ��ப� � 䠩� make_mingw.bat:

          SET HB_PATH=c:\harbour

 �᫨ �� �ᯮ���� xHarbour, ᭨��� �������਩ � ��ப� 'XHARBOUR = yes'
 � 䠩�� makefile.gcc. ��⥬ ������� the make_mingw.bat, � �ᯮ��塞� 䠩�
 �ࢥ� letodb.exe �㤥� ᮧ��� � ��⠫��� bin/, � ������⥪� rdd 
 librddleto.a - � ��⠫��� lib/.

      2.5 ���ઠ letodb � ������� hbmk

      ����� ������ ����������� ᮡ��� letodb � ������� �⨫��� ᡮન Harbour,
 ����ᠭ��� Viktor Szakats. ���⠪�� ��������� ��ப� ��� ᡮન:

      hbmk2 [-hb10|-xhb] rddleto.hbp letodb.hbp [-target=tests/test_ta.prg]

 �������⥫�� ��ࠬ����: -hb10 - Harbour version is 1.0 or 1.0.1,
                      -xhb - xHarbour,
                      -target="blank"\test_ta.prg - ᡮઠ ��⮢�� �ਬ�஢.

      2.6 �����প� �������⥫��� �㭪権

      ��ࢥ� letodb ��� ���᫥��� ���祭�� ���� � �������� ��ࠦ����� � 䨫���
 �ᯮ���� ���᫨⥫� Harbour � ����᪠�� �맮� ᫥����� �㭪権:

      ABS, ALLTRIM, AT, CHR, CTOD, DATE, DAY, DELETED, DESCEND, DTOC, DTOS,
      EMPTY, I2BIN, L2BIN, LEFT, LEN, LOWER, LTRIM, MAX, MIN, MONTH, PAD, PADC,
      PADL, PADR, RAT, RECNO, RIGHT, ROUND, RTRIM, SPACE, STOD, STR, STRZERO,
      SUBSTR, REPLICATE, TIME, TRANSFORM, TRIM, UPPER, VAL, YEAR,
      hb_ATokens, hb_WildMatch

      �᫨ ��� ����� �㭪権 ����室��� ���������, � � ����� source/server.prg
᫥��� �������� ��ப�:

      REQUEST <cFName1>[, ...]

      � ���ᮡ��� �ࢥ�.

      3. ����� � ��⠭���� �ࢥ�

      ���� �������:
      
      letodb.exe                    ( ��� Windows )
      ./letodb                      ( ��� Linux )

      ��� ��⠭��� �ࢥ� ������� �� �� �ᯮ��塞� 䠩� � ��ࠬ��஬
 'stop':

      letodb.exe stop               ( ��� Windows )
      ./letodb stop                 ( ��� Linux )

      ��� ��१���㧪� ����� letoudf.hrb, ������� �ᯮ��塞� 䠩� � ��ࠬ��஬ 'reload':

      letodb.exe reload             ( under Windows )
      ./letodb reload               ( under Linux )

      ��㦡� Windows (�ࢥ� ������ ���� ᮡ࠭ � 䫠��� __WIN_SERVICE__):

      ��� ��⠭���� � 㤠����� �㦡� ����室��� �ࠢ� �����������.
      ��� ��⠭���� �㦡� �맮��� letodb � ��ࠬ��஬ 'install':

      letodb.exe install

      ��� 㤠����� �㦡�, �맮��� letodb � ��ࠬ��஬ 'uninstall':

      letodb.exe uninstall

      ����� � ��⠭�� �㦡� �믮������ �� �������� �㦡.

      4. ���䨣��஢���� �ࢥ�

      4.1 letodb.ini

      �᫨ ��� �� ���ࠨ���� ��ࠬ���� �� 㬮�砭��, �� ����� ��⠭�����
 ���祭�� ��६����� ���䨣��樨 � 䠩�� letodb.ini. � �����饥 �६�
 �������� ᫥���騥 ��ࠬ���� ���䨣��樨 ( 㪠���� ���祭�� �� 㬮�砭�� ),
 ��� ����� ���� �ய�ᠭ� � ᥪ樨 [MAIN]:

      [MAIN]
      Port = 2812              -    ���� �ࢥ�;
      Ip =                     -    ip ���� �ࢥ�; �᫨ �� �� 㪠���, � �ࢥ�
                                    �ᯮ���� �� �⥢� ����䥩�� (�� ip ����),
                                    ����騥�� � ��������;
      TimeOut = -1             -    ⠩���� ��� ᮥ�������;
      DataPath =               -    ���� � ���� ������ �� �ࢥ�;
      LogPath = letodb.log     -    ���� � ��� ���-䠩��;
      Default_Driver = CDX     -    RDD �� 㬮�砭�� ��� ������ 䠩��� �� �ࢥ� ( CDX/NTX );
      Memo_Type =              -    ⨯ ���� ( FPT/DBT ). �� 㬮�砭��: FPT ��� DBFCDX, DBT ��� DBFNTX;
      Lower_Path = 0           -    �᫨ 1, �८�ࠧ����� �� ��� � ������� ॣ�����;
      EnableFileFunc = 0       -    �᫨ 1, ࠧ�襭� �ᯮ�짮����� 䠩����� �㭪権
                                    ( leto_file(), leto_ferase(), leto_frename();
      EnableAnyExt = 0         -    �᫨ 1, ࠧ�襭� ᮧ����� ⠡��� ������ � �����ᮢ � ���७���,
                                    �⫨�� �� �⠭���⭮�� ( dbf,cdx,ntx );
      EnableUDF = 1            -    �᫨ 1 (�� 㬮�砭��), ࠧ�襭� �ᯮ�짮����� udf-�㭪権 � ��䨪ᮬ "UDF_",
                                    �᫨ 2, ࠧ�襭� �ᯮ�짮����� udf-�㭪権 � ��묨 �������,
                                    �᫨ 0, �맮�� udf-�㭪権 ����饭�;
      Pass_for_Login = 0       -    �᫨ 1, ����室��� ���ਧ��� ���짮��⥫�
                                    ��� ᮥ������� � �ࢥ஬;
      Pass_for_Manage = 0      -    �᫨ 1, ����室��� ���ਧ��� ���짮��⥫� ���
                                    �ᯮ�짮����� �㭪権 �ࠢ����� �ࢥ஬
                                    ( Leto_mggetinfo(), etc. );
      Pass_for_Data = 0        -    �᫨ 1, ����室��� ���ਧ��� ���짮��⥫� ���
                                    ����䨪�樨 ������;
      Pass_File = "leto_users" -    ���� � ��� 䠩�� ���ଠ樨 ���짮��⥫��;
      Crypt_Traffic = 0        -    �᫨ 1, � �����, ��।������ �� ��, ��������;
      Share_Tables  = 0        -    �᫨ 0 (�� 㬮�砭��, ��� ०�� ������� � ������
                                    ���� �஥�� letodb), letodb ���뢠�� �� ⠡����
                                    � �������쭮� ०���, �� �������� 㢥�����
                                    �ந�����⥫쭮���. �᫨ 1 (���� ०��, �������� ��᫥ 11.06.2009),
                                    ⠡���� ���뢠���� � ⮬ ०���, � ����� �� ���뢠��
                                    ������᪮� �ਫ������, �������쭮� ��� ०��� ࠧ�������, ��
                                    �������� letodb ࠡ���� ᮢ���⭮ � ��㣨�� �ਫ�����ﬨ.
      No_Save_WA = 0           -    ����� ��� ०�� ��⠭�����, �� ������ �맮�� dbUseArea()
                                    �ࢥ� ����⢨⥫쭮 ���뢠�� 䠩� � ᮧ���� ����� ࠡ����
                                    ������� (workarea) - � ०��� �� 㬮�砭�� �����
                                    䠩� ���뢠���� �ࢥ஬ ⮫쪮 ���� ࠧ � ��ࠧ���� ����
                                    ॠ�쭠� ࠡ��� ������� ��� �⮣� 䠩�� ��� ��� �����⮢.
                                    ������᪨ �� ०�� ����� 㢥����� �ந�����⥫쭮���
                                    �� ����讬 ������⢥ ��⨢��� �����⮢, �� �� �������筮
                                    ���஢����, ���⮬� ��� ࠡ��� �ਫ������ ४���������
                                    �ᯮ�짮���� ०�� �� 㬮�砭��.
      Cache_Records            -    ���-�� ����ᥩ, �⠥��� �� ���� ࠧ (� ��� ������)
      Max_Vars_Number = 10000  -    ���ᨬ��쭮� ������⢮ ࠧ���塞�� ��६�����
      Max_Var_Size = 10000     -    ���ᨬ���� ࠧ��� ⥪�⮢�� ��६�����
      Trigger = <cFuncName>    -    ������쭠� �㭪�� letodb RDDI_TRIGGER
      PendingTrigger = <cFuncName>- ������쭠� �㭪�� letodb RDDI_PENDINGTRIGGER
      EnableSetTrigger = 0     -    �᫨ 1, ࠧ�蠥� �������� ��⠭���� �ਣ���
                                    � ������� dbInfo( DBI_TRIGGER, ... )
      Tables_Max  = 5000       -    ������⢮ ⠡���
      Users_Max = 500          -    ������⢮ ���짮��⥫��
      Debug = 0                -    �஢��� �⫠���
      Optimize = 0             -    �᫨ 1, SET HARDCOMMIT OFF
      AutOrder = 0             -    ����ன�� ��� SET AUTORDER
      ForceOpt = 0             -    ����ன�� ��� _SET_FORCEOPT

      �������� ��।����� ᥪ�� [DATABASE], �᫨ �� ��� 㪠���� ��⠫�� ��,
 � ���஬ ⠡���� ������ ���뢠���� ��㣨� RDD:

      [DATABASE]
      DataPath =               -    (��易⥫쭠� ����)
      Driver = CDX             -    ( CDX/NTX )

      ����� ��।����� �⮫쪮 ᥪ権 [DATABASE], ᪮�쪮 ����室���.

      � Windows 䠩� letodb.ini ������ ���� ࠧ��饭 � ⮬ ��⠫���, �
 ���஬ ��室���� �ࢥ� letodb.
      � Linux �ࢥ� ��� ��� 䠩� � ⮬ ��⠫���, ��㤠 �� ���⮢��,
 ���, �� ��㤠�, � ��⠫��� /etc.

      4.2 ���ਧ��� ���짮��⥫��

      �⮡� ������� �����⥬� ���ਧ�樨, ����室��� ��⠭����� ���� �� ᫥����� ��ࠬ��஢
 letodb.ini � 1: Pass_for_Login, Pass_for_Manage, Pass_for_Data. �� ��। �⨬
 ����室��� ᮧ����, ��� ������, ������ ���짮��⥫� � �ࠢ��� �����������, ��᪮��� �����
 ��⥬� ���ਧ�樨 ࠡ�⠥�, ⮫쪮 ���ਧ������ ���짮��⥫� � �ࠢ��� ����������� �����
 ���������/�������� ���짮��⥫�� � ��஫�.
      �⮡� �������� ���짮��⥫�, ����室��� ������� �맮� �㭪樨 LETO_USERADD() � ���������
 �ணࠬ��, � �ਬ���:

      LETO_USERADD( "admin", "secret:)", "YYY" )

 ��� "YYY" �� ��ப�, ����� ���� �ࠢ� ���������஢����, �ࠢ����� � �ࠢ� �� ������. �� �����
 ⠪�� �ᯮ�짮���� �ணࠬ�� utils/manager/console.prg, �⮡� ��⠭����� ��� �������� ����� ���ਧ�樨.

 ��� ᮥ������� � �ࢥ஬ � ����묨 ���ਧ�樨 ( ������ ���짮��⥫� � ��஫��) ����室���
 �맢��� �㭪�� LETO_CONNECT().

      5. ���������� � �ࢥ஬ ������᪨� �ணࠬ�

      �⮡� ᪮��������� � �ࢥ஬, �०�� �ᥣ� ����室��� �ਫ�������� ������⥪� rddleto
 � ᢮��� �ਫ������, � �������� � ��砫� ᢮�� �ணࠬ�� ��� ��ப�:

      REQUEST LETO
      RDDSETDEFAULT( "LETO" )

      ��� ������ 䠩�� dbf �� �ࢥ� ����室��� ��⠢��� � ������ SET PATH TO,
 ��� � ������� USE ���� � �ࢥ�� � �⠭���⭮� �ଥ:
 //ip_address:port/data_path/file_name.

      �᫨ ����� ��ࠬ��� 'DataPath' � ���䨣��樮���� 䠩�� �ࢥ�, � �� �����
 �����⮥ ���祭��, ����室��� 㪠�뢠�� �� ����� ���� � 䠩�� �� �ࢥ�,
 � ���� �⭮�⥫�� ( �⭮�⥫쭮 ���祭�� 'DataPath' ).
      ���ਬ��, �᫨ ����室��� ������ 䠩� test.dbf, ����� �ᯮ����� ��
 �ࢥ� 192.168.5.22 � ��⠫��� /data/mydir � ���祭�� ��ࠬ��� 'DataPath'
 ( � 䠩�� ���䨣��樨 �ࢥ� letodb.ini ) '/data', ᨭ⠪�� ������ ����
 ⠪��:

      USE "//192.168.5.22:2812/mydir/test"

      �᫨ �ࢥ� �� ����饭 ��� �� 㪠���� ������ ����, �㤥� ᣥ���஢��� �訡�� ������.
 �������� �஢���� ����㯭���� �ࢥ� ��। ����⨥� 䠩��� �맮��� �㭪樨
 leto_Connect( cAddress ), ����� ��୥� -1 � ��砥 ��㤠筮� ����⪨:

      IF leto_Connect( "//192.168.5.22:2812/mydir/" ) == -1
         Alert( "Can't connect to server ..." )
      ENDIF

      �� ᮥ������� � �ࢥ஬ ������ ��।��� ��� ���ଠ�� � ������� ��࠭��, ������
�ࢥ� ��⥬ �ᯮ���� ��� ����権 ������樨, 䨫���樨 � ��� �������� ��㣨� ����権.
������� ��࠭�� �� ������ ������ ���� ��⠭������ �� ᮥ������� � �ࢥ஬ letodb.

      5.2 ����� � 䨫��ࠬ�

      ������ ��⠭���������� ����� ��ࠧ��: � ������� ������� SET FILTER TO ��� �맮���
�㭪樨 dbSetFilter(). ������, ����� ����� ���� �믮���� �� �ࢥ�, ���뢠���� ��⨬���஢����.
�᫨ 䨫��� �� ����� ���� �믮���� �� �ࢥ�, �� ���� ����⨬���஢����. ����� 䨫���
���� ��������, ��᪮��� � �ࢥ� �� ࠢ�� ����訢����� �� �����, ����� ��⥬
䨫�������� �� ������. �⮡� ������ ��⨬���஢���� 䨫���, ����室���, �⮡� � �����᪮�
��ࠦ���� ��� 䨫��� ������⢮���� ��६���� ��� �㭪樨, ��।������ �� ������.
�⮡� �஢����, ���� �� 䨫��� ��⨬���஢����, ���� �맢��� �㭪�� LETO_ISFLTOPTIM().

      6. ��ࠢ����� ��६���묨

      Letodb �������� ᮧ������ ��६����, ����� ࠧ�������� ����� �ਫ�����ﬨ,
 ����� ᮥ�������� � �ࢥ஬. ��६���� ����� ࠧ���� �� ��㯯�, � ��� ����� ����� �����᪨�,
 楫� ��� ᨬ����� ⨯. ����� ��⠭����� ���祭�� ��६�����, ������� ���, 㤠����, ��� �맢���
 ���६���/���६��� ( �᫮��� ��६�����, ����⢥��� ). �� ����樨 � ��६���묨 �믮�������
 ��᫥����⥫쭮 � ����� ��⮪�, ⠪ �� ��६���� ����� �ᯮ�짮������ ��� ᥬ����. ��. ᯨ᮪
 �㭪権 ��� ᨭ⠪�� �㦭�� �㭪樨, � �ਬ�� tests/test_var.prg.
      ���� Letodb.ini ����� ᮤ�ঠ�� ��ப� ��� ��।������ ���ᨬ��쭮�� ������⢠ ��६�����
 � ���ᨬ��쭮� ����� ⥪�⮢�� ��६�����.

      7. ���᮪ �㭪権

      ���� �ਢ���� ����� ( �� ������ ����ᠭ�� ) ᯨ᮪ �㭪権,
 ����㯭�� ��� �ᯮ�짮����� � �����᪮� �ਫ������ � �ਫ��������� RDD LETO.

      7.1 ��ࠢ����� ᮥ��������

      LETO_CONNECT( cAddress, [ cUserName ], [ cPassword ], [ nTimeOut ], [ nBufRefreshTime ] )
                                                           --> nConnection, -1 �� ��㤠�
 nBufRefreshTime ��।���� ���ࢠ� � 0.01 ᥪ. �� ���祭�� �⮣� ���ࢠ��
 ���� ��� ����ᥩ �㤥� ��������. ���祭�� �� 㬮�砭�� 100 (1 ᥪ)

      LETO_CONNECT_ERR()                                   --> nError
      LETO_DISCONNECT( [ cConnString | nConnection ] )     --> nil
      LETO_SETCURRENTCONNECTION( [ cConnString | nConnection ] ) --> nConnection
      LETO_GETCURRENTCONNECTION()                          --> nConnection
      LETO_GETSERVERVERSION()                              --> cVersion
      LETO_GETLOCALIP()                                    --> IP ���� ������
      LETO_ADDCDPTRANSLATE(cClientCdp, cServerCdp )        --> nil
      LETO_PATH( [<cPath>], [cConnString | nConnection] )  --> cOldPath
      LETO_PING( [ cConnString | nConnection ] )           --> lResult

      7.2 ����� � �࠭����ﬨ

      LETO_BEGINTRANSACTION( [ nBlockLen ], [ lParseTrans ] )
 ��ࠬ��� <nBlockLen> �ᯮ������ ��� ��࠭�� ࠧ��� ����� ��� �뤥����� �����.
 ��� ������ �࠭���権 ⠪�� ��ࠧ�� ����� ����⢥��� �᪮��� �믮������ �࠭���樨
 �� ��஭� ������.
 �᫨ ���祭�� ��ࠬ��� <lParseTransRec> ࠢ�� .F., ���� �࠭���樨 �� ��ࠡ��뢠����
 �� �६� ����祭�� ����ᥩ � �ࢥ�.

      LETO_ROLLBACK()

      LETO_COMMITTRANSACTION( [ lUnlockAll ] )             --> lSuccess

      LETO_INTRANSACTION()                                 --> lTransactionActive

      7.3 �������⥫�� �㭪樨 ��� ⥪�饩 ࠡ�祩 ������

      LETO_COMMIT()
 �� �㭪�� ����� �ᯮ�짮������ ��� ⥪�饩 ࠡ�祩 ������ ����� �맮�� �㭪権:

 dbCommit()
 dbUnlock()

 ������ ��ࠢ��� �ࢥ�� 3 �����: ��� ���������� ⥪�饩 �����, ��� ��� ⥪�饩
 ����� �� ��� � ��� ���� �����஢�� ࠡ�祩 ������. Leto_Commit ��ࠢ��� ⮫쪮
 ���� ����� ��� �믮������ ��� ����権

      LETO_SUM( <cFieldNames>|<cExpr>, [ cFilter ], [xScope], [xScopeBottom] )
                                                           --> nSumma - �᫨ ��।��� ���� ���� ��� ��ࠦ����, ���
                                                               {nSumma1, nSumma2, ...} ��� ��᪮�쪨� �����
 ���� ��ࠬ��� leto_sum �� ᯨ᮪ ����� ��� ��ࠦ����, ࠧ�������� ����⮩:
 leto_sum("Sum1,Sum2", cFilter, cScopeTop, cScopeBottom) �����頥�
 ���ᨢ � �㬬��� ����� Sum1 � Sum2.

 �᫨ � ����⢥ ����� ���� ��।�� ᨬ��� "#", leto_sum �����頥� ������⢮ ��ࠡ�⠭��� ����ᥩ:
 leto_sum("Sum1,Sum2,Sum1+Sum2,#", cFilter, cScopeTop, cScopeBottom) -->
 {nSum1, nSum2, nSum3, nCount}

 �᫨ �㭪樨 ��।��� ⮫쪮 ���� ��� ����, ��� �����頥� �᫮��� ���祭�� �㬬� �⮣� ����

      LETO_GROUPBY(cGroup, <cFields>|<cExpr>, [cFilter], [xScopeTop], [xScopeBottom]) --> aValues

 �� �㭪�� �����頥� ��㬥�� ���ᨢ. ���� ����� ������ ��ப� - ��
 ���祭�� ���� <cGroup>, ������ � 2-�� - �㬬� ����� ��� ��ࠦ����,
 ࠧ�������� ����⮩, �������� � ��ࠬ��� <cFields>. �᫨ ᨬ��� "#" ��।��
 � ����⢥ ����� ���� � <cFields>, leto_groupby �����頥� ������⢮ ����ᥩ
 � ������ ��㯯�.
                                                               {{xGroup1, nSumma1, nSumma2, ...}, ...}
      LETO_ISFLTOPTIM()                                    --> lFilterOptimized

      dbInfo( DBI_BUFREFRESHTIME[, nNewVal])               --> nOldVal
  ��⠭���� ������ ���祭�� ��� �६��� ���������� skip � seek ���஢ ⥪�饩
  ࠡ�祩 ������ � 0.01 ᥪ. �᫨ -1: �ᯮ������ ����ன�� ��� ᮥ�������.
  �᫨ 0 - ���� �ᯮ������� ����ﭭ�.

      dbInfo( DBI_CLEARBUFFER )
  ������� ��頥� skip ����.

      7.4 �������⥫�� �㭪樨 rdd

      leto_CloseAll( [ cConnString | nConnection ] )       --> nil
 ����뢠�� �� ࠡ�稥 ������ ��� 㪠������� ᮥ������� ��� ᮥ������� �� 㬮�砭��

      7.5 ����ன�� ��ࠬ��஢ ������

      LETO_SETSKIPBUFFER( nSkip )                          --> nCount (����⨪� �ᯮ�짮����� ����)
 Skip-���� �।�����祭 ��� ��⨬���樨 ������⢥���� �맮��� skip
 �� �㭪�� ������ ࠧ��� � ������� ��� skip-���� ��� ⥪�饩 ࠡ�祩 ������.
 �� 㬮�砭�� ࠧ��� skip-���� 10 ����ᥩ. Skip-���� ���� ��㭠�ࠢ�����.
 skip-���� ���뢠���� ��᫥ BUFF_REFRESH_TIME (1 ᥪ)
 �᫨ ��ࠬ��� �㭪樨 ���饭, ��� �����頥� ����⨪� �ᯮ�짮����� ���� (������⢮ ���������)

      LETO_SETSEEKBUFFER( nRecsInBuf )                     --> nCount (����⨪� �ᯮ�짮����� ����)
 Seek ���� �।�����祭 ��� ��⨬���樨 ������⢥���� �맮��� seek
 �� �㭪�� ������ ࠧ��� � ������� ��� seek-���� ��� ⥪�饣� ������.
 �᫨ ������ ������� � ���� �� �믮������ dbSeek(), ��� �� ����㦠���� � �ࢥ�.
 ������� skip-�����, seek-���� ���뢠���� ��᫥ BUFF_REFRESH_TIME (1 ᥪ)
 �᫨ ��ࠬ��� �㭪樨 ���饭, ��� �����頥� ����⨪� �ᯮ�짮����� ���� (������⢮ ���������)
 �� 㬮�砭��, seek-���� �몫�祭.

      LETO_SETFASTAPPEND( lFastAppend )                    --> lFastAppend (�।��饥 ���祭��)
 �᫨ �� ����� ०�� fast append, � ������ ���������� �� �ࢥ� �ࠧ�
 ��᫥ �맮�� �㭪樨 dbAppend(). �᫨ ��� ०�� �����, � ������ ����������
 �� �맮�� dbCommit(), ���� ��᫥ �맮�� �㭪権 ������樨. �� 㬮�砭�� ०��
 fast append �� �����. ����⢨� �⮩ ��⠭���� �����࠭���� �� �� ࠡ�稥 ������

      RddInfo( RDDI_REFRESHCOUNT, <lSet>,, [nConnection] )
 �� 㬮�砭�� 䫠� RDDI_REFRESHCOUNT ��⠭�����. �᫨ �� ��⠭�����, �㭪��
 RecCount() ����騢��� ���祭�� ������⢠ ����ᥩ � �ࢥ�, �᫨ �� ��⠭����� -
 �ᯮ���� ��᫥���� ���祭��, ����祭��� � �ࢥ�. �᫨ ��㣮� �ਫ������
 �������� ����� � ⥡����, � ���祭�� ������⢠ ����ᥩ �� ����� ���� ����祭�
 �ࠧ�.
 �᫨ 䫠� RDDI_REFRESHCOUNT �� ��⠭�����, dbGoto(0) ��頥� ���� ����� �
 ��⠭�������� 䫠�� eof � ��㣨� ����� ����� � �ࢥ��.

      RddInfo( RDDI_BUFKEYCOUNT, <lSet>,, [nConnection] )
 �� 㬮�砭�� 䫠� RDDI_BUFKEYCOUNT �� ��⠭�����. �᫨ �� �� ��⠭�����, �㭪��
 ordKeyCount() ����騢��� ���祭�� ������⢠ ���祩 � �ࢥ�, �᫨ ��⠭����� -
 �ᯮ���� ��᫥���� ���祭��, ����祭��� � �ࢥ�.

      RddInfo( RDDI_BUFKEYNO, <lSet>,, [nConnection] )
 �� 㬮�砭�� 䫠� RDDI_BUFKEYNO �� ��⠭�����. �᫨ �� �� ��⠭�����, �㭪��
 ordKeyNo() ����騢��� ���祭�� �ࢥ�, �᫨ ��⠭����� -
 �ᯮ���� ��᫥���� ���祭��, ����祭��� � �ࢥ�.

      RddInfo( RDDI_CLIENTID,,, [nConnection] )
 �����頥� ᮡ�⢥��� ����� ������ (nUserStru).

      7.6 ������� �㭪樨

 ��ࠬ��� <cFileName> ��� 䠩����� �㭪権 ����� ᮤ�ঠ�� ��ப� �������
 � �ࢥ�� letodb � �ଠ�:
 //ip_address:port/data_path/file_name.
 �᫨ ��ப� ������� ���饭�, �ᯮ������ ⥪�饥 ��⨢��� ᮥ�������:
 /data_path/file_name.
 �� 䠩�� �ᯮ�������� �⭮�⥫쭮 ��⠫��� DataPath �� �ࢥ�. �ᯮ�짮�����
 ����� ".." � ����� ����饭�.

      LETO_FILE( cFileName )                               --> lFileExists
 �஢���� ����⢮����� 䠩�� �� �ࢥ�, ������ �㭪樨 File()

      LETO_FERASE( cFileName )                             --> -1 �� ��㤠�
 ������ 䠩� �� �ࢥ�.

      LETO_FRENAME( cFileName, cFileNewName )              --> -1 �� ��㤠�
 ��२�����뢠�� 䠩� <cFileName> --> <cFileNewName>. cFileNewName ������ ����
 㪠��� ��� ��ப� �������.

      LETO_MEMOREAD( cFileName )                           --> cStr
 �����頥� ᮤ�ন��� 䠩�� �� �ࢥ� ��� ᨬ������ ��ப�, ������ �㭪樨
 MemoRead()

      Leto_MemoWrite( cFileName, cBuf )                    --> lSuccess
 �����뢠�� ᮤ�ন��� ᨬ���쭮� ��ப� � 䠩� �� �ࢥ�, ������ �㭪樨
 MemoWrit()

      LETO_MAKEDIR( cDirName )                             --> -1 �� ��㤠�
 ������� ��⠫�� �� �ࢥ�

      LETO_DIREXIST( cDirName )                            --> lDirExists
 �஢���� ����⢮����� ��⠫��� �� �ࢥ�

      LETO_DIRREMOVE( cDirName )                           --> -1 �� ��㤠�
 ������ ��⠫�� �� �ࢥ�

      LETO_FERROR()                                        --> nError
 �����頥� ��� �訡�� ��᫥���� 䠩����� �㭪樨

      Leto_FileRead( cFileName, nStart, nLen, @cBuf )      --> -1 if failed
 ��⠥� ᮤ�ন��� 䠩�� � ᬥ饭�� <nStart> ࠧ��஬ <nLen>

      Leto_FileWrite( cFileName, nStart, cBuf )            --> lSuccess
 �����뢠�� ��६����� <cBuf> � 䠩� �� �ࢥ� � ᬥ饭�� <nStart>

      Leto_FileSize( cFileName )                           --> -1 if failed
 �����頥� ࠧ��� 䠩�� �� �ࢥ�

      Leto_FileAttr( cFileName [, cNewAttr] )              --> cAttr
 �����頥� ��� ��⠭�������� ��ਡ��� 䠩�� �� �ࢥ�

      Leto_Directory( cDir[, cAttr] )                      --> aDirectory
 �����頥� ᮤ�ন��� ��⠫��� �� �ࢥ� � �ଠ�, �������筮� �㭪樨
 Directory()

      7.7 �㭪樨 �ࠢ�����

      LETO_MGGETINFO()                                     --> aInfo
 �� �㭪�� �����頥� ��ࠬ���� ⥪�饣� ᮥ������� � ���� ���ᨢ� ��
 17 ����⮢ ᨬ���쭮�� ⨯�:
 1 - ⥪�饥 �-�� ���짮��⥫��
 2 - ���ᨬ��쭮� �-�� ���짮��⥫��
 3 - ⥪�饥 �-�� ������� ⠡���
 4 - ���ᨬ��쭮� �-�� ������� ⠡���
 5 - �६� ࠡ��� �ࢥ�
 6 - �-�� ����権
 7 - ���� ��ࠢ����
 8 - ���� ���⠭�
 9 - ⥪�饥 �-�� ������� �����ᮢ
 10 - ���ᨬ��쭮� �-�� ������� �����ᮢ
 11 - data path
 12 - max day wait
 13 - ulWait
 14 - ��饥 �-�� �࠭���権
 15 - �-�� �ᯥ��� �࠭���権
 16 - �ᯮ�짮���� �����
 17 - ���ᨬ��쭮 �ᯮ�짮���� �����

      LETO_MGGETUSERS( [<cTableId>|<nTable>] )             --> aInfo
 �㭪�� �����頥� ��㬥�� ���ᨢ, ������ ��ப� ���ண�
 ᮤ�ন� ���ଠ�� � ���짮��⥫�:
 aInfo[i,1] - ����� ������
 aInfo[i,2] - ip ����
 aInfo[i,3] - �⥢�� ��� ������
 aInfo[i,4] - ��� �ணࠬ��
 aInfo[i,5] - ⠩����

      LETO_MGGETTABLES( [<cUserId>|<nUser>] )              --> aInfo
 �㭪�� �����頥� ��㬥�� ���ᨢ, ������ ��ப� ���ண�
 ᮤ�ন� ���ଠ�� �� ������� ⠡����:
 aInfo[i,1] - ����� ⠡����
 aInfo[i,2] - ��� ⠡����

      LETO_MGGETLOCKS( [<cUserId>|<nUser>] )               --> aInfo
 �㭪�� �����頥� ��㬥�� ���ᨢ, ������ ��ப� ���ண�
 ᮤ�ন� ���ଠ�� � �����஢���:
 aInfo[i,1] - ��� ⠡����
 aInfo[i,2] - ��ப� � ᯨ᪮� �����஢����� ����ᥩ

      LETO_MGGETTIME()                                     --> aDateTime
 �㭪�� �����頥� ���ᨢ {<dDate>, <nSeconds>}:
 dDate - ��� �ࢥ�;
 nSeconds - ᥪ㭤 ��᫥ ���㭮�.
 ��� �८�ࠧ������ ���祭�� � ��६����� DateTime (Harbour) �맮���:
 hb_DTOT( aDateTime[1], aDateTime[2] )

      LETO_MGKILL( <nUser>|<cUserId> )
 �㭪�� �⪫�砥� ���짮��⥫� � ����஬ <nUser>

      LETO_LOCKCONN( lOnOff )                              --> lSuccess
 ��᫥ �맮�� leto_lockconn( .t. ) �� ���� ᮥ������� ����������� �ࢥ஬,
 �� �맮�� leto_lockconn( .f. )

      LETO_LOCKLOCK( lOnOff, nSecs )                       --> lSuccess
 �� �㭪�� ������� �����襭�� ��� ����権 ���������� ������, ��� ���
 ����������, � ��⠥��� �����஢��� �ࢥ� �� ��� ��������� � �����⮢.
 �᫨ ����⪠ �����஢�� �뫠 �ᯥ譮�, �㭪�� �����頥� True.

      7.8 �㭪樨 �ࠢ����� ���짮��⥫ﬨ

      LETO_USERADD( cUserName, cPass [, cRights ] )        --> lSuccess
      LETO_USERPASSWD( cUserName, cPass )                  --> lSuccess
      LETO_USERRIGHTS( cUserName, cRights )                --> lSuccess
      LETO_USERFLUSH()                                     --> lSuccess
      LETO_USERGETRIGHTS()                                 --> cRights

      7.9 �㭪樨 �ࠢ����� ��६���묨 �ࢥ�

      LETO_VARSET( cGroupName, cVarName, xValue[, nFlags[, @xRetValue]] ) --> lSuccess

 �㭪�� ��ᢠ����� ���祭�� <xValue> ��६����� <cVarName> �� ��㯯� <cGroupName>
 ����易⥫�� ��ࠬ��� <nFlags> ����� ������ ०��� ࠡ��� �㭪樨:
 LETO_VCREAT    - �᫨ ��६����� �� �������, ��� �㤥� ᮧ����;
 LETO_VOWN      - ᮡ�⢥���� ��६����� ���짮��⥫�, �᢮��������� ��᫥ ��ᮥ�������;
 LETO_VDENYWR   - ����� ��ᢠ������ ���祭�� ��㣨� ���짮��⥫��;
 LETO_VDENYRD   - ����� �⥭�� ���祭�� ��㣨� ���짮��⥫��;
 LETO_VPREVIOUS - ������ �।��饣� ���祭�� ��६����� � <xRetValue>.

      LETO_VARGET( cGroupName, cVarName )                  --> xValue

 �㭪�� �����頥� ���祭�� ��६����� <cVarName> �� ��㯯� <cGroupName>

      LETO_VARINCR( cGroupName, cVarName )                 --> nValue

 ���६��� ���祭�� ��६����� <cVarName> �� ��㯯� <cGroupName>

      LETO_VARDECR( cGroupName, cVarName )                 --> nValue

 ���६��� ���祭�� ��६����� <cVarName> �� ��㯯� <cGroupName>

      LETO_VARDEL( cGroupName, cVarName )                  --> lSuccess

 �������� ��६����� <cVarName> �� ��㯯� <cGroupName>

      LETO_VARGETLIST( [cGroupName [, nMaxLen]] )          --> aList

 �㭪�� �����頥� ��㬥�� ���ᨢ ��६�����: { {<cVarName>, <value>}, ...}

      7.10 �맮� udf-�㭪権 �� �ࢥ�

      LETO_UDF( cSeverFunc, xParam1, ... )                 --> xResult
 �㭪�� ��뢠���� �� ������᪮�� �ਫ������. ��ப�
 <cServerFunc> - ��ப� � ��ࠬ��ࠬ� ᮥ������� �ࢥ� � ������
 udf-�㭪樨:
 //ip_address:port/funcname
 �㭪�� <funcname> ������ ����⢮���� �� letodb �ࢥ�.
 ���� ��ࠬ��� udf-�㭪樨 - nUserStru.
 Udf-�㭪�� ����� �������� १���� (��� ⨯�) �������.
 �ਬ��� udf-�㭪権 �. � tests/letoudf.prg

      LETO_UDFEXIST( cSeverFunc )                          --> lExist
 leto_udfExist �஢����, ������� �� udf-�㭪�� �� �ࢥ� letodb.
 ��ࠬ��� <cSeverFunc> �������� ⠪ ��, ��� � ��� leto_udf().

      LETO_PARSEREC( cRecBuf )                             --> nil
 ��� �㭪�� ����室��� �맢���, �᫨ udf-�㭪�� �����頥� � ����⢥
 १���� ���� �����, ����� ������ ���� ⥪�饩 � १���� ��
 ࠡ���.

      LETO_PARSERECODRS( cRecBuf )                         --> nil
 ��� �㭪�� ����室��� �맢��� ��� ���������� skip-���� ��� १����
 �믮������ udf-�㭪樨 UDF_dbEval. ��᫥ �맮�� �⮩ �㭪樨 ����� ��
 skip-���� �롨����� �맮���� �㭪樨 dbSkip(). �ਬ�� �. � ����������
 � UDF_dbEval � tests/letoudf.prg

      7.11 �㭪樨 ࠡ��� � bitmap 䨫��ࠬ�.

 �᫨ letodb ᮡ࠭ � �����প�� rdd BMDBFCDX/BMDBFNTX, � ������� �����প�
 ᫥����� �㭪権:

      LBM_DbGetFilterArray()                               --> aFilterRec
      LBM_DbSetFilterArray( aFilterRec )                   --> nil
      LBM_DbSetFilterArrayAdd( aFilterRec )                --> nil
      LBM_DbSetFilterArrayDel( aFilterRec )                --> nil

 �����祭�� � ��ࠬ���� ��� �㭪権 ⠪�� ��, ��� � ᮮ⢥������� �㭪権 BM_*.

      LBM_DbSetFilter( [<xScope>], [<xScopeBottom>], [<cFilter>] ) -> nil
 �㭪�� ��⠭�������� bitmap-䨫��� ��� ⥪�饣� ������ � �� �᫮���,
 ��।�������� ��ࠬ��ࠬ� <xScope>, <xScopeBottom>, <cFilter>
 ������ ������ ��᫥ �맮�� LBM_DbSetFilter() �㤥� ��ࢮ� �������,
 㤮���⢮���饩 �᫮��� 䨫���.

      8. �⨫�� �ࠢ����� �ࢥ஬

      ����� ᮡ��� ��� �⨫��� �ࠢ����� �ࢥ஬, GUI � ���᮫���. ��室�� ⥪���
 ��室���� � ��⠫��� utils/manage directory.

      �⨫�� GUI, manage.prg, ᮧ���� � ������� ������⥪� HwGUI. �᫨ � ��� ��⠭������ HwGUI,
 ��⠢�� ��ப� 'set HWGUI_INSTALL=' � 䠩� utils/manage/bld.bat ���� � 
 ��⠫��� HwGUI � ������� bld.bat. �⨫�� manage.exe �㤥� ᮡ࠭�.
   
      ��� ��, �� �� �ᯮ���� HwGUI, ���� �⨫�� �ࠢ����� � ०��� ���᮫�,
 console.prg. ������ console.exe � ������� 䠩�� make/bat, ����� �� �ᯮ����
 ��� ᡮન �ணࠬ�� Harbour, ����饩 �� ������ �����. ����室��� ⮫쪮 ��������
 rddleto.lib � ᯨ�� ������⥪. �������e console.exe � ������ �ࢥ� ��� ip ���ᮬ
 � ����஬ ���� � ����⢥ ��ࠬ���:

      console.exe server_name:nPort
      console.exe ip_address:nPort

 server_name � ip_address � ��������� ��ப� ������ ���� ��� ������ ᫥襩
 ( '//' ), ��᪮��� Clipper/Harbour �ᯮ���� �� ��� ᢮�� �㦤.

      9. �㭪樨, �믮��塞� �� �ࢥ�

      �� �㭪樨 ����� ��뢠���� �� �믮������ � ������ � ������� �㭪樨
 leto_udf, � ⠪�� �� �㭪権, ��।������� � 䠩�� letoudf.hrb. ���� ��ࠬ���
 ⠪�� �㭪樨 �ᥣ�� nUserStru

      leto_Alias( nUserStru, cClientAlias )                --> cRealAlias
 �㭪�� �����頥� ����� �ࢥ� ��� ������᪮�� ����� <cClientAlias>.
 ����� �ࢥ� ��⥬ ����� �ᯮ�짮������ � ������ rdd-�������

      leto_RecLock( nUserStru [, nRecord] )                --> lSuccess
 �㭪�� leto_RecLock �������� ������ � ����஬ <nRecord>, ��� ⥪���� ������

      leto_RecUnLock( nUserStru [, nRecord] )              --> Nil
 �㭪�� leto_UnRecLock ᭨���� �����஢�� � ����� ����� <nRecord>, ��� �
 ⥪�饩 �����

      leto_RecLockList( nUserStru, aRecNo )                --> lSuccess
 �㭪�� leto_RecLockList �������� ����� � ����ࠬ�, 㪠����묨 � ���ᨢ�
 aRecNo. �᫨ �����஢�� �����-���� ����� �� 㤠����, �����஢�� ᭨������,
 � �����頥��� १���� .F.
      ��� �㭪�� ����� ��뢠�� �� �ࢥ� � ���㫥 letoudf.prg, ��� � ������
 �맮��� leto_UDF( "leto_RecLockList", aRecNo )

      leto_TableLock( <nUserStru>, [<nFlag>], [<Secs>])    --> lSuccess
      leto_TableUnLock( <nUserStru>, [<nFlag>])

      nFlag - ��ࠬ��� �� 1 �� 32, nSecs - ᥪ㭤 �������� �����஢�� (1 �� 㬮�砭��)
      �� �㭪樨 �।�����祭� ��� ���ᯥ祭�� ������७⭮�� ����㯠 � ⠡���
      ��� ����⢨� �஬� ���������� ������.

      leto_SelectArea( nUserStru, nAreaId )                --> lSuccess

      leto_areaID( nUserStru )                             --> nAreaId
 �㭪�� �����頥� ����७��� ����� id ��� ⥪�饩 ࠡ�祩 ������

      leto_ClientID()                                      --> nUserStru
 �㭪�� �����頥� ����� ������ (nUserStru), -1 �� ��㤠�

      letoUseArea( nUserStru, cFileName, cAlias, lShared, lReadOnly, cdp ) --> nAreaId
      letoOrdListAdd( nUserStru, cBagName )                --> Nil
      letoOrdCreate( nUserStru, cBagName, cKey, cTagName,
                     lUnique, cFor, cWhile, lAll, nRecNo,
                     nNext, lRest, lDesc, lCustom,
                     lAdditive )                           --> Nil
      letoCloseArea( nUserStru )                           --> Nil

 �㭪樨 letoUseArea, letoOrdListAdd, letoOrdCreate, letoCloseArea,
 leto_RecLock, leto_RecLock �।�����祭� ��� �ᯮ�짮����� � udf-�㭪��� �����
 rdd �㭪権: dbUseArea, OrdListAdd, OrdCreate, dbCloseArea, RLock, dbUnlock

 �㭪樨 �ࠢ����� ��६���묨 �ࢥ�:

      LETO_VARSET( nUserStru, cGroupName, cVarName, xValue[, nFlags )
                                                           --> lSuccess
      LETO_VARGET( nUserStru, cGroupName, cVarName )       --> xValue
      LETO_VARINCR( nUserStru, cGroupName, cVarName )      --> nValue
      LETO_VARDECR( nUserStru, cGroupName, cVarName )      --> nValue
      LETO_VARDEL( nUserStru, cGroupName, cVarName )       --> lSuccess
      LETO_VARGETLIST( nUserStru, [cGroupName, [lValue]] ) --> aList

 Zip/Unzip �㭪樨:

      leto_Zip( nUserStru, [cDirName], [acFiles], [nLevel], [lOverwrite], [cPassword], [acExclude], [lWithPath] )
                                                           --> cZip
      leto_UnZip( nUserStru, [cDirName], cZip, [cPassword], [lWithPath] )   --> lSuccess
 ���:
 cDirName - ��⠫�� �⭮�⥫쭮 DataPath;
 cZip - ᮤ�ন��� ��娢� ��� �ᯠ�����;
 ��⠫�� ��ࠬ���� ᮮ⢥������ ��ࠬ��ࠬ �㭪権 hb_ZipFile/hb_UnZipFile.
 �맮� �㭪権 � ������:
 leto_UDF("leto_Zip", [cDirName], ...)
 leto_UDF("leto_UnZip", [cDirName], cZip, ...)
