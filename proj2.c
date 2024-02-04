#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <limits.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <stdbool.h>
#include <string.h>
#include <sys/ipc.h>
#include <stdint.h>
#include <sys/user.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>


int *otevreno;  // otevreni posty
int *pocetZ;    // pocet zbyvajicich zakazniku
int *NZ;        // pocet zakazniku
int *NU;        // pocet uredniku
int *TZ;        // max cas (ms) cekani zakaznika, nez vejde na postu 0 <= TZ <=10000
int *TU;        // max cas (ms) prestavky urednika 0 <= TU <= 100
int *F;         // max cas (ms) uzavreni posty pro nove prichozi 0 <= F <= 10000


int *prepazka_1; //obsazeni prepazky 1
int *prepazka_2; //obsazeni prepazky 2
int *prepazka_3; //obsazeni prepazky 3

int *operace;   // cislo operace

sem_t *sem_Ucall_1; // urednik vola zakaznika, prepazka 1
sem_t *sem_Ucall_2; // urednik vola zakaznika, prepazka 2
sem_t *sem_Ucall_3; // urednik vola zakaznika, prepazka 3

sem_t *sem_Zcall_1; // zakaznik vola dalsiho zakaznika k prepazce 1 (u prepazky stoji vzdy jen 1)
sem_t *sem_Zcall_2; // zakaznik vola dalsiho zakaznika k prepazce 2 (u prepazky stoji vzdy jen 1)
sem_t *sem_Zcall_3; // zakaznik vola dalsiho zakaznika k prepazce 3 (u prepazky stoji vzdy jen 1)

sem_t *sem_synch_1; // nejdrive je zakaznik zavolan (called) pak urednik obslouzi (serving), prepazka 1
sem_t *sem_synch_2; // nejdrive je zakaznik zavolan (called) pak urednik obslouzi (serving), prepazka 2
sem_t *sem_synch_3; // nejdrive je zakaznik zavolan (called) pak urednik obslouzi (serving), prepazka 3

sem_t *sem_operace; // synchronizovany vypis

sem_t *sem_otevreno; // kontrola zda uz je posta zavrena, aby zakaznik nemohl vstoupit

FILE *output; // vystupni soubor proj2.out


