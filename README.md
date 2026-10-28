# mrm-mouse firmware

The STM32G0B1 firmware includes the application functionality from
`mrm-fox/mrm-fox`: USB CDC text commands, classic CAN transmission and reception,
CAN device discovery, lidar and ultrasonic measurements, color measurements and
illumination control, motor control, and the status LED heartbeat.

## USB commands

Connect a serial terminal or RobotBlocks to the USB COM port. Send one command
per line with LF, CR, or CRLF; commands are case-insensitive. Motor numbers and
speeds are decimal; CAN identifiers, payload bytes and change selectors are hex.

| Command | Behavior |
| --- | --- |
| `help` or `?` | List commands. |
| `enable` | Clear speeds and wake the motor drivers. |
| `disable` | Stop all four motors and put the drivers to sleep. |
| `stop` | Stop all motors and cancel a running motor test. |
| `status` | Report driver state and M1 through M4 speeds. |
| `mot <1-4> <-100..100>` | Set one motor's signed speed; requires enable. Other motors retain their speeds. |
| `test` | Run all four motors through 0 to +100 to -100 to 0, then stop (about 20 seconds). Requires enable; stop, disable or a mot command cancels the sweep. |
| `can std <id> [bytes]` | Queue an 11-bit data frame, ID 0..7FF, zero to eight bytes. |
| `can ext <id> [bytes]` | Queue a 29-bit data frame, ID 0..1FFFFFFF, zero to eight bytes. |
| `can status` | Report controller state, bus-off/passive flags, error counters and FIFO space. |
| `can rx on` | Forward all standard and extended data identifiers (default). |
| `can rx off` | Clear pending reports and forward only odd response identifiers. |
| `can rx status` | Report forwarding mode, delivered/dropped/error counts and queued frames. |
| `devices <start-id> <end-id>` | Clear the active device list, probe even standard IDs in the inclusive range and list responding devices. |
| `change <current-id> <selector>` | Send command 40 plus an 8-bit ID selector; current ID 0..7FF, selector 0..FF (0..F for a discovered lidar). |
| `lid <1-16> start` | Start continuous lidar measurements at ID 180 + 2 * (number - 1). |
| `lid <1-16> stop` | Stop measurements at the corresponding lidar ID. |
| `lidar <1-16> distance` | Start the lidar if needed, then return its latest stored distance as `OK LIDAR <number> DISTANCE <value>`. Requires the sensor in the discovered device table; the value is zero until its first measurement arrives. |
| `col illumination <0-3>` | Send command 50 plus the illumination level to every discovered COL sensor, preserving its standard/extended format. |

Example session:

```text
enable
mot 1 25
mot 2 -25
status
stop
disable
can std 123 01 02 ff
can ext 1abcde 10 20
devices 180 19e
lid 1 start
can rx status
```

CAN reports have the format `CAN RX STD 181 3 13 34 12`, terminated with CRLF.
`OK CAN QUEUED` confirms entry into the controller FIFO; it does not confirm a
physical transmission or acknowledgement. CAN FD and remote frames are excluded.
There is no automatic bus-off recovery.

The active table holds up to 16 LID/MOT/REF/COL/ULT devices. Alive reports register
devices; LID and ULT measurements update unsigned little-endian distances, and
COL messages 06..0A update all 14 big-endian color values. These values are
available in `can_devices`; the original raw CAN reports also reach the USB host.
MOT and REF devices are discovered, with their traffic available as raw reports.

## Mouse hardware adaptation

Mouse has four hardware PWM channels on TIM2: motor 1 PA15, motor 2 PB3,
motor 3 PB10, motor 4 PC7. Direction outputs use the corresponding MOTORx_DIR
definitions in `main.h`; PD3 controls driver sleep. The existing 10 kHz PWM
configuration and low output polarity are retained. Drivers start enabled with
zero commanded speeds. Fox's other four motors and DAC current-limit outputs
are not present on this board.

CAN uses PB8 RX / PB9 TX, normal classic mode at 250 kbit/s
(48 MHz / 12 / 16), with automatic retransmission. Actual bus operation requires
the board's CAN transceiver, termination and another active node for acknowledgement.

