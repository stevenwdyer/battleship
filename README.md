# Battleship

This project is a scaled-down recreation of the strategy game titled "Battleship". It utilizes two Arduinos that communicate with each other via UART, 3x3 LED matrices for grid visualization, joysticks for ship/shot selection, buttons for ship/shot confirmation, passive buzzers for audio cues, and 7-segment for score/state tracking.

<img src="https://i.imgur.com/STr2Olo.png" height="80%" width="80%" alt="Disk Sanitization Steps"/>

<img src="https://i.imgur.com/DaJOfOU.png" height="80%" width="80%" alt="Disk Sanitization Steps"/>

<img src="https://i.imgur.com/iRiCjK2.png" height="80%" width="80%" alt="Disk Sanitization Steps"/>

## Design

<img src="https://i.imgur.com/DbFibRV.png" height="80%" width="80%" alt="Disk Sanitization Steps"/>

<img src="https://i.imgur.com/ij30NSP.png" height="80%" width="80%" alt="Disk Sanitization Steps"/>

## Control Flow

Unlike the classic game of Battleship, this scaled-down implementation uses one 3x3 grid consisting of LEDs for both ship selection as well as shot targeting. Each player will interact with a joystick to select an LED on the grid and a button to confirm their choice of LED. This process will be the same for both ship selection as well as shot targeting. The game is in a best-of-3 format, with the first player to reach 2 hits winning.

To begin the game, both players are initially in the ship selection phase and can select a singular LED as the location of their ship; if one player selects before the other, they will have to wait until both ships are selected. Once both players have selected a ship, one Arduino, designated by the isPlayerOne flag in the program, will move into the shooting state, while the other moves into the idle state, where it will wait for a message from the other Arduino.

When a player is in the shooting phase, there are two options: selecting a shot or resetting the game. If the player chooses to reset the game, achieved by holding down the button for 3 seconds, a message will be sent to the other Arduino, requesting a reset. If a confirmation is sent back, both players will move back into the simultaneous ship selection phase with any previous scores reset to 0.

If a player chooses a target LED to shoot, on button confirmation, the target information will be sent to the other Arduino, where it will return a HIT or MISS code. If a MISS code is returned, the other Arduino moves into the shooting phase while the shooter switches to idle. If a HIT command is returned, the Arduino that returns this command will briefly wait for a GAME WON response from the shooting Arduino, which will send that message if their score reaches 2 from the previous hit. If GAME WON is received by the other Arduino, it will reset, with the other player resetting as well, because their score has reached 2 points, prompting a new game cycle.

If the Arduino receives a HIT command, and the GAME WON signal is not sent back, the game continues with both players moving back into the ship selection phase o choose a new ship location. The player who got the hit will have a winner’s advantage, shooting first again when the next round begins.

## Documentation
File | Contents
---- | --------
`datasheets/000036 - datasheet KY-023-Joy-IT.pdf` | Joystick datasheet.
`datasheets/0900766b80c0c368.pdf.pdf` | 74HC32 quad 2- input OR gate datasheet.
`datasheets/1811151426_Wuxi-ARK-Tech-Elec-SM471801N_C193718.pdf` | 7-segment display datasheet.
`datasheets/Button.pdf` | Button datasheet.
`datasheets/LEDRGB-L-154A4SURK.pdf` | LED datasheet.
`datasheets/Passive buzzer.pdf` | Passive buzzer datasheet.
`datasheets/TC74HC04AF_ToshibaSemiconductor.pdf` | 74HC04 hex inverter datasheet.
`datasheets/TC74HC08AF_ToshibaSemiconductor.pdf` | 74HC08 quad 2-input AND gate datasheet.
`schematics/3x3 LED Matrix Wiring.png` | 3x3 LED matrix wiring.
`schematics/Arduino, Buzzer, Joystick, and Button Wiring.png` | Arduino, buzzer, joystick, and button wiring.