void fce_zakaznik(int *idZ, sem_t *sem_zakaznikid) // proces zakaznik
{
    srand(time(NULL));
    int wait_time;
    int id;
    int service_type;

    // prichod, udeleni id, vyber servisu
    sem_wait(sem_zakaznikid);
    *pocetZ = *pocetZ+1;
    *idZ = *idZ+1;
    id = *idZ; // originalni id

    // synchronizovany vypis operace
    sem_wait(sem_operace);
    setvbuf(output, NULL, _IONBF, 0); // nastavi fprintf aby zapisoval do souboru okamzite
    fprintf(output, "%d: Z %d: started\n", *operace, *idZ); 
    *operace = *operace+1;
    sem_post(sem_operace);

    service_type = rand() % 3 + 1; // vyber servisu
    
    sem_post(sem_zakaznikid);
    wait_time = rand() % (*TZ+1);
    usleep(wait_time*1000);

    // prichod, udeleni id, vyber servisu

    

    // zarazeni se do rady
        switch (service_type)
        {
        case 1:
            
            sem_wait(sem_otevreno); // vstup do kriticke sekce => zde je jen jeden proces
            if(*otevreno == 0) // kontrola zda je posta otevrena 
            {         
                *pocetZ = *pocetZ-1;     
                sem_wait(sem_operace);
                setvbuf(output, NULL, _IONBF, 0); // nastavi fprintf aby zapisoval do souboru okamzite
                fprintf(output, "%d: Z %d: going home\n", *operace, id);
                *operace = *operace+1;
                sem_post(sem_operace);  
                sem_post(sem_otevreno); // pusti dalsi proces
                return; 
            }
            
            sem_wait(sem_Zcall_1); // pouze jeden proces 
            // synchronizovany vypis operace
            sem_wait(sem_operace);
            setvbuf(output, NULL, _IONBF, 0); // nastavi fprintf aby zapisoval do souboru okamzite
            fprintf(output, "%d: Z %d: entering office for a service %d\n", *operace, id, service_type);
            *operace = *operace+1;
            sem_post(sem_operace); 

            sem_post(sem_otevreno); // zajisteni aby vypis probehl pred vypisem closing

            sem_wait(sem_Ucall_1); // zakaznik ceka na zavolani
        
            // synchronizovany vypis operace
            sem_wait(sem_operace);
            setvbuf(output, NULL, _IONBF, 0); // nastavi fprintf aby zapisoval do souboru okamzite
            fprintf(output, "%d: Z %d: called by office worker\n", *operace, id); 
            *operace = *operace+1;
            sem_post(sem_operace);

            sem_post(sem_synch_1); // spravne poradi operaci urednika a zakaznika => 1. called 2. serving

            sem_post(sem_Zcall_1); // dalsi proces
            wait_time = rand() % 11;
            usleep(wait_time*1000);
            break;
        case 2:
            // stejny princip jako u case 1
            
            sem_wait(sem_otevreno); 
            if(*otevreno == 0)
            {     
                *pocetZ = *pocetZ-1;     
                sem_wait(sem_operace);
                setvbuf(output, NULL, _IONBF, 0); // nastavi fprintf aby zapisoval do souboru okamzite
                fprintf(output, "%d: Z %d: going home\n", *operace, id);
                *operace = *operace+1;
                sem_post(sem_operace);
                sem_post(sem_otevreno);
                return;
            }
            

            // stejny princip jako u case 1
            sem_wait(sem_Zcall_2);
        
        
            sem_wait(sem_operace);
            setvbuf(output, NULL, _IONBF, 0); // nastavi fprintf aby zapisoval do souboru okamzite
            fprintf(output, "%d: Z %d: entering office for a service %d\n", *operace, id, service_type);
            *operace = *operace+1;
            sem_post(sem_operace);

            sem_post(sem_otevreno); 


            sem_wait(sem_Ucall_2); 

            // synchronizovany vypis operace
            sem_wait(sem_operace);
            setvbuf(output, NULL, _IONBF, 0); // nastavi fprintf aby zapisoval do souboru okamzite
            fprintf(output, "%d: Z %d: called by office worker\n", *operace, id); 
            *operace = *operace+1;
            sem_post(sem_operace);


            sem_post(sem_synch_2); 

            sem_post(sem_Zcall_2);
            wait_time = rand() % 11;
            usleep(wait_time*1000);

            break;
        case 3:
        // stejny princip jako u case 1

            sem_wait(sem_otevreno); 
            if(*otevreno == 0)
            {                
                *pocetZ = *pocetZ-1;
                sem_wait(sem_operace);
                setvbuf(output, NULL, _IONBF, 0); // nastavi fprintf aby zapisoval do souboru okamzite
                fprintf(output, "%d: Z %d: going home\n", *operace, id);
                *operace = *operace+1;
                sem_post(sem_operace);
                sem_post(sem_otevreno);
                return;
            }
            

            // stejny princip jako u case 1
            sem_wait(sem_Zcall_3);
        
            sem_wait(sem_operace);
            setvbuf(output, NULL, _IONBF, 0); // nastavi fprintf aby zapisoval do souboru okamzite
            fprintf(output, "%d: Z %d: entering office for a service %d\n", *operace, id, service_type);
            *operace = *operace+1;
            sem_post(sem_operace);

            sem_post(sem_otevreno); 

            sem_wait(sem_Ucall_3);

            // synchronizovany vypis operace
            sem_wait(sem_operace);
            setvbuf(output, NULL, _IONBF, 0); // nastavi fprintf aby zapisoval do souboru okamzite
            fprintf(output, "%d: Z %d: called by office worker\n", *operace, id); 
            *operace = *operace+1;
            sem_post(sem_operace);

            sem_post(sem_synch_3);

            sem_post(sem_Zcall_3);
            wait_time = rand() % 11;
            usleep(wait_time*1000);
        
            break;
        default:
            break;
        }
    // zakaznik konci
    *pocetZ = *pocetZ-1;
    sem_wait(sem_operace);
    setvbuf(output, NULL, _IONBF, 0); // nastavi fprintf aby zapisoval do souboru okamzite
    fprintf(output, "%d: Z %d: going home\n", *operace, id);
    *operace = *operace+1;
    sem_post(sem_operace);
}




