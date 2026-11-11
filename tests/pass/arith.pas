program arith(output);
var a, b: integer;
begin
  writeln(1 + 2 * 3);            { 7: multiplying before adding }
  writeln((1 + 2) * 3);          { 9 }
  writeln(10 - 4 - 3);           { 3: left associative }
  writeln(-2 + 5);               { 3: the sign applies to the first term }
  writeln(-7 mod 3);             { -1: parsed as -(7 mod 3), see 6.7.1 }
  writeln(7 div 2);              { 3 }
  writeln(7 mod 3);              { 1 }
  a := -7;
  writeln(a div 2);              { -3: truncation toward zero }
  writeln(a mod 3);              { 2: the result of mod is never negative }
  writeln(a div (-1));           { 7 }
  a := 17; b := 5;
  writeln(a div b * b + a mod b);  { 17 }
  writeln(maxint);
  writeln(-maxint);
  writeln(maxint - 1 + 1)
end.
