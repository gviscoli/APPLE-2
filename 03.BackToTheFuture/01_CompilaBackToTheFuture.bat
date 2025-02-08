@echo Inizio compilazione

@cl65 -t apple2 pch-UthernetCheck.c -o pch-Uth.bin -O -m pch-UthernetCheck.map -vm ../00.LIBRERIE/PCH/pch_network.lib ../00.LIBRERIE/IP65/lib/ip65.lib ../00.LIBRERIE/IP65/lib/ip65_tcp.lib ../00.LIBRERIE/IP65/drivers/ip65_apple2.lib ../00.LIBRERIE/CC65/lib/apple2.lib

@echo Compilazione Terminata

@echo

@echo Copia del file binario all'interno dell'immagine

java -jar C:\UTILITY\AppleCommander\AppleCommander-ac-1.9.0.jar -d build\BackToTheFuture.dsk pch.Uth.bin
java -jar C:\UTILITY\AppleCommander\AppleCommander-ac-1.9.0.jar -as build\BackToTheFuture.dsk pch.Uth.bin < pch-Uth.bin