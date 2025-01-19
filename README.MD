
# Sviluppatori appassionati di retrocomputing 

![logo](/00.ASSETS/images/apple-ii_logo.jpg)

## Repository per lo sviluppo di programmi Apple ][
Questo repository Github è stato realizzato per raccogliere e condividere esperienze nello sviluppo di programmi Apple ][

## Tools

<b>CC65</b> (https://cc65.github.io/)<br>
CC65 è una suite completa di strumenti open-source per lo sviluppo di software in linguaggio C e assembly per computer a 8 bit, tra cui Commodore 64, Apple II, NES, Atari 8-bit e molte altre piattaforme retro. È particolarmente apprezzata dagli sviluppatori e dagli appassionati di retrocomputing per la sua capacità di generare codice ottimizzato e compatibile con queste macchine.

<b>AppleCommander</b> (https://applecommander.github.io/)<br>
AppleCommander è uno strumento software multipiattaforma utilizzato per gestire file system e immagini disco dei computer Apple II. È scritto in Java ed è molto popolare tra gli appassionati di retrocomputing per il suo supporto ampio e flessibile dei formati di disco utilizzati dall'Apple II. 

AppleCommander è attualmente in grado di leggere e visualizzare informazioni sulle immagini disco, esportare file di vario tipo, creare immagini disco, importare file in immagini disco e ha un compilatore rudimentale.

<b>AppleWin</b> (https://github.com/AppleWin/AppleWin)<br>
AppleWin è un popolare emulatore per computer Apple II che funziona su sistemi Windows. È uno strumento molto usato dagli appassionati di retrocomputing per emulare i computer Apple II su hardware moderno, permettendo di eseguire vecchi programmi, giochi e sistemi operativi come DOS 3.3 o ProDOS.

## Compilazione

Il seguente comando serve per compilare un sorgente in codice c per generare i seguenti files:

 - helloworld.bin
 - helloworld.bin.map
 - helloworld.o

### Esempio
<b>cl65</b> <b>-t</b> apple2 helloworld.c <b>-o</b> helloworld.bin <b>-O</b>  <b>-m</b> helloworld.bin.map <b>-vm</b> ../00.IP65/ip65/ip65.lib ../00.IP65/drivers/ip65_apple2.lib

CC65 include configurazioni predefinite per varie piattaforme, come Commodore 64, NES, Apple II, ecc. qui di seguito vengono descritti i parametri dell'esempio precedente:

- <b>[-t apple2]</b> | serve per specificare il target della piattaforma come Apple II.
- <b>[-o helloworld.bin]</b>  | specifica il nome del file di output. In questo caso, il file binario eseguibile sarà helloworld.bin
- <b>[-O]</b>  | abilita le ottimizzazioni del compilatore per migliorare le prestazioni del codice generato
- <b>[-m helloworld.bin.map]</b>  | genera una mappa di memoria durante il processo di linking. Il file helloworld.bin.map contiene informazioni utili per il debug, come la disposizione delle sezioni di memoria
- <b>[-vm]</b>  | abilita messaggi di avviso più dettagliati durante il processo di compilazione e linking


## Creazione di file immagine .DSK
La seguente guida descrive la procedura per la creazione di una immagine .DSK a partire dall'immagine base prodos.dsk. prodos.dsk è generalmente un'immagine disco creata per essere utilizzata con computer Apple II che eseguono il sistema operativo ProDOS. ProDOS è un sistema operativo sviluppato da Apple per sostituire DOS 3.3 sui computer Apple II, introducendo miglioramenti significativi come un supporto migliore per dischi rigidi, directory gerarchiche, e maggiore efficienza nella gestione dei file.

1) Copiare l'immagine base in una nuova immagine: <b>copy</b> prodos.dsk hello.dsk

2) Usare il programma <b>AppleCommander</b> per aggiungere al file hello.dsk il programma helloworld.bin

3) Usare l'emulatore <b>AppleWin</b> per caricare l'immagine hello.dsk e testare i programmi sviluppati
