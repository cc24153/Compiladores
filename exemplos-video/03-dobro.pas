program dobro(input, output);
var
    numero: integer;

procedure dobrar(var valor: integer);
var
    calculo: integer;
begin
    calculo := valor * 2;
    valor := calculo
end;

begin
    numero := 6;
    dobrar(numero);
    write(numero)
end.
