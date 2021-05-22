MODULE SetUp;

FROM StdInOut IMPORT WriteLn, WriteString, ReadCard, GotoXY, CLS, ReadLine,
                                                                ReadString;
FROM InOut IMPORT Read;
FROM Utilities IMPORT Beep;
FROM NumberConversion IMPORT StringToCard;
FROM ASCII IMPORT esc, nul, EOL;
FROM MyLib IMPORT ReadStr, WriteDecimal, RaisePower, SoundUp, SoundDown, Error,
          ScreenSaver, WriteTime, WriteDate, ReadStrEOLFS, BigBox, ClearScreen,
          Pause, YesNo, YesNo1, More, FindMins, ReadCaps, GetDate, Header, READ,
          LineHeader, ConvertDecimal, ReadPart;
IMPORT FileSystem; (* Lookup, SetPos, WriteChar, Close, Rename,
                       Response, GetPos, Length, ReadChar, File; *)
FROM Strings IMPORT CompareStr, Insert, Concat, ConstAssign, Assign, Delete;
FROM Terminal IMPORT KeyPressed;
FROM GetInit IMPORT InitString;
FROM Video IMPORT Black, Blue, Brown, Cyan, Green, Red, Magenta, Gray, High,
           Blink, ClrEOL, ClrLine, Color, ColorBg, ColorFg, CursorOff,
           CursorOn, DeleteLine, Down, GetAttrib, GetCursor, InsertLine, Left,
           Right, SetAttrib, SetCursor, ScrollDown, ScrollUp, SetVideo, Up,
           WhatPage, WhereX, WhereY, WriteAtXY;

VAR
         InitStr : ARRAY [0..43] OF CHAR;
           TelNo : ARRAY [0..13] OF CHAR;
 TempId, LogonId : ARRAY [0..12] OF CHAR;
            Port : ARRAY [0..3] OF CHAR;
            Baud : ARRAY [0..3] OF CHAR;
         Setting : ARRAY [0..2] OF CHAR;
   ValidKey, MInit, ModemChanged, PwUpdated : BOOLEAN;
            I, j : CARDINAL;
              ch : CHAR;
            file : FileSystem.File;

PROCEDURE GetModemStuff(VAR number, logonid, initstr, port,
                                    baud, setting : ARRAY OF CHAR) : BOOLEAN;

VAR
            file : FileSystem.File;
            i, j : CARDINAL;
BEGIN

        FileSystem.Lookup( file, "NEWSPARM.DAT", FALSE );
        IF (file.res = FileSystem.done) THEN
            FOR i := 0 TO 12 DO
             FileSystem.ReadChar( file, logonid[i] );
             j := ORD(logonid[i]);
             j := (j DIV 2);
             logonid[i] := CHR(j);
             END (* for *);
            FOR i := 0 TO 13 DO
             FileSystem.ReadChar( file, number[i] );
             END (* for *);
            FOR i := 0 TO 43 DO
             FileSystem.ReadChar( file, initstr[i] );
             END (* for *);
            FOR i := 0 TO 3 DO
               FileSystem.ReadChar( file, port[i] );
              END (* for *);
            FOR i := 0 TO 3 DO
               FileSystem.ReadChar( file, baud[i] );
              END (* for *);
            FOR i := 0 TO 2 DO
               FileSystem.ReadChar( file, setting[i] );
             END (* for *);
            FileSystem.Close( file );
          RETURN TRUE;
       ELSE
          RETURN FALSE;
       END (* if *)

END GetModemStuff;

