#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/stat.h>
#include <pthread.h>
#include "queue.h"
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define NUM_CONSUMERS 1

int total = 0;          // Variable global para el computo total del coste de todas las operaciones

queue *buff;            // Creamos un buffer circular

typedef struct{         // Estructura para almacenar la operacion de una maquina en la entrada de datos
    int idmaquina;
    int tipomaquina;
    int tiempouso;
}moperacion;

typedef struct{         // Estructura para el paso de parametros a la funcion productor (Estructua para pasar por parametro a un thread productor) 
    int indiceinicio;
    int indicefinal;
    queue *buff;
    moperacion *array;  // Referencia del array de operaciones para los threads
}paramfp;

// Variables para el control de la concurrencia
pthread_mutex_t mut;                // Mutex para control productores-consumidor
pthread_cond_t condicion_full;		// Variable condicion para cuando este lleno
pthread_cond_t condicion_empty;     // Variable condicion para cuando este vacio

/**
 * Entry point
 * @param argc
 * @param argv
 * @return
 */


//          FUNCION DEL PRODUCTOR       
void *funcionproductor(void *arg){
       
       // Recogemos los parametros que se le pasan al thread productor
        paramfp *parametros;
        parametros =(paramfp *) arg;
     
        pthread_mutex_lock(&mut); // Accedemos a los parametros indice inicial y final que le tocan a cada uno de los productores 
        int indiceinicio = parametros->indiceinicio;
        int indicefinal = parametros->indicefinal;
        pthread_mutex_unlock(&mut);
      
        for(int i=indiceinicio;i<indicefinal;i++){  // Bucle que se recorre para que cada thread realize las insercciones que le han tocado en el reparto
            // Bloqueamos la seccion critica   
            pthread_mutex_lock(&mut);
                    
            // Creamos un elemento para insertar en el buffer
            struct element * elemento = malloc(sizeof(struct element));
            elemento->ocupado=1;
            elemento->type = parametros->array[i].tipomaquina;
            elemento->time = parametros->array[i].tiempouso;

            // Espera en caso de que el buffer este lleno hasta que el consumidor saque un elemento 
            while(queue_full(parametros->buff))
                pthread_cond_wait(&condicion_full,&mut);

            // Insertamos el elemento en el buffer circular
            queue_put(parametros->buff,elemento);

            pthread_cond_broadcast(&condicion_empty); // Una vez insertamos un elemento en la cola avisamos al consumidor por si esta a la espera

            pthread_mutex_unlock(&mut);  
            free(elemento); //Liberamos memoria de cada elemento que creamos por iteracion
        }
    

        pthread_mutex_lock(&mut);   // Aumentamos el contador de threads productores que han acabado su tarea
            buff->contadorTh++;
        pthread_mutex_unlock(&mut);

        // Fin del thread
        pthread_exit(NULL); 
}

//          FUNCION DEL CONSUMIDOR   
void *funcionconsumidor(){
        struct element *a ;
        
        while((buff->contadorlecturas<buff->noperacionestotales) || queue_empty(buff)!=1){  // Bucle que correra mientras haya elementos por procesar 
            pthread_mutex_lock(&mut);
              
            while ((queue_empty(buff)==1)&&(buff->contadorlecturas<buff->noperacionestotales))// Variable condicion para cuando se encuentre vacia la cola
                pthread_cond_wait(&condicion_empty,&mut);
                
            if(queue_empty(buff)!=1){
                a = queue_get(buff);

                // Cada tipo de maquina tiene un coste por minuto aqui se le asigna el valor dependiendo del tipo
                int valordeclase;
                if(a->type==1){         // En caso de ser de tipo 1 la maquina su consumo es de 1€/minuto
                    valordeclase=1;
                }
                else if(a->type==2){    // En caso de ser de tipo 2 la maquina su consumo es de 3€/minuto
                    valordeclase=3;
                }
                else if(a->type==3){    // En caso de ser de tipo 3 la maquina su consumo es de 10€/minuto
                    valordeclase=10;
                }

                // Realiza la operacion para añadir coste
                total = total + (a->time*valordeclase);
                // Aumentamos el contador de lecturas realizadas
                buff->contadorlecturas++;

                // Una vez recogido un elemento del buffer le indicamos a los productores que estan a la espera por que el buffer esta lleno que pueden entrar
                pthread_cond_broadcast(&condicion_full);
            }
         
        
            pthread_mutex_unlock(&mut);
        
        } 
        
        // Fin del thread consumidor
        pthread_exit(NULL);
}