void fce_urednik(int *idU, sem_t *sem_urednikid, sem_t *sem_prepazka)
{
    srand(time(NULL));
    int id;
    int sem_value_1;
    int sem_value_2;
    // prichod, udeleni id
    sem_wait(sem_urednikid);
    *idU = *idU+1;
    id = *idU; // originalni id

    // synchronizovany vypis operace
    sem_wait(sem_operace);
    setvbuf(output, NULL, _IONBF, 0); // nastavi fprintf aby zapisoval do souboru okamzite
    fprintf(output, "%d: U %d: started\n", *operace, *idU);
    *operace = *operace+1;
    sem_post(sem_operace);

    sem_post(sem_urednikid); 
    // prichod, udeleni id

    int obsluhovana_prepazka=0;
    int wait_time;
    while(*pocetZ > 0 || *otevreno == 1) // bezi dokud jsou na poste zakaznici, bezi kdyz je posta otevrena, ale zakaznici jeste nedorazili
    {
        sem_wait(sem_prepazka); // prepazku vybira vzdy jen 1 urednik
        // nahodny vyber z volnych (prepazka_n==0) prepazek 
        if(*prepazka_1 == 0 && *prepazka_2 == 0 && *prepazka_3 == 0)
        {
            obsluhovana_prepazka = rand() % 3;
            obsluhovana_prepazka++;
        }
        else if(*prepazka_1 == 0 && *prepazka_2 == 0 && *prepazka_3 == 1)
        {
            obsluhovana_prepazka = rand() % 2;
            obsluhovana_prepazka++;
        }
        else if(*prepazka_1 == 0 && *prepazka_2 == 1 && *prepazka_3 == 0)
        {
            obsluhovana_prepazka = rand() % 2;
            obsluhovana_prepazka = obsluhovana_prepazka*2+1;
        }
        else if(*prepazka_1 == 0 && *prepazka_2 == 1 && *prepazka_3 == 1)
        {
            obsluhovana_prepazka = 1;
        }
        else if(*prepazka_1 == 1 && *prepazka_2 == 0 && *prepazka_3 == 0)
        {
            obsluhovana_prepazka = rand() % 2;
            obsluhovana_prepazka = obsluhovana_prepazka+2;
        }
        else if(*prepazka_1 == 1 && *prepazka_2 == 0 && *prepazka_3 == 1)
        {
            obsluhovana_prepazka = 2;
        }
        else if(*prepazka_1 == 1 && *prepazka_2 == 1 && *prepazka_3 == 0)
        {
            obsluhovana_prepazka = 3;
        }
        // obsazeni prepazky
        switch(obsluhovana_prepazka)
        {
            case 1:
                *prepazka_1 = 1; // prepazka 1 je obsazena
                break;
            case 2:
                *prepazka_2 = 1; // prepazka 2 je obsazena
                break;
            case 3:
                *prepazka_3 = 1; // prepazka 3 je obsazena
                break;
            default:
                break;
        }

        sem_post(sem_prepazka);

        switch(obsluhovana_prepazka)
        {
        case 1:
            sem_getvalue(sem_Ucall_1, &sem_value_1); 
            sem_getvalue(sem_Zcall_1, &sem_value_2);
            if(sem_value_1 == 0 && sem_value_2 ==0) // urednik zjisti, jestli nekdo stoji ve fronte 1
            {
                sem_post(sem_Ucall_1); // zavola zakaznika
                sem_wait(sem_synch_1); // spravne poradi operaci urednika a zakaznika => 1. called 2. serving

                // synchronizovany vypis operace
                sem_wait(sem_operace);
                setvbuf(output, NULL, _IONBF, 0); // nastavi fprintf aby zapisoval do souboru okamzite
                fprintf(output, "%d: U %d: serving a service of type 1\n", *operace, id);
                *operace = *operace+1;
                sem_post(sem_operace);


                wait_time = rand() % 11;
                usleep(wait_time*1000);
                
                // synchronizovany vypis operace
                sem_wait(sem_operace);
                setvbuf(output, NULL, _IONBF, 0); // nastavi fprintf aby zapisoval do souboru okamzite
                fprintf(output, "%d: U %d: service finished\n", *operace, id);
                *operace = *operace+1;
                sem_post(sem_operace);

                *prepazka_1 = 0; // prepazka 1 se uvolni
            }
            else // nikdo ve fronte nestoji => urednik si da pauzu
            {
                *prepazka_1 = 0; // prepazka 1 se uvolni
                
                // synchronizovany vypis operace
                sem_wait(sem_operace);
                setvbuf(output, NULL, _IONBF, 0); // nastavi fprintf aby zapisoval do souboru okamzite
                fprintf(output, "%d: U %d: taking break\n", *operace, id);
                *operace = *operace+1;
                sem_post(sem_operace);

                wait_time = rand() % (*TU+1);
                usleep(wait_time*1000);

                // synchronizovany vypis operace
                sem_wait(sem_operace);
                setvbuf(output, NULL, _IONBF, 0); // nastavi fprintf aby zapisoval do souboru okamzite
                fprintf(output, "%d: U %d: break finished\n", *operace, id);
                *operace = *operace+1;
                sem_post(sem_operace);

            }
            break;
        case 2:
            // stejny princip jako u case 1
            sem_getvalue(sem_Ucall_2, &sem_value_1);
            sem_getvalue(sem_Zcall_2, &sem_value_2);
            if(sem_value_1 == 0 && sem_value_2 ==0) 
            {
                sem_post(sem_Ucall_2);
                sem_wait(sem_synch_2);

                sem_wait(sem_operace);
                setvbuf(output, NULL, _IONBF, 0); // nastavi fprintf aby zapisoval do souboru okamzite
                fprintf(output, "%d: U %d: serving a service of type 2\n", *operace, id);
                *operace = *operace+1;
                sem_post(sem_operace);

                wait_time = rand() % 11;
                usleep(wait_time*1000);

                sem_wait(sem_operace);
                setvbuf(output, NULL, _IONBF, 0); // nastavi fprintf aby zapisoval do souboru okamzite
                fprintf(output, "%d: U %d: service finished\n", *operace, id);
                *operace = *operace+1;
                sem_post(sem_operace);

                *prepazka_2 = 0; 
            }
            else 
            {
                *prepazka_2 = 0; 

                sem_wait(sem_operace);
                setvbuf(output, NULL, _IONBF, 0); // nastavi fprintf aby zapisoval do souboru okamzite
                fprintf(output, "%d: U %d: taking break\n", *operace, id);
                *operace = *operace+1;
                sem_post(sem_operace);

                wait_time = rand() % (*TU+1);
                usleep(wait_time*1000);

                sem_wait(sem_operace);
                setvbuf(output, NULL, _IONBF, 0); // nastavi fprintf aby zapisoval do souboru okamzite
                fprintf(output, "%d: U %d: break finished\n", *operace, id);
                *operace = *operace+1;
                sem_post(sem_operace);
            }
            break;
        case 3:
            // stejny princip jako u case 1
            sem_getvalue(sem_Ucall_3, &sem_value_1);
            sem_getvalue(sem_Zcall_3, &sem_value_2);
            if(sem_value_1 == 0 && sem_value_2 ==0) 
            {
                sem_post(sem_Ucall_3);
                sem_wait(sem_synch_3);

                sem_wait(sem_operace);
                setvbuf(output, NULL, _IONBF, 0); // nastavi fprintf aby zapisoval do souboru okamzite
                fprintf(output, "%d: U %d: serving a service of type 3\n", *operace, id);
                *operace = *operace+1;
                sem_post(sem_operace);


                wait_time = rand() % 11;
                usleep(wait_time*1000);

                sem_wait(sem_operace);
                setvbuf(output, NULL, _IONBF, 0); // nastavi fprintf aby zapisoval do souboru okamzite
                fprintf(output, "%d: U %d: service finished\n", *operace, id);
                *operace = *operace+1;
                sem_post(sem_operace);

                *prepazka_3 = 0; 
            }
            else 
            {
                *prepazka_3 = 0; 

                sem_wait(sem_operace);
                setvbuf(output, NULL, _IONBF, 0); // nastavi fprintf aby zapisoval do souboru okamzite
                fprintf(output, "%d: U %d: taking break\n", *operace, id);
                *operace = *operace+1;
                sem_post(sem_operace);

                wait_time = rand() % (*TU+1);
                usleep(wait_time*1000);

                sem_wait(sem_operace);
                setvbuf(output, NULL, _IONBF, 0); // nastavi fprintf aby zapisoval do souboru okamzite
                fprintf(output, "%d: U %d: break finished\n", *operace, id);
                *operace = *operace+1;
                sem_post(sem_operace);

            }
            break;
        
        default: // vsechny prepazky jsou plne => urednik si da pauzu

            sem_wait(sem_operace);
            setvbuf(output, NULL, _IONBF, 0); // nastavi fprintf aby zapisoval do souboru okamzite
            fprintf(output, "%d: U %d: taking break\n", *operace, id);
            *operace = *operace+1;
            sem_post(sem_operace);

            wait_time = rand() % (*TU+1);
            usleep(wait_time*1000);

            sem_wait(sem_operace);
            setvbuf(output, NULL, _IONBF, 0); // nastavi fprintf aby zapisoval do souboru okamzite
            fprintf(output, "%d: U %d: break finished\n", *operace, id);
            *operace = *operace+1;
            sem_post(sem_operace);

            break;
        }



    }
    // urednik konci
    sem_wait(sem_operace);
    setvbuf(output, NULL, _IONBF, 0); // nastavi fprintf aby zapisoval do souboru okamzite
    fprintf(output, "%d: U %d: going home\n", *operace, id);
    *operace = *operace+1;
    sem_post(sem_operace);
}

