program control(output);
var a, b, t, n, d: integer;
    prime: Boolean;
begin
  { Euclid }
  a := 1071; b := 462;
  while b <> 0 do begin
    t := a mod b; a := b; b := t
  end;
  writeln(a);

  { the else belongs to the nearest if }
  a := 1; b := 2;
  if a = 1 then
    if b = 1 then writeln('inner then')
    else writeln('inner else');

  { repeat runs its body at least once }
  n := 10;
  repeat
    n := n + 1
  until n > 5;
  writeln(n);

  { primes below 30 }
  n := 2;
  while n < 30 do begin
    prime := true;
    d := 2;
    while d * d <= n do begin
      if n mod d = 0 then prime := false;
      d := d + 1
    end;
    if prime then write(n, ' ');
    n := n + 1
  end;
  writeln
end.
