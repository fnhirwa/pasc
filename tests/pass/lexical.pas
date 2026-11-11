PROGRAM Lexical(Output);
{ comment closed by the alternative delimiter *)
(* comment closed by the reference delimiter }
VAR Count, COUNT2: Integer;
    Flag: BOOLEAN;
BEGIN
  count := 3; Count2 := count{no separator needed}+1;
  flag := (COUNT = 3) AND (count2 = 4) Or False;
  WriteLn(count2, ' ', FLAG);
END.
