@cls
@echo:
@echo:

@echo Inizio compilazione

@echo   *** Compila: pch-Uth.bin
@REM cl65 -t apple2 pch-UthernetCheck.c -o pch-Uth.bin -O -m pch-UthernetCheck.map -vm ../00.LIBRERIE/PCH/pch_network.lib ../00.LIBRERIE/IP65/lib/ip65.lib ../00.LIBRERIE/IP65/lib/ip65_tcp.lib ../00.LIBRERIE/IP65/drivers/ip65_apple2.lib ../00.LIBRERIE/CC65/lib/apple2.lib
@cl65 -t apple2 pch-UthernetCheck.c -o pch-Uth.bin -O -m pch-UthernetCheck.map -vm ../00.LIBRERIE/PCH/pch_network.lib ../00.LIBRERIE/IP65/lib/ip65_tcp.lib ../00.LIBRERIE/IP65/drivers/ip65_apple2.lib ../00.LIBRERIE/CC65/lib/apple2.lib


@echo   *** Compila: test-client1.bin
@cl65 -t apple2 test-client1.c stream3.c -o test-client1.bin -O -m test-client1.map -vm ../00.LIBRERIE/PCH/pch_network.lib ../00.LIBRERIE/IP65/lib/ip65.lib ../00.LIBRERIE/IP65/lib/ip65_tcp.lib ../00.LIBRERIE/IP65/drivers/ip65_apple2.lib ../00.LIBRERIE/CC65/lib/apple2.lib

@echo Compilazione Terminata

@echo:

@echo Copia del file binario [ pch.Uth.bin come pch.Uth ] all'interno dell'immagine
@java -jar C:\UTILITY\AppleCommander\AppleCommander-ac-1.9.0.jar -d build\BackToTheFuture.dsk pch.Uth
@java -jar C:\UTILITY\AppleCommander\AppleCommander-ac-1.9.0.jar -as build\BackToTheFuture.dsk pch.Uth < pch-Uth.bin

@echo Copia del file binario [ test-client1.bin come test.client1 ] all'interno dell'immagine
@java -jar C:\UTILITY\AppleCommander\AppleCommander-ac-1.9.0.jar -d build\BackToTheFuture.dsk test.client1
@java -jar C:\UTILITY\AppleCommander\AppleCommander-ac-1.9.0.jar -as build\BackToTheFuture.dsk test.client1 < test-client1.bin

@echo:
@echo: