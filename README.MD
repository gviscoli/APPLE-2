
![logo](/00.ASSETS/images/apple-ii_logo.jpg)

# Repository per lo sviluppo di programmi Apple 2e

# Link utili

## Tools

cc65 è un pacchetto completo di sviluppo per sistemi 65(C)02, che include un potente assemblatore di macro, un compilatore C, un linker, un librarian e molti altri strumenti: https://cc65.github.io/

## Repository per sviluppo

TBD

### Compilazione

Il seguente comando serve per generare il file .bin che deve essere caricaro all'interno del file immagine con estensione .DSK

<b>cl65</b> <b>-t</b> apple2 helloworld.c <b>-o</b> helloworld.bin <b>-O</b>  <b>-m</b> helloworld.bin.map <b>-vm</b> ../00.IP65/ip65/ip65.lib ../00.IP65/drivers/ip65_apple2.lib

CC65 include configurazioni predefinite per varie piattaforme, come Commodore 64, NES, Apple II, ecc. qui di seguito vengono descritti i parametri dell'esempio precedente:

- <b>[-t apple2]</b> | serve per specificare il target della piattaforma come Apple II.
- <b>[-o helloworld.bin]</b>  | specifica il nome del file di output. In questo caso, il file binario eseguibile sarà helloworld.bin
- <b>[-O]</b>  | abilita le ottimizzazioni del compilatore per migliorare le prestazioni del codice generato
- <b>[-m helloworld.bin.map]</b>  | genera una mappa di memoria durante il processo di linking. Il file helloworld.bin.map contiene informazioni utili per il debug, come la disposizione delle sezioni di memoria
- <b>[-vm]</b>  | abilita messaggi di avviso più dettagliati durante il processo di compilazione e linking