bool isNumber(char number[]) // overeni jestli je vyraz cislo
{
    int i = 0;

    for (; number[i] != 0; i++)
    {
        if (!isdigit(number[i]))
            return false;
    }
    return true;
}



bool check_args(int argc, char *argv[]) // funkce na overeni spravnosti argumetu
{
    int tmp; // pomocna promenna
    if (argc != 6) 
    {
        fprintf(stderr, "Wrong number of arguments\n");
        return false; // spatny pocet argumentu
    }

    if(!isNumber(argv[1]))
    {
        fprintf(stderr, "Argument 1 is not a positive integer\n");
        return false; // neni ciselna hodnota
    }

    if(!isNumber(argv[2]))
    {
        fprintf(stderr, "Argument 2 is not a positive integer\n");
        return false; // neni ciselna hodnota
    }
    else
    {
        tmp = atoi(argv[2]);
        if(tmp == 0)
        {
            fprintf(stderr, "Argument 2 is 0\n");
            return false; // nulovy pocet uredniku nedava smysl, zakazniky nelze obslouzit
        }
    }

    if(!isNumber(argv[3]))
    {
        fprintf(stderr, "Argument 3 is not a positive integer\n");
        return false; // neni ciselna hodnota
    }
    else
    {
        tmp = atoi(argv[3]);
        if(tmp < 0 || tmp > 10000) // 0 <= TZ <= 10000
        {
            fprintf(stderr, "Value of argument 3 is not in interval <0; 10000>\n");
            return false; // spatny rozsah
        }
    }

    if(!isNumber(argv[4]))
    {
        fprintf(stderr, "Argument 4 is not a positive integer\n");
        return false; // neni ciselna hodnota
    }
    else
    {
        tmp = atoi(argv[4]);
        if(tmp < 0 || tmp > 100) // 0 <= TU <= 100
        {
            fprintf(stderr, "Value of argument 4 is not in interval <0; 100>\n");
            return false; // spatny rozsah
        }
    }

    if(!isNumber(argv[5]))
    {
        fprintf(stderr, "Argument 5 is not a positive integer\n");
        return false; // neni ciselna hodnota
    }
    else
    {
        tmp = atoi(argv[5]);
        if(tmp < 0 || tmp > 10000) // 0 <= F <= 10000
        {
            fprintf(stderr, "Value of argument 5 is not in interval <0; 10000>\n");
            return false; // spatny rozsah
        }
    }
    return true;
}

