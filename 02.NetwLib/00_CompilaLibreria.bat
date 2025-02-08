@echo Inizio compilazione

@call cc65 -Oirs pch_network.c -o build\pch_network.s
@call ca65 build\pch_network.s -o build\pch_network.o
@call ar65 a build\pch_network.lib build\pch_network.o

@echo Compilazione Terminata

@echo Copia dei files nel repositori comune

@copy pch_network.h ..\00.LIBRERIE\PCH
@copy build\pch_network.lib ..\00.LIBRERIE\PCH

@echo Procedura completata
