#include <all.h>

natl box;

void scrittore(natq a){
    for(natl i = 0; i < 5; i++){
        printf("[scrittore] sending message '%d'\n", i);
        msgbox_send(box, i);
    }
    terminate_p();
}

void lettore(natq i){
    natl msg;
    while(true){
        msg = msgbox_recv(box);
        printf("[lettore %d] received message '%d'\n", (int)i, msg);
    }
    terminate_p();
}


extern "C" void main()
{
    box = msgbox_init();
    flog(LOG_INFO, "[main] allocata msgbox %u", box);

    activate_p(lettore, 0, 40, LIV_UTENTE);
    activate_p(lettore, 1, 40, LIV_UTENTE);
    activate_p(scrittore, 0, 10, LIV_UTENTE);

    terminate_p();
}