void dealokace(sem_t *sem_prepazka, sem_t *sem_urednikid, sem_t *sem_zakaznikid, int *idZ, int *idU)
{
    // dealokace sdilene pameti
    sem_destroy(sem_otevreno);
    munmap(sem_otevreno, sizeof(sem_t));

    sem_destroy(sem_operace);
    munmap(sem_operace, sizeof(sem_t));

    sem_destroy(sem_synch_1);
    munmap(sem_synch_1, sizeof(sem_t));

    sem_destroy(sem_synch_2);
    munmap(sem_synch_2, sizeof(sem_t));

    sem_destroy(sem_synch_3);
    munmap(sem_synch_3, sizeof(sem_t));

    sem_destroy(sem_prepazka);
    munmap(sem_prepazka, sizeof(sem_t));

    sem_destroy(sem_Zcall_3);
    munmap(sem_Zcall_3, sizeof(sem_t));

    sem_destroy(sem_Ucall_3);
    munmap(sem_Ucall_3, sizeof(sem_t));

    sem_destroy(sem_Zcall_2);
    munmap(sem_Zcall_2, sizeof(sem_t));

    sem_destroy(sem_Ucall_2);
    munmap(sem_Ucall_2, sizeof(sem_t));

    sem_destroy(sem_Zcall_1);
    munmap(sem_Zcall_1, sizeof(sem_t));

    sem_destroy(sem_Ucall_1);
    munmap(sem_Ucall_1, sizeof(sem_t));

    sem_destroy(sem_urednikid);
    munmap(sem_urednikid, sizeof(sem_t));

    sem_destroy(sem_zakaznikid);
    munmap(sem_zakaznikid, sizeof(sem_t));

    munmap(operace, sizeof(int));
    munmap(prepazka_3, sizeof(int));
    munmap(prepazka_2, sizeof(int));
    munmap(prepazka_1, sizeof(int));
    munmap(pocetZ, sizeof(int));
    munmap(otevreno, sizeof(int));
    munmap(idZ, sizeof(int));
    munmap(idU, sizeof(int));
    munmap(NZ, sizeof(int));
    munmap(NU, sizeof(int));
    munmap(TZ, sizeof(int));
    munmap(TU, sizeof(int));
    munmap(F, sizeof(int));
    // dealokace sdilene pameti
    fclose(output);
}