//          Funcion MAIN
int main (int argc, const char * argv[] ) {
    
    if(argc!=4){ // Caso de error en el que no se le pasa el numero correcto de argumentos
        perror("Error numero invalido de argumentos \n");
        exit(-1);
    }
    
    // Recogemos los parametros pasados por argv
    char filename [500]; // FIXME HE PUESTO 500 POR PONER ALGO PERO DEBERIA SER PATHMAX O ALGUNA CONSTANTE 
    strcpy(filename,argv[1]); 
    int numproducers =atoi(argv[2]);
    int buffsize =atoi(argv[3]);
    if(numproducers<1 ){    // Caso de error en el que se le pasa un numero de threads productores menor que 1
        perror("Error no se pueden poner productores negativos \n");
        exit(-1);
    }
    if(buffsize<1){        // Caso de error en el que se le pasa un tamaño de buffer incorrecto
        perror("Error buffer size  \n");
        exit(-1);
    }
    
    int noperaciones; // numero de operaciones a realizar (primera linea del fichero)
    int OPERACIONESREALES=0; // Variable para cuantificar el numero de operaciones reales que hay independientemente de lo que se especifica en la primera linea del fichero
    
    // abrimos el fichero y comprobamos si se ha producido un error al abrir
    FILE * fp;
    if((fp=fopen(filename,"r"))==NULL){
        perror("Error al abrir el archivo \n");
    }
    fscanf(fp,"%d",&noperaciones); // obtenemos el numero de operaciones especificadas en la primera linea del archivo 
   
    
    // Creamos el array de operaciones para almacenar todas las operaciones definidas por noperaciones y pasarselo a los threads productores
    moperacion operaciones[noperaciones];

    
    int i=0;
    while(fscanf(fp,"%d %d %d", &operaciones[i].idmaquina, &operaciones[i].tipomaquina, &operaciones[i].tiempouso)!=EOF){ // Recogemos los datos de las operaciones hasta llegar al fin del fichero
           
        if(operaciones[i].tipomaquina!=1 && operaciones[i].tipomaquina!=2 && operaciones[i].tipomaquina!=3){  // Caso de error en el que se pone una maquina de un tipo que no existe
            perror("ERROR: Se ha introducido en el fichero un tipo de maquina erronea \n");
            exit(-1);
        }
        OPERACIONESREALES++;

        if(OPERACIONESREALES>noperaciones){ //Caso en el que solo se quieren realizar las operaciones indicadas cuando hay mas
            break;
        }
        i++;
    }
        
    fclose(fp);

    if(OPERACIONESREALES<noperaciones){ // Caso de error en el que el numero de operaciones reales es menor que el indicado en la primera linea del fichero
            perror("Error: no se especifica el numero correcto de operaciones que hay en el documento \n");
            exit(-1);
    }
   
    // Inicializamos el buffer circular 
    buff=queue_init(buffsize); 

    buff->NumProd = numproducers;   
    buff->contadorTh = 0;   // Ponemos a 0 el numero de threads productores que han acabado
    buff->noperacionestotales = noperaciones;

    // Reparto de las operaciones del array obtenemos el numero de elementos por thread y el resto que se le añadira al final al ultimo thread 
    int operacionesporth;
    int resto;

    if(numproducers<=noperaciones){
    operacionesporth =(int) noperaciones/numproducers;
    resto = (int) noperaciones%numproducers;
    }else if(numproducers==1){  // Reparto para 1 solo thread productor 
        operacionesporth = noperaciones;
        resto = 0;
    }
    
    //TODO Reparto para cuando hay mas threads que operaciones O ES UN ERROR???

    
    // Indices para cada thread
    int indiceinicio=0;
    int indicefinal;
    
    // Inicializamos el mutex y las variables condicion
    pthread_mutex_init(&mut, NULL);
	pthread_cond_init(&condicion_empty, NULL);
    pthread_cond_init(&condicion_full, NULL);

    pthread_t mythread[numproducers]; // Array para almacenar los manejadores de los threads creados
    paramfp parametros[numproducers];
    for(int i=0;i<numproducers;i++){
        if(noperaciones>=numproducers && numproducers!=1){

            //Reparto de indices para cada thread productor
            if(i==numproducers-1){ // Caso para el ultimo thread que en caso de no ser multiplo los elementos del array y threads se llevara de mas
                indiceinicio = indiceinicio+(operacionesporth);
                indicefinal = indicefinal + (operacionesporth+resto);
            }
            else if(i==0){ //caso inicial 
                indiceinicio=0;
                indicefinal = indiceinicio + operacionesporth;
            }else{  //Caso comun 
                indiceinicio = indiceinicio+operacionesporth;
                indicefinal = indicefinal +(operacionesporth);
            }
         
        }
        else if(numproducers==1){ // Caso en el que solo hay 1 productor
            indiceinicio = 0;
            indicefinal = noperaciones;
        }
        else if(noperaciones<numproducers){ // TODO en este caso se reparten las n operaciones en los n primeros threads y el reto no hacen nada o no se crean
                printf("CASO EN EL QUE HAY MAS THREADS PRODUCTORES QUE OPERACIONES A REALIZAR \n");
                
        }

        // Metemos en cada estructura de paso de parametros los indices correspondientes para el thread productor
        parametros[i].indiceinicio = indiceinicio;
        parametros[i].indicefinal = indicefinal;
        parametros[i].buff = buff;
        parametros[i].array = operaciones;
       
        // Creamos el thread productor: le pasamos la funcion y la estructura por parametros 
        pthread_create(&mythread[i],NULL,(void * (*)(void *))funcionproductor, &parametros[i]); 

    }

    // Creamos el thread del consumidor 
    pthread_t thconsumer;
    pthread_create(&thconsumer,NULL,(void * (*)(void *))funcionconsumidor,NULL);
                   
    //  Recogemos los threads creados
    for(int i=0;i<numproducers;i++){
        pthread_join(mythread[i],NULL);
    }
    pthread_join(thconsumer,NULL);

    // Destruimos los mutex y variables condicion usados
    pthread_mutex_destroy(&mut);
    pthread_cond_destroy(&condicion_empty);
    pthread_cond_destroy(&condicion_full);
    
    // Destruimos el buffer circular y liberamos sus recursos usados
    queue_destroy(buff);

    // Imprimimos el computo total de las operaciones calculadas
    printf("Total: %i €.\n", total);
    return 0;
}