
;
; Compilazione: ca65 net-utility.s -o net-utility.o
;

.export _init_ip_via_dhcp  ; Esporta la funzione per l'uso in C

.include "../00.LIBRERIE/IP65/inc/net.inc"  ; Assumiamo che net.inc contenga la macro

.proc _init_ip_via_dhcp
    init_ip_via_dhcp  ; Chiama la macro
    rts               ; Ritorna al chiamante
.endproc
