@echo off
setlocal
cd /d "%~dp0.."
if not exist build mkdir build
cl /nologo /W3 /D_CRT_SECURE_NO_WARNINGS /ITests\can_bus\mocks /ICore\Inc Core\Src\can_bus.c Core\Src\can_devices.c Tests\can_bus\test_can_bus.c /Fe:build\can_bus_tests.exe /Fo:build\ || exit /b 1
build\can_bus_tests.exe || exit /b 1
cl /nologo /W3 /D_CRT_SECURE_NO_WARNINGS /ICore\Inc Core\Src\can_devices.c Tests\can_bus\test_can_devices.c /Fe:build\can_devices_tests.exe /Fo:build\ || exit /b 1
build\can_devices_tests.exe || exit /b 1
cl /nologo /W3 /D_CRT_SECURE_NO_WARNINGS /ITests\usb_bridge\mocks /ITests\can_bus\mocks /ICore\Inc Core\Src\app_usb_bridge.c Core\Src\can_devices.c Tests\usb_bridge\test_usb_bridge.c /Fe:build\usb_bridge_tests.exe /Fo:build\ || exit /b 1
build\usb_bridge_tests.exe || exit /b 1
cl /nologo /W3 /D_CRT_SECURE_NO_WARNINGS /ITests\can_bus\mocks /ICore\Inc Core\Src\usb_commands.c Core\Src\can_devices.c Tests\can_bus\test_usb_commands.c /Fe:build\usb_commands_tests.exe /Fo:build\ || exit /b 1
build\usb_commands_tests.exe || exit /b 1
cl /nologo /W3 /D_CRT_SECURE_NO_WARNINGS /ITests\motor_control\mocks /ICore\Inc Core\Src\motor_control.c Tests\motor_control\test_motor_control.c /Fe:build\motor_control_tests.exe /Fo:build\ || exit /b 1
build\motor_control_tests.exe || exit /b 1
