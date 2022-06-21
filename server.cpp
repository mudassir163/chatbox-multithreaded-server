#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <mutex>

#define MAX_LEN 200
#define NUM_COLORS 6
#define SERVERPORT 10000
#define SOCKETERROR (-1)
#define SERVER_BACKLOG 8


using namespace std;

typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;

struct terminal{
	int id;
	string name;
	int socket;
	thread th;
};

vector<terminal> clients;
string def_col="\033[0m";
string colors[]={"\033[31m", "\033[32m", "\033[33m", "\033[34m", "\033[35m","\033[36m"};
int seed=0;
mutex cout_mtx,clients_mtx;

string color(int code);
void set_name(int id, char name[]);
void shared_print(string str, bool endLine);
int broadcast_message(string message, int sender_id);
int broadcast_message(int num, int sender_id);
void end_connection(int id);
void handle_client(int client_socket, int id);
int check(int exp , const char *msg);


int main(){
	int server_socket , client_socket;
	SA_IN server_addr, client_addr;

	// SETUP THE SOCKET
	check( (server_socket = socket(AF_INET,SOCK_STREAM,0)) , "Failed to create socket");

	//BIND THE SERVER
	server_addr.sin_family=AF_INET;
	server_addr.sin_port=htons(SERVERPORT);
	server_addr.sin_addr.s_addr=INADDR_ANY;

	check( (bind(server_socket , (SA *)&server_addr,sizeof(server_addr))) , "Bind failed");
	bzero(&server_addr.sin_zero,0); // set all buffer values to zero

	//LISTEN 
	check((listen(server_socket,SERVER_BACKLOG)) , "Listening failed");

	unsigned int len=sizeof(SA_IN); 
	cout << colors[NUM_COLORS-1]<<"\n\t****CHAT ROOM****"<<"\n"<<def_col;

	while(true){

		check( (client_socket = accept(server_socket , (SA *)&client_addr , &len) ) , "Accept failed");

		seed++;
		thread t( handle_client , client_socket , seed );
		lock_guard<mutex> guard(clients_mtx);
		clients.push_back({seed, string("Anonymous"),client_socket,(move(t))});
	}

	for(int i=0; i<clients.size(); i++){
		if(clients[i].th.joinable()) clients[i].th.join();
	}

	close(server_socket);
	return 0;
}

string color(int code){
	return colors[code%NUM_COLORS];
	}

// Set name of client
void set_name(int id, char name[]){
	for(int i=0; i<clients.size(); i++){
			if(clients[i].id==id) clients[i].name=string(name);		
	}	
}

// For synchronisation of cout statements
void shared_print(string str, bool endLine=true){	
	lock_guard<mutex> guard(cout_mtx);
	cout<<str;
	if(endLine) cout<<endl;
}

// Broadcast message to all clients except the sender
int broadcast_message(string message, int sender_id){
	char temp[MAX_LEN];
	strcpy(temp,message.c_str());
	for(int i=0; i<clients.size(); i++){
		if(clients[i].id!=sender_id) send(clients[i].socket,temp,sizeof(temp),0);
	}		
}

// Broadcast a number to all clients except the sender
int broadcast_message(int num, int sender_id){
	for(int i=0; i<clients.size(); i++){
		if(clients[i].id!=sender_id) send(clients[i].socket,&num,sizeof(num),0);
	}		
}

void end_connection(int id){
	for(int i=0; i<clients.size(); i++){
		if(clients[i].id==id){
			lock_guard<mutex> guard(clients_mtx);
			clients[i].th.detach();
			clients.erase(clients.begin()+i);
			close(clients[i].socket);
			break;
		}
	}				
}

// error handler
int check(int exp, const char *msg)
{
    if (exp == SOCKETERROR)
    {
        perror(msg);
        exit(1);
    }
    return exp;
}

void handle_client(int client_socket, int id){
	char name[MAX_LEN],str[MAX_LEN];
	recv(client_socket,name,sizeof(name),0);
	set_name(id,name);	

	// Display welcome message
	string welcome_message=string(name)+string(" has joined");
	broadcast_message("#NULL",id);	
	broadcast_message(id,id);								
	broadcast_message(welcome_message,id);	
	shared_print(color(id)+welcome_message+def_col);
	
	while(true){
		int bytes_received=recv(client_socket,str,sizeof(str),0);
		if(bytes_received<=0)
			return;
		if(strcmp(str,"#exit")==0){
			// Display leaving message
			string message=string(name)+string(" has left");		
			broadcast_message("#NULL",id);			
			broadcast_message(id,id);						
			broadcast_message(message,id);
			shared_print(color(id)+message+def_col);
			end_connection(id);							
			return;
		}
		broadcast_message(string(name),id);					
		broadcast_message(id,id);		
		broadcast_message(string(str),id);
		shared_print(color(id)+name+" : "+def_col+str);		
	}	
}