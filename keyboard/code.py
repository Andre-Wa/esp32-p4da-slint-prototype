"""
code.py - Teclado 4x12 (RP2040-Zero + KMK)

Layout inspirado no "Construindo um Pequeno Teclado Mecânico" e no
projeto Altoid Tin Cyberdeck (github.com/exercising-ingenuity/
altoid-tin-cyberdeck).
"""

import board
from kmk.kmk_keyboard import KMKKeyboard
from kmk.keys import KC
from kmk.scanners import DiodeOrientation
from kmk.modules.layers import Layers
from kmk.extensions.media_keys import MediaKeys

keyboard = KMKKeyboard()

# --- Matriz física ---
# Colunas: GP0 a GP11 (12 colunas)
keyboard.col_pins = (
    board.GP0, board.GP1, board.GP2, board.GP3,
    board.GP4, board.GP5, board.GP6, board.GP7,
    board.GP8, board.GP9, board.GP10, board.GP11,
)
# Linhas: GP15 a GP12, NESSA ordem.
keyboard.row_pins = (board.GP15, board.GP14, board.GP13, board.GP12)
keyboard.diode_orientation = DiodeOrientation.COL2ROW

layers_ext = Layers()
keyboard.modules.append(layers_ext)
keyboard.extensions.append(MediaKeys())

# Atalhos
_______ = KC.TRNS   # tecla "transparente": usa a camada abaixo 
XXXXXXX = KC.NO      # tecla sem função 
LOWER = KC.MO(1)
RAISE = KC.MO(2)
ADJUST = KC.MO(3)

# Referência de teclas em: https://github.com/KMKfw/kmk_firmware/blob/main/docs/en/keycodes.md
keyboard.keymap = [
    # ---------------------------------------------------------------
    # Camada 0 - BASE (QWERTY)
    # ---------------------------------------------------------------
    [
        KC.ESC,  KC.Q,    KC.W,    KC.E,    KC.R,    KC.T,    KC.Y,    KC.U,    KC.I,    KC.O,    KC.P,     KC.BSPC,
        KC.TAB,  KC.A,    KC.S,    KC.D,    KC.F,    KC.G,    KC.H,    KC.J,    KC.K,    KC.L,    KC.SCLN,  KC.ENT,
        KC.LSFT, KC.Z,    KC.X,    KC.C,    KC.V,    KC.B,    KC.N,    KC.M,    KC.COMM, KC.DOT,  KC.SLSH,  KC.RSFT,
        KC.LCTL, KC.LGUI, KC.LALT, LOWER,   RAISE,   KC.SPC,  KC.SPC,  KC.RALT, KC.LEFT, KC.DOWN, KC.UP,    KC.RGHT,
    ],

    # ---------------------------------------------------------------
    # Camada 1 - LOWER (números + símbolos), segurando a tecla Lower
    # ---------------------------------------------------------------
    [
        KC.QUOT, KC.N1,   KC.N2,   KC.N3,   KC.N4,   KC.N5,   KC.N6,   KC.N7,   KC.N8,   KC.N9,   KC.N0,    KC.BSPC,
        KC.TAB,  KC.EXLM, KC.AT,   KC.HASH, KC.DLR,  KC.PERC, KC.CIRC, KC.AMPR, KC.ASTR, KC.LPRN, KC.RPRN,  KC.DEL,
        _______, KC.MINS, KC.EQL,  KC.LBRC, KC.RBRC, KC.BSLS, KC.UNDS, KC.PLUS, KC.LCBR, KC.RCBR, KC.PIPE,  KC.GRV,
        _______, _______, _______, _______, _______, _______, _______, ADJUST,  _______, _______, _______, _______,
    ],

    # ---------------------------------------------------------------
    # Camada 2 - RAISE (funções + navegação), segurando a tecla Raise
    # ---------------------------------------------------------------
    [
        KC.F1,   KC.F2,   KC.F3,   KC.F4,   KC.F5,   KC.F6,   KC.F7,   KC.F8,   KC.F9,   KC.F10,  KC.F11,   KC.F12,
        KC.MNXT, KC.MPRV, KC.MPLY, KC.VOLD, KC.VOLU, KC.MUTE, KC.BRIU, KC.BRID, KC.HOME, KC.PGDN, KC.PGUP,  KC.END,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,  _______,
        _______, _______, _______, ADJUST,  _______, _______, _______, _______, _______, _______, _______,  _______,
    ],

    # ---------------------------------------------------------------
    # Camada 3 - ADJUST (Lower + Raise juntas): funções de hardware
    # ---------------------------------------------------------------
    [
        KC.RESET, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,  XXXXXXX,
        XXXXXXX,  XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,  XXXXXXX,
        XXXXXXX,  XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,  XXXXXXX,
        XXXXXXX,  XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,  XXXXXXX,
    ],
]

if __name__ == '__main__':
    keyboard.go()
