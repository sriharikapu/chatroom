#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "comm.h"
#include "error.h"
#include "serialize.h"
#include "logging.h"
#include "user.h"
#include "tcp_server.h"
#include "database.h"


#define MAX_USERS 100
//struct para usuarios

t_user * user_list[MAX_USERS] = { NULL };
int connected_users;
int max_index = -1;

pthread_mutex_t user_assign;

//generate index
//name to index

int get_free_index(){

    for (int i = 0; i < MAX_USERS; i++){

        if (user_list[i] == NULL)
            return i;
    }

    return -1;
}


void * connection_handler(void * context) //STRUCT DE CONTEXTO = socket_desc
{

    context_info * my_context = (context_info*) context;
    int connection_fd = my_context->new_connection_descriptor;

    //
    pthread_mutex_lock(&user_assign);
    int user_index = get_free_index();
    int r_bytes;

    t_user * new_user = create_user(connection_fd);
    add_user(user_list, new_user, user_index);
    pthread_mutex_unlock(&user_assign);
    //



    if (user_index > max_index){
        max_index = user_index;
    }

    while (1){ 

        if (user_list[user_index] == NULL) //Parche para comandos que no requieren conexion.
            break;
        
        r_bytes = load_buffer(connection_fd, user_list[user_index]->recv_buffer);
        
        while (user_list[user_index]->recv_buffer->pos + 1 < r_bytes){ //se recibieron dos paquetes juntos o mas.
            handle_tcp_packets(user_index);
            user_list[user_index]->recv_buffer->pos+=1;
        }

        clean_buffer(user_list[user_index]->recv_buffer);

        if (user_list[user_index]->connection_descriptor < 0){
            delete_user(user_list, user_index);
            break;
        }

    }

   	return NULL;
}

void server_main(context_info * context){


	pthread_t sniffer_thread;
    //crear listas de usuario y eso.
     
    if( pthread_create( &sniffer_thread , NULL ,  connection_handler , (void*) context) < 0) //pasar contexto, que contenga new_con y run
        log_error(ERROR, "Thread creation");
        return;

}

 
int main(int argc , char * argv[]) {

    connection_info server_info;

    strcpy(server_info.ip, "127.0.0.1");
    server_info.port = 8888;
    int run = 1;

	if(access(DB_FILE, F_OK) == -1 ) {
		db_create();
	}

    if (pthread_mutex_init(&user_assign, NULL) != 0)
        return 1;
    

    listen_connections((void *)&server_info, server_main, &run); //manejo de errores
    
    pthread_mutex_destroy(&user_assign);
    return 0;
}

