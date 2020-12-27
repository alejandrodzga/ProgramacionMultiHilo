#ifndef HEADER_FILE
#define HEADER_FILE


struct element {
  int ocupado; // cuando esta ocupado es 1 y cuando esta vacio es 0
  int type; //Machine type
  int time; //Using time
};

typedef struct queue {
  int noperacionestotales; // Numero total de operaciones que vienen en el fichero
  int nelementos;// numero de elementos que hay en la cola y el numero total de operaciones
  int contadorTh; // contador que incrementa cada vez que se procesa una operacion hasta llegar a nelementos
  int contadorlecturas; // contador que se incrementa cada vez que el consumidor realiza una lectura
  int NumProd; // numero total de productores
  int size; // numero maximo de elementos
  int index; // cabezal para leer o escribir
  struct element array[]; // buffer circular, AQUI SE INTRODUCEN LOS ELEMENTOS
}queue;

queue* queue_init (int size);
int queue_destroy (queue *q);
int queue_put (queue *q, struct element* elem);
struct element * queue_get(queue *q);
int queue_empty (queue *q);
int queue_full(queue *q);

#endif
