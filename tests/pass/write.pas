program wr(output);
var c: char; ok: Boolean; w: integer;
begin
  writeln('[', 42:6, ']');         { right-justified }
  writeln('[', 12345:2, ']');      { a short width is widened }
  writeln('[', -7:4, ']');
  writeln('[', 'abc':5, ']');
  writeln('[', 'abcdef':3, ']');   { a string is truncated }
  c := 'x';
  writeln('[', c, ']', '[', c:3, ']');
  ok := 3 < 4;
  writeln('[', ok, ']', '[', not ok, ']', '[', ok:2, ']');
  writeln('it''s');                { apostrophe-image }
  w := 4;
  writeln(1:w, 2:w + 1);
  write(output, 'explicit file');
  writeln(output);
  writeln
end.
