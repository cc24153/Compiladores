program contagem(input, output);
var
    n: integer;
begin
    n := 3;
    while n > 0 do
    begin
        if n = 2 then
            write(20)
        else
            write(n);
        n := n - 1
    end
end.