BEGIN

 CursorOff;
 InitString( TelNo );
 InitString( InitStr );
 InitString( LogonId );
 InitString( Port );
 InitString( Baud );
 InitString( Setting );

 MInit := TRUE; ModemChanged := FALSE; PwUpdated := FALSE;

 IF GetModemStuff( TelNo, LogonId, InitStr, Port, Baud, Setting ) THEN
    MInit := FALSE;
   END (* if *);

  ValidKey := TRUE;

  REPEAT

   IF ValidKey THEN
        CLS;
        GotoXY(27,4); WriteString('   GM Noon News Setup');
        GotoXY(27,5); WriteString('------------------------');
        GotoXY(25,8); WriteString('F1) Ä Enter COM Port Number');
        GotoXY(25,10);WriteString('F2) Ä Enter Communication Params');
        GotoXY(25,12);WriteString('F3) Ä Enter Modem Baud Rate');
        GotoXY(25,14);WriteString('F4) Ä Enter Modem Init String');
        GotoXY(25,16);WriteString('F5) Ä Enter Local Telephone Number');
        GotoXY(25,18);WriteString('F6) Ä Enter Logon Name/Password');
        GotoXY(25,21);WriteString('ESC) Ä QUIT Setup');
     END (* if *);

   Read(ch);
   IF ch = nul THEN
   Read(ch);
   CASE ORD(ch) OF

 59  (* F1 *)
         :  CLS;
            Header("Setup your modem's COM port");
            GotoXY(5,10); WriteString('Enter COM port (eg."COM1"):  ');
            IF NOT MInit THEN
              WriteString(Port);
              GotoXY(33,10);
            ELSE
              ConstAssign( "COM1", Port );
              WriteString(Port);
              GotoXY(33,10);
            END (* if *);
            ReadCaps( Port, FALSE );
       IF YesNo1('Is this information correct? ENTER=[Y]es or [N]o: ',3,21) THEN
            ModemChanged := TRUE;
          END (* if *);
         ValidKey := TRUE
         |
 60  (* F2 *)
         : CLS;
            Header("Setup your modem's parameters");
            GotoXY(5,10); WriteString('Enter modem settings (eg."E71"):  ');
            IF NOT MInit THEN
              WriteString(Setting);
              GotoXY(38,10);
            ELSE
              ConstAssign( "E71", Setting );
              WriteString(Setting);
              GotoXY(38,10);
            END (* if *);
            ReadCaps( Setting, FALSE );
       IF YesNo1('Is this information correct? ENTER=[Y]es or [N]o: ',3,21) THEN
            ModemChanged := TRUE;
          END (* if *);
         ValidKey := TRUE
         |
 61  (* F3 *)
         : CLS;
            Header("Setup your modem's baud rate");
            GotoXY(5,10); WriteString('Enter modem baud rate (eg."2400"):  ');
            IF NOT MInit THEN
              WriteString(Baud);
              GotoXY(40,10);
            ELSE
              ConstAssign( "2400", Baud );
              WriteString(Baud);
              GotoXY(40,10);
            END (* if *);
            ReadCaps( Baud, FALSE );
       IF YesNo1('Is this information correct? ENTER=[Y]es or [N]o: ',3,21) THEN
            ModemChanged := TRUE;
          END (* if *);
         ValidKey := TRUE
         |
 62  (* F4 *)
         : CLS;
           Header("Modem Initialization String");
           GotoXY(5,5); WriteString("44 characters are allowed in the string.");
           GotoXY(3,8); WriteString("Enter init string:  ");
           IF NOT MInit THEN
           WriteString(InitStr);
           GotoXY(22,8);
           END (* if *);
           ReadCaps( InitStr, MInit );
     IF YesNo1('Is this information correct? ENTER=[Y]es or [N]o: ',3,21) THEN
           ModemChanged := TRUE;
        END (* if *);
         ValidKey := TRUE
         |
 63  (* F5 *)
         : CLS;
           Header("Change Telemail Telephone Number");
           GotoXY(5,5); WriteString("14 characters are allowed.");
           GotoXY(5,8); WriteString("Enter new number:  ");
           IF NOT MInit THEN
           WriteString(TelNo);
           GotoXY(23,8);
           END (* if *);
           ReadPart( TelNo, MInit );
     IF YesNo1('Is this information correct? ENTER=[Y]es or [N]o: ',3,21) THEN
           ModemChanged := TRUE;
        END (* if *);
         ValidKey := TRUE
         |
 64  (* F6 *)
         : CLS;
           Header("Change Telemail Logon Password");
           GotoXY(5,5); WriteString("Enter password, eg. abcefg");
           GotoXY(5,8); WriteString("Enter new password:  ");
            ReadCaps( TempId, TRUE );
     IF YesNo1('Is this information correct? ENTER=[Y]es or [N]o: ',3,21) THEN
           Assign( TempId, LogonId );
           ModemChanged := TRUE;
           PwUpdated := TRUE;
        END (* if *);
         ValidKey := TRUE;
         |
   ELSE
       ValidKey := FALSE;
       Beep;

 END (* case *);

ELSIF (ORD(ch) # 27) THEN
     ValidKey := FALSE; Beep;

END (* if *);

UNTIL ORD(ch) = 27;

  CLS;
  WriteString('Copyright 1995 Fritz Feuerbacher'); WriteLn;
  WriteString('407-331-9312'); WriteLn;
  WriteString('Telemail A.923423'); WriteLn;
  WriteString('ind01464@pegasus.cc.ucf.edu'); WriteLn;
  WriteString('v.2.0'); WriteLn;

     IF ModemChanged THEN
         FileSystem.Lookup( file, "NEWSPARM.DAT", TRUE );
               FOR I := 0 TO 12 DO
                   j := ORD(LogonId[I]);
                   j := (j*2);
                 FileSystem.WriteChar( file, CHR(j) );
                END (* for *);
              FOR I := 0 TO 13 DO
                 FileSystem.WriteChar( file, TelNo[I] );
                END (* for *);
               FOR I := 0 TO 43 DO
                 FileSystem.WriteChar( file, InitStr[I] );
                END (* for *);
               FOR I := 0 TO 3 DO
                 FileSystem.WriteChar( file, Port[I] );
                END (* for *);
               FOR I := 0 TO 3 DO
                 FileSystem.WriteChar( file, Baud[I] );
                END (* for *);
               FOR I := 0 TO 2 DO
                 FileSystem.WriteChar( file, Setting[I] );
                END (* for *);
        FileSystem.Close( file );
       END (* if *);

CursorOn;

END SetUp.
