# wxBriscola
Il gioco della briscola a due giocatori scritto in c++ e wxwidgets.
Non ha il multiplayer e supporta il caricamento di mazzi regionali ed arbitrari e il caricamento delle localizzazioni.

Per compilare è necessario impostare le seguenti librerie di wxwidgets:

wxbase

wxmsw core

Il metodo predefinito di compilazione è con meson.

Funziona con gcc, mingw e [visual studio](https://github.com/numerunix/wxbriscola-for-windows), sia con le wxwidgets oldstable (3.0) stable (3.1) che development (3.2).

E' possibile inserire la propria localizzazione scaricando uno dei files .mo, sono estratti con GNU GetText, e traducendoli col blocco note o poedit. Per la localizzazione arbitraria bisogna inserire il file .mo all'interno di una cartella "LC_LOCALES" all'interno di una cartella che ha come nome il nome corto del linguaggio "it" o "it_IT" per italiano, "en_US" per inglese americano, "en_UK" per inglese regno unito o semplicemente "en" per inglese, "es_ES" per spagnolo di spagna o "es" per spagnolo e via discorrendo, tutto all'interno della cartella locale. Il programma è programmato per vedere automaticamente il nuovo file di traduzione e lo mostrerà nel menù.

Per le carte, è possibile usare 42 immagini arbitrarie, oltre a quelle preimpostate, basta creare una cartella di nome arbitrario in "Mazzi" ed inserire 40 immagini con nome da "0" a "40" e due immagini col nome "retro carte mazzo" e "retro carte pc" tutte in formato jpeg.

Il programma le vedrà in automatico e aggiornerà il menù.

Sentitevi liberi di contribuire con localizzazioni e mazzi regionali.

# Gameplay
[![youtube](https://i.ibb.co/YRJwZHm/mq2.jpg)](https://youtu.be/8gezs7pv26o)
[![youtube](https://i.ibb.co/M82pkwY/mq2-1.jpg)](https://youtu.be/dvE60cja1rY)

# Installazione

# The OLD FASCION DEBIAN WAY
Per installare i package Deb disponibili nella sezione release, bisogna usare dpkg passando come parametro i e i nomi dei files da installare.
Verosimilmente

# cd Scaricati

# sudo dpkg -i *.deb

A questo punto bisogna scaricare le librerie wxwidgets necessarie per l'esecuzione

# sudo apt -f install

I package sono universali e vanno bene sia per Ubuntu che per debian.
Sentitevi liberi di incorporarli nei vostri server apt, a patto di mantenere integro il binario, come prevede la licenza GPL.

# Set di mazzi arbitrario
Sono necessari 4 semi, ognuno di 10 carte.
- Bastoni è rappresentato con le immagini jpeg coi numeri da 0 a 9 (0 è 1 di bastoni, 9 è 10 di bastoni, in sequenza)

- Coppe è rappresentato con le immagini jpeg coi numeri da 10 a 19

- Denari è rappresentato con le immagini jpeg da 20 a 29

- Spade è rappresentato con le immagini jpeg da 30 a 39

Sono presenti, in oltre:
- il retro di una singola carta che rappresenta le carte del computer di nome "retro carte pc.jpg"

- il retro della carta del pc girata di 90 gradi chiamata "retro carte mazzo.jpg" che rappresenta il tallone

Queste 42 immagini vanno posizionate in una sottocartella della cartella mazzi presente nella directory di lavoro.
Il programma vedrà la nuova cartella e la aggiungerà automaticamente al menù mazzi.

# Localizzazione
Per localizzare il programma è necessario usare il programma https://poedit.net/download
Una volta installato, scaricare uno qualsiasi dei files .po presenti nella cartella "po" nella rott del repository.
Per prima cosa bisogna rinominare il file usando la dicitura a due caratteri (se prendete il file italiano e volete tradurlo in tedesco, bisogna prendere il file it.po e rinominarlo in de.po), a questo punto aprirlo ed andare in catalogo > proprietà e modificare la proprietà lingua in de.
A questo punto tradurre iogni singola riga nella colonna Traduzione.
Una volta finito bisogna andare in file > compila e compilare il file in mo.
A questo punto bisogna prendere il relativo mo di traduzioni di wxwidgets e metterlo nella stessa cartella del vostro file.

![screen-2022-05-24-18-56-09](https://user-images.githubusercontent.com/49764967/170091585-c946486a-f964-48a3-a4e1-4ebbf012c75b.png)
![screen-2022-05-24-18-55-32](https://user-images.githubusercontent.com/49764967/170091587-6895150e-815f-45c3-a99a-d7924880a406.png)

In alternativa è possibile aprire il file mo con un file di testo, modificare solo il valore delle varie stringhe msgstr e poi mandarmelo facendo il commit affinché io possa cambiare la localizzazione del file con poedit, compilarlo e metterlo ne programma ufficiale.

![Screenshot_20220525-053827](https://user-images.githubusercontent.com/49764967/170174993-e10401e0-cfc3-4bd1-a1f7-c68e442738b5.png)
![Screenshot_20220525-051036](https://user-images.githubusercontent.com/49764967/170175002-016bf07f-d0ae-490a-8267-cb7b391957f0.png)


Potete mandarmi in po di traduzione quando volete, basta fare il commit su questa piattaforma, sarò ben felice di compilarlo e inserirlo nel programma dandovi il credit.

Si ringrazia una certa Alice Victoria conosciuta dalle mie parti per la contribuzione della localizzazione spagnola.
Si ringrazia Francesca Milano per la contribuzione alla localizzazione Francese

Richiedo esplicitamente la localizzazione in tedesco e portoghese.

# Sviluppi futuri
E' opportuno effettuare la derivazione delle classi helper per sfruttare i socket al fine di ottenere un multiplayer alla tetrinet. Questo non può esssere fatto con le wxSocket, perché le wxSocket non sono attualmente in grado di porre limiti alle connessioni, mentre il gioco è a due e quindi è opportuno efffetuare lo sviluppo nativo sia in ambito windows che in ambito GNU/Linux.
Se volete farlo, siete liberi di poterlo sviluppare e di mandarmi i sorgenti come pull request, sarà mia premura mettervi tra gli sviluppatori del programma.
Se, invece, volete produrre traduzioni di qualsiasi genere, siete comunque liberi di mandarmele, sempre facendo la pull request, in questo modo verrete inseriti tra i traduttori del programma

# Problemi di localizzazione
Ho provveduto a localizzare la wxbriscola in italiano, spagnolo e inglese.
Su windows non tutto il programma esce localizzato. I files di localizzazione delle librerie wxwidgets sono inclusi e le chiamate alle librerie di sistema si basano sulla lingua di sistema, per cui se si prova a mettere la lingua spagnola, la schermata del font non sarà tradotta in spagnolo e per averla bisogna cambiare la lingua del sistema operativo.

Su GNU/Linux c'è bisogno del package di localizzazione di wxwidgets (wxwidgets-i18n) ma il programma risulterà tutto correttamente localizzato.

Potrebbe comparire il messaggio d'errore all'inizio "Locale xx_xx cannot be set) ed è perché il sistema non in uso non ha installate tutte le librerie di traduzioni (tipicamente quelle di gnome), però il programma risulterà lo stesso correttamente localizzato.

In alcune occasioni, potrebbe apparire il programma in lingua italiana, solo che al posto di avere gli accenti ha gli apostrofi, questo accade quando non trova la libreria di localizzazione richiesta e quindi mostra le stringhe senza nessuna traduzione.
Per ovviare al problema e vedere quindi correttamente gli accenti italiani, bisogna assicurarsi che il file wxBirscola.mo si trovi nella cartella LC_LOCALES sotto locale nella directory di esecuzione del programma.

# Ricompilazione
Scaricate il file wxBriscola 0.4.1.1 dal [mio sito web](http://numerone.altervista.org), poi prendete tutti i files da git, copiate la cartella Mazzi ed il file debian/rules dalla 0.4.1.1 nelle relative directory della 0.4.2, ed infine date il comando dpkg-buildpackage presente nei package installati da devscripts in ambiente debian.

# Come scaricarla

[![wxbriscola](https://snapcraft.io/wxbriscola/badge.svg)](https://snapcraft.io/wxbriscola)

# Donazioni

[![paypal](https://www.paypalobjects.com/it_IT/IT/i/btn/btn_donateCC_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=H4ZHTFRCETWXG)

Puoi donare anche tramite carta Hype a patto di avere il mio numero di cellulare nella rubrica. Sai dove lo puoi trovare? Sul mio curriculum.
Apri l'app Hype, fai il login, seleziona PAGAMENTI, INVIA DENARO, seleziona il mio numero nella rubrica, imposta l'importo, INSERISCI LA CAUSALE e segui le istruzioni a video.

