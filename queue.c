#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "queue.h"


//To create a queue 
queue* queue_init(int size){
    
    queue * q = (queue *)malloc(sizeof(queue)+size*sizeof(*q->array)); // Reserva memoria dinamicamente
     //q->array = (struct element *) malloc(sizeof(struct element)*size);
    q->size=size;
    q->nelementos=0; // inicialmente no tiene elementos 
    q->index=0; // empieza en 0 
    q->contadorlecturas=0;
    q->contadorTh=0;
 
    for(int i=0; i<q->size;i++){
        (q->array[i]).ocupado=0;
        (q->array[i]).time=0;
        (q->array[i]).type=0;
    }
       
    return q;
}


// To Enqueue an element
int queue_put(queue *q, struct element * x) {
    
    if (q->index>=q->size){ // si llega al final tiene que volver al principio
        q->index=0;
    }

    if (queue_full(q)==1){
        perror("Error: El buffer circular estaba lleno y se ha intentado escribir");
        free(q);
        exit(-1);
    }else{
        for(int i = 0; i<q->size;i++){ 
           q->index++;
           if (q->index>=q->size){ // si llega al final tiene que volver al principio
                        q->index=0;
            }

            if(q->array[q->index].ocupado==0){ // caso de haber un hueco libre
                q->array[q->index]=*x;
                q->nelementos++;
                break;
            }     
        }  
    }
    return q->index;
}

 
// To Dequeue an element.
struct element* queue_get(queue *q) {  // IMPORTANTE: Al recoger el return desde donde se llama a la funcion hacer free para evitar desbordamiento
    
    struct element * elementoo = malloc(sizeof(struct element));
    
   if(queue_empty(q)==1){ //caso en el que se intente leer y esta vacio
	  perror("El buffer circular estaba vacio \n");
        free(q);
        exit(-1);
	}
     else{// caso en el que hay elementos por leer
            for(int i=0;i<q->size;i++){
                if(q->array[i].ocupado==1){
                    elementoo->ocupado=q->array[i].ocupado;
                    elementoo->time=q->array[i].time;
                    elementoo->type=q->array[i].type;

                    q->array[i].ocupado=0; // estas tres lineas dejarian "vacio" el elemento
                    q->array[i].time=0;
                    q->array[i].type=0;
                    q->nelementos--;
                    break;
                }
                  
            }
    }
        
    return elementoo;
}


//To check queue state
int queue_empty(queue *q){
    int vacio = 0;
    if(q->nelementos==0){   // en caso de que no haya elementos estara vacio 
        vacio = 1;
    }

    return vacio;
}


int queue_full(queue *q){
    int lleno =0;
    if(q->nelementos==q->size){ // en caso de que el numero total de elementos que hay sea igual al tamaño 
        lleno=1;
    }
    
    return lleno;
}


//To destroy the queue and free the resources
int queue_destroy(queue *q){
    if(!queue_empty(q)){ // por si queda algun elemento, es poco probable que se de este caso 
		printf("ERROR no esta vacia la cola\n");
		exit(-1);
	}
    
    free(q); // liberamos los recursos ocupados por q 
    return 0;
}


int printqueue(queue *q){  // metodo para imprimir el contenido de la cola solo para pruebas
    int size = q->size;
    
    printf("Numero de elementos que hay dentro: %d ; Tamaño: %d ; Indice en: %d \n",q->nelementos,q->size,q->index);
    printf("Elementos del array: \n");
    for(int i=0;i<size;i++){
        
        printf("Esta ocupado: %d ; Tipo de maquina: %d ; Tiempo de uso: %d \n",q->array[i].ocupado,q->array[i].type,q->array[i].time);
    }

    return 0;
}
    