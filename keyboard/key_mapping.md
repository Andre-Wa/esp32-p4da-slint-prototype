# Teclado USB RP2040 4x12

## Especificações

- RP2040-Zero como placa e microcontrolador;
- CircuitPython e kmk-firmware como software;
- Materiais usados:
    - PCB protoboard para soldar componentes
    - 48 botões 6*6*4.3 mm
    - 48 diodos 1N4148 200mA 100V
- Um layout de teclas 4x12;
- Orientação do diodo: ColumnToRow;
- Organização dos pinos:
    - colunas (GP0 a GP11);
    - linhas (GP15 a GP12).

## Pinos utilizados

```json
[
    "col":{0,1,2,3,4,5,6,7,8,9,10,11},
    "row":{15,14,13,12}
]
```

## Layout do Teclado

Layout de 4 camadas pra um teclado ortolinear 4x12 (48 teclas).

### Camada 0 - Base (QWERTY)

```
┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐
│ Esc │  Q  │  W  │  E  │  R  │  T  │  Y  │  U  │  I  │  O  │  P  │ Bksp│
├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
│ Tab │  A  │  S  │  D  │  F  │  G  │  H  │  J  │  K  │  L  │  ;  │Enter│
├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
│Shift│  Z  │  X  │  C  │  V  │  B  │  N  │  M  │  ,  │  .  │  /  │Shift│
├─────┼─────┼─────┼─────┼─────┼─────┴─────┼─────┼─────┼─────┼─────┼─────┤
│Ctrl │ Gui │ Alt │Lower│Raise│  Espaço   │AltGr│Left │Down │ Up  │Right│
└─────┴─────┴─────┴─────┴─────┴───────────┴─────┴─────┴─────┴─────┴─────┘
```

Setas no canto inferior direito, Tab no lugar do Caps Lock.

### Camada 1 - Lower (números + símbolos)

Ativa segurando a tecla **Lower**.

```
┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐
│  '  │  1  │  2  │  3  │  4  │  5  │  6  │  7  │  8  │  9  │  0  │Bksp │
├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
│ Tab │  !  │  @  │  #  │  $  │  %  │  ^  │  &  │  *  │  (  │  )  │ Del │
├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
│     │  -  │  =  │  [  │  ]  │  \  │  _  │  +  │  {  │  }  │  |  │  ´  │
├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
│     │     │     │     │Adjus│     │     │     │     │     │     │     │
└─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┘
```

### Camada 2 - Raise (função + navegação)

Ativa segurando a tecla **Raise**.

```
┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐
│ F1  │ F2  │ F3  │ F4  │ F5  │ F6  │ F7  │ F8  │ F9  │ F10 │ F11 │ F12 │
├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
│MNext│MPrev│MPlay│VolDn│VolUp│MMute│BriDn│BriUp│Home │PgDn │PgUp │ End │
├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
│     │     │     │     │     │     │     │     │     │     │     │     │
├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
│     │     │     │Adjus│     |     |     │     │     │     │     │     │
└─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┘
```

### Camada 3 - Adjust (Lower + Raise ao mesmo tempo)

Segurar as duas teclas de camada
juntas é um gesto difícil de fazer por acidente, então é seguro colocar
algo mais "perigoso" aqui.

```
┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐
│Reset│     │     │     │     │     │     │     │     │     │     │     │
├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
│     │     │     │     │     │     │     │     │     │     │     │     │
├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
│     │     │     │     │     │     │     │     │     │     │     │     │
├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
│     │     │     │     │     │     │     │     │     │     │     │     │
└─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┘
```

`KC.RESET` só reinicia o software do KMK.

## Ajustando o layout

No arquivo `code.py`, trocar o que
uma tecla faz é editar o `KC.XXX` correspondente e salvar. Lista completa de
keycodes disponíveis:
[github.com/KMKfw/kmk_firmware/blob/main/docs/en/keycodes.md](https://github.com/KMKfw/kmk_firmware/blob/main/docs/en/keycodes.md)

## Links de referência

- Inspirado em [Building a Tiny Mechanical Keyboard](https://youtu.be/eqOzTvWjd7U).
    - [exercising-ingenuity/altoid-tin-cyberdeck](https://github.com/exercising-ingenuity/altoid-tin-cyberdeck)
- [KMKfw/kmk_firmware](https://github.com/KMKfw/kmk_firmware)
- [Referência para a placa](https://www.waveshare.com/wiki/RP2040-Zero)