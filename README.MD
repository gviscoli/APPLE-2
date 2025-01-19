
# Repository per lo sviluppo di programmi Apple 2e ![logo](/00.ASSETS/images/apple-ii_logo.jpg)

# Repository per lo sviluppo di programmi Apple 2e
sviluppatori e dagli appassionati di retrocomputing 

## TOOLS

<b>CC65</b> (https://cc65.github.io/)
CC65 è una suite completa di strumenti open-source per lo sviluppo di software in linguaggio C e assembly per computer a 8 bit, tra cui Commodore 64, Apple II, NES, Atari 8-bit e molte altre piattaforme retro. È particolarmente apprezzata dagli sviluppatori e dagli appassionati di retrocomputing per la sua capacità di generare codice ottimizzato e compatibile con queste macchine.

<b>AppleCommander</b> (https://applecommander.github.io/)
AppleCommander è uno strumento software multipiattaforma utilizzato per gestire file system e immagini disco dei computer Apple II. È scritto in Java ed è molto popolare tra gli appassionati di retrocomputing per il suo supporto ampio e flessibile dei formati di disco utilizzati dall'Apple II. 

AppleCommander è attualmente in grado di leggere e visualizzare informazioni sulle immagini disco, esportare file di vario tipo, creare immagini disco, importare file in immagini disco e ha un compilatore rudimentale.


## COMPILAZIONE SORGENTI

Il seguente comando serve per compilare un sorgente in codice c per generare i seguenti files:

 - helloworld.bin
 - helloworld.bin.map
 - helloworld.o

<b>cl65</b> <b>-t</b> apple2 helloworld.c <b>-o</b> helloworld.bin <b>-O</b>  <b>-m</b> helloworld.bin.map <b>-vm</b> ../00.IP65/ip65/ip65.lib ../00.IP65/drivers/ip65_apple2.lib

CC65 include configurazioni predefinite per varie piattaforme, come Commodore 64, NES, Apple II, ecc. qui di seguito vengono descritti i parametri dell'esempio precedente:

- <b>[-t apple2]</b> | serve per specificare il target della piattaforma come Apple II.
- <b>[-o helloworld.bin]</b>  | specifica il nome del file di output. In questo caso, il file binario eseguibile sarà helloworld.bin
- <b>[-O]</b>  | abilita le ottimizzazioni del compilatore per migliorare le prestazioni del codice generato
- <b>[-m helloworld.bin.map]</b>  | genera una mappa di memoria durante il processo di linking. Il file helloworld.bin.map contiene informazioni utili per il debug, come la disposizione delle sezioni di memoria
- <b>[-vm]</b>  | abilita messaggi di avviso più dettagliati durante il processo di compilazione e linking


### CREAZIONE DI  IMMAGINE .DSK PARTENDO DAL PRODOS.DSK