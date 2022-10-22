project('wxbriscola', 'cpp', version: '0.5')
subdir('po')
wx_dep = dependency('wxwidgets', version : '>=3.0.0', required : true)
executable('wxbriscola', ['BriscoApp.cpp','BriscoFrame.cpp','BriscoPanel.cpp', 'carta.cpp', 'CartaAltaFrame.cpp', 'DoubleValidator.cpp', 'elaboratoreCarteBriscola.cpp', 'giocatore.cpp', 'giocatoreHelperCpu.cpp', 'giocatoreHelperUtente.cpp', 'IntValidator.cpp', 'mazzo.cpp', 'OpzioniFrame.cpp'], dependencies : [wx_dep], install: true)

subdir('Mazzi')