USB commands and CAN reports are processed in `USBPD_DPM_UserExecute`, because
the generated `USBPD_DPM_Run` owns the main scheduler loop. A bounded
`USBPD_DPM_Poll` step keeps the existing power-delivery state machines running
during command waits, without recursively dispatching USB commands. Existing
board power-delivery callbacks and configuration are retained. The LED shows
one normal, two warning or three error flashes according to `errorCode`.

USB replies use a persistent transmit buffer with a 20 ms bounded wait. CAN
sensor commands also stop retrying a full FIFO after 20 ms. A scan waits another
50 ms for responses while continuing power-delivery polling. USB forwarding
uses a 16-frame queue; slow or disconnected hosts can cause overflow. Commands
must be shorter than 64 characters; the receive ring holds 255 bytes. Pace
commands and wait for replies. Overlong lines are discarded. After receive-ring
overflow, send a line ending before resending commands; queued input is dropped
to avoid executing truncated commands.

## USB-C Power Delivery

The USB-C port is a sink. On receiving the charger's source capabilities, the
firmware requests a fixed 9 V / 3 A profile if offered. It selects the actual
position of that profile, rather than assuming it is PDO 2. PPS/APDO profiles,
higher voltages and 9 V offers below 3 A are not selected. If the target is
unavailable, it requests fixed 5 V at up to 500 mA, bounded by the advertised
source current, with the capability-mismatch flag set.

VBUS is measured on PA5 (`ADC_VBUS`, ADC1 channel 5) through 200 kOhm from VBUS
and 10.2 kOhm to GND. ADC1 uses 12-bit single conversions, 160.5-cycle Common 1
sampling and startup calibration. Firmware also samples the internal reference
to compensate the conversion for VDDA. At 9 V, PA5 should measure about 437 mV.
The fixed sink PDOs are installed in `USBPD_PWR_IF_Init` inside a USER CODE
section, so they survive regeneration. The longer ADC sampling time is saved in
the `.ioc` file as well.

`USBPD_DPM_Is9V3AReady(0)` returns enabled only after the 9 V / 3 A request is
accepted and the policy engine reports sink ready. Detach, hard reset, rejected
requests and disabled PD clear that state. This reports protocol readiness; it
does not implement a physical current limiter or change the motor enable policy.

After flashing, reconnect a PD charger offering fixed 9 V / 3 A and check VBUS
with a meter or USB-PD analyzer. Verify negotiation, 5 V fallback, and reconnect
on the board; compilation and host tests do not verify the physical power path.

## Build and verification

VS Code completion and navigation use the installed STM32Cube clangd server
and `.clangd` points it to `build/Debug/compile_commands.json`. Workspace settings
provide the installed CMake, Ninja and ARM GCC paths, so configuring the Debug
preset refreshes the database even when these tools are absent from VS Code's
PATH. The Microsoft C/C++ IntelliSense engine is disabled in this workspace to
avoid conflicting language servers; its ARM configuration is also provided in
`.vscode/c_cpp_properties.json` if you choose to switch engines later.

Open this directory as the VS Code workspace, select the Debug configure preset
if prompted, and run **Developer: Reload Window** after applying the settings.
The tool paths in `.vscode/settings.json` and `c_cpp_properties.json` match this
computer's installed versions; update them if you move or upgrade the tools.

With the STM32 ARM compiler, CMake and Ninja on PATH:

```text
cmake --preset Debug
cmake --build --preset Debug
```

Output: `build/Debug/mrm-mouse.elf`.

From a Visual Studio x64 Native Tools command prompt:

```text
Tests\run_host_tests.cmd
```

Six native test programs cover PD profile selection and VBUS conversion,
CAN validation and RX buffering, sensor values,
USB buffer ownership and disconnects, command routing and overflow recovery,
bounded waits and background polling, and all four PWM channels including the
finite motor sweep. Tests use mocked peripherals and do not access hardware.

The Debug firmware compiled and linked successfully. The firmware has not been
flashed; motor direction/polarity, physical CAN traffic, USB enumeration and
power-delivery behavior need verification on the board.

When regenerating with CubeMX, retain the custom CMake source list and the
`USBPD_DPM_Poll` addition/refactor in `USBPD/App/usbpd_dpm_core.c`, which lies
outside generated USER CODE sections. Main, CDC and DPM callback hooks are in
USER CODE sections.
