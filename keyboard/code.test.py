"""
code.test.py
Esquema de teste para a matriz 4*12 (RP2040-Zero, GPIO 0 to 15)
"""

import board
import digitalio
import time

test_pins = [
    board.GP0, board.GP1, board.GP2, board.GP3, board.GP4,
    board.GP5, board.GP6, board.GP7, board.GP8, board.GP9,
    board.GP10, board.GP11, board.GP12, board.GP13, board.GP14, board.GP15
]

# Start all pins as Pull-Up
ios = []
for pin in test_pins:
    io = digitalio.DigitalInOut(pin)
    io.direction = digitalio.Direction.INPUT
    io.pull = digitalio.Pull.UP
    ios.append(io)

print("--- Scanner started ---")

while True:
    for i in range(len(ios)):
        ios[i].direction = digitalio.Direction.OUTPUT
        ios[i].value = False

        for j in range(len(ios)):
            if i == j:
                continue
            
            if not ios[j].value:
                pin_a = str(test_pins[i]).replace("board.", "")
                pin_b = str(test_pins[j]).replace("board.", "")
                print(f"Pins: {pin_a} e {pin_b}")
                
                while not ios[j].value:
                    time.sleep(0.1)

        ios[i].direction = digitalio.Direction.INPUT
        ios[i].pull = digitalio.Pull.UP

    time.sleep(0.01)