int main(int argc, char *argv[])
{
    // checking args
    if(!(check_args(argc, argv))) // sprane agrumenty => return true, proto negace
    {
        exit(1);
    }
    // checking args


    output = fopen("proj2.out", "w");



    srand(time(NULL)); // kvuli nahodnosti cisel

    // alokace sdilene pameti
    // promenne
    NZ = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *NZ = atoi(argv[1]);

    NU = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *NU = atoi(argv[2]);

    TZ = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *TZ = atoi(argv[3]);

    TU = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *TU = atoi(argv[4]);

    F = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *F = atoi(argv[5]);

    int *idZ = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0); // id zakaznika
    *idZ = 0;
    int *idU = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0); // id urednika
    *idU = 0;
    otevreno = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *otevreno = 1;
    pocetZ = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *pocetZ = 0;

   

    prepazka_1 = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *prepazka_1 = 0;
    prepazka_2 = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *prepazka_2 = 0;
    prepazka_3 = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *prepazka_3 = 0;

    operace = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *operace = 1;
    // promenne
    // semafory
    sem_t *sem_zakaznikid = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0); // zajisti originalitu id zakaznika
    sem_init(sem_zakaznikid, 1, 1); 

    sem_t *sem_urednikid = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0); // zajisti originalitu id urednika
    sem_init(sem_urednikid, 1, 1); 

    sem_Ucall_1 = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_init(sem_Ucall_1, 1, 0);

    sem_Zcall_1 = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_init(sem_Zcall_1, 1, 1);

    sem_Ucall_2 = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_init(sem_Ucall_2, 1, 0);

    sem_Zcall_2 = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_init(sem_Zcall_2, 1, 1);

    sem_Ucall_3 = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_init(sem_Ucall_3, 1, 0);

    sem_Zcall_3 = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_init(sem_Zcall_3, 1, 1);

    sem_t *sem_prepazka = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0); // vyber prepazky (muze delat jen 1 proces)
    sem_init(sem_prepazka, 1, 1);


    sem_synch_1 = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_init(sem_synch_1, 1, 0);

    sem_synch_2 = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_init(sem_synch_2, 1, 0);

    sem_synch_3 = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_init(sem_synch_3, 1, 0);
    
    sem_operace = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_init(sem_operace, 1, 1);

    sem_otevreno = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_init(sem_otevreno, 1, 1);
    // semafory
    // alokace sdilene pameti
 
    pid_t zakaznik;
    pid_t urednik;
    // vytvoreni procesu zakaznik
    for(int i = 0; i < *NZ; i++)
    {
        zakaznik = fork();
        if(zakaznik == 0)
        {
            fce_zakaznik(idZ, sem_zakaznikid);
            exit(0);
        }
        if(zakaznik < 0) // selhani fork()
        {
            fprintf(stderr, "fork() error\n"); // chybove hlaseni
            dealokace(sem_prepazka, sem_urednikid, sem_zakaznikid, idZ, idU); // dealokace zdroju
            kill(0, SIGKILL); // ukonceni vsech procesu
        }
    }
    // vytvoreni procesu urednik
    for(int i = 0; i < *NU; i++)
    {
        urednik = fork();
        if(urednik == 0)
        {
            fce_urednik(idU, sem_urednikid, sem_prepazka);
            exit(0);
        }
        if(urednik < 0) // selhani fork()
        {
            fprintf(stderr, "fork() error\n"); // chybove hlaseni
            dealokace(sem_prepazka, sem_urednikid, sem_zakaznikid, idZ, idU); // dealokace zdroju
            kill(0, SIGKILL); // ukonceni vsech procesu
        }
    }
    int wait_time = (*F / 2) + rand() % (*F / 2 + 1);
    usleep(wait_time*1000);

    sem_wait(sem_otevreno); // kriticka sekce => vypise closing a nastavi promennou otevreno na 0
    *otevreno = 0; // zavreno

    sem_wait(sem_operace);
    setvbuf(output, NULL, _IONBF, 0); // nastavi fprintf aby zapisoval do souboru okamzite
    fprintf(output, "%d: closing\n", *operace);
    *operace = *operace+1;
    sem_post(sem_operace);

    sem_post(sem_otevreno); // uvolni pristup, s tim ze uz otevreno = 0 
    
    while(wait(NULL)>0); // cekani na skonceni vsech procesu

    // dealokace sdilene pameti
    dealokace(sem_prepazka, sem_urednikid, sem_zakaznikid, idZ, idU);
    // dealokace sdilene pameti
    return 0;
}