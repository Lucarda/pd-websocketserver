/*
 * Copyright (C) 2016-2020 Davidson Francis <davidsondfgl@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */
 
 /*
 * pd-websocketserver modifications 
 * Copyright (c) 2021 Lucas Cordiviola <lucarda27@hotmail.com>
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ws.h>


#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#else			
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#endif





t_class *websocketserver_class;

static void pthreadwrap(t_websocketserver *x);
static void fudiparse_binbuf(t_websocketserver *x, const unsigned char *msg);
static void fudi_bytes_outlet(t_websocketserver *x, const unsigned char *msg);
static void client_open(t_websocketserver *x, int fd);
static void client_close(t_websocketserver *x, int fd);
static void websocketserver_disconnect(t_websocketserver *x);









/**
 * @dir example/
 * @brief wsServer examples folder
 *
 * @file send_receive.c
 * @brief Simple send/receiver example.
 */

/**
 * @brief Called when a client connects to the server.
 *
 * @param fd File Descriptor belonging to the client. The @p fd parameter
 * is used in order to send messages and retrieve informations
 * about the client.
 */
void onopen(int fd, t_websocketserver *x)
{
	char *cli;
	cli = ws_getaddress(fd);
	
	sys_lock();
	
	if (x->verbosity) {
	logpost(x,2,"Connection opened, client: %d | addr: %s", fd, cli);
	}
	t_atom ap[3];
    SETSYMBOL(&ap[0], gensym("open"));
    SETFLOAT(&ap[1], (t_float)fd);
	SETSYMBOL(&ap[2], gensym(cli));
	
	client_open(x, fd);
	
    outlet_list(x->list_out2, NULL, 3, ap);
	
	sys_unlock();
	
	
	free(cli);
}

/**
 * @brief Called when a client disconnects to the server.
 *
 * @param fd File Descriptor belonging to the client. The @p fd parameter
 * is used in order to send messages and retrieve informations
 * about the client.
 */
 

void onclose(int fd, t_websocketserver *x)
{
	char *cli;
	cli = ws_getaddress(fd);
	
	sys_lock();
	
	if (x->verbosity) {
	logpost(x,2,"Connection closed, client: %d | addr: %s", fd, cli);
	}
	t_atom ap[3];
    SETSYMBOL(&ap[0], gensym("close"));
    SETFLOAT(&ap[1], (t_float)fd);
	SETSYMBOL(&ap[2], gensym(cli));
	
    outlet_list(x->list_out2, NULL, 3, ap);
	
	client_close(x, fd);
	
	sys_unlock();
	
	
	free(cli);
}



/**
 * @brief Called when a client connects to the server.
 *
 * @param fd File Descriptor belonging to the client. The
 * @p fd parameter is used in order to send messages and
 * retrieve informations about the client.
 *
 * @param msg Received message, this message can be a text
 * or binary message.
 *
 * @param size Message size (in bytes).
 *
 * @param type Message type.
 */
void onmessage(int fd, const unsigned char *msg, uint64_t size, int type, t_websocketserver *x)
{
	char *cli;
	cli = ws_getaddress(fd);
	
	sys_lock();
	
	if (x->verbosity) {
	logpost(x,2,"I receive a message: %s (size: %" PRId64 ", type: %d), from: %s/%d",
		msg, size, type, cli, fd);
	}	
	
	fudi_bytes_outlet(x, msg);
	
	fudiparse_binbuf(x, msg);
	
	outlet_float(x->sock_out, (t_float)fd);
	
	sys_unlock();
	
	free(cli);

	/**
	 * Mimicks the same frame type received and re-send it again
	 *
	 * Please note that we could just use a ws_sendframe_txt()
	 * or ws_sendframe_bin() here, but we're just being safe
	 * and re-sending the very same frame type and content
	 * again.
	 */
	 
	 
	//ws_sendframe(fd, (char *)msg, size, true, type);
}

	


////////////////////pd external //////////////////////

static void client_open(t_websocketserver *x, int fd) 
{
	int i;
	for (i=0;i<MAX_CLIENTS;i++) {
		if (x->opencloselist[i] == 0) {
				x->opencloselist[i] = fd;
				break;
		}
	}
}



static void client_close(t_websocketserver *x, int fd) 
{
	int i;
	for (i=0;i<MAX_CLIENTS;i++) {
		if (x->opencloselist[i] == fd) {
				x->opencloselist[i] = 0;
				break;
		}
	}
}


static void closeclient(t_websocketserver *x, t_floatarg fd) 
{
	int fdc = (int)fd;
	ws_close_client(fdc);
	
}


static void fudiparse_binbuf(t_websocketserver *x, const unsigned char *amsg)
{
	
  size_t len = strlen(amsg);
  t_binbuf* bbuf = binbuf_new();
  binbuf_text(bbuf, amsg, len);
  
    
  int msg, natom = binbuf_getnatom(bbuf);
  t_atom *at = binbuf_getvec(bbuf);
  
  for (msg = 0; msg < natom;) {
    int emsg;
    for (emsg = msg; emsg < natom && at[emsg].a_type != A_COMMA
           && at[emsg].a_type != A_SEMI; emsg++)
      ;
    if (emsg > msg) {
      int i;
      /* check for illegal atoms */
      for (i = msg; i < emsg; i++)
        if (at[i].a_type == A_DOLLAR || at[i].a_type == A_DOLLSYM) {
          pd_error(x, "fudiparse: got dollar sign in message");
          goto nodice;
        }

      if (at[msg].a_type == A_FLOAT) {
        if (emsg > msg + 1)
          outlet_list(x->x_msgout, 0, emsg-msg, at + msg);
        else outlet_float(x->x_msgout, at[msg].a_w.w_float);
      }
      else if (at[msg].a_type == A_SYMBOL) {
        outlet_anything(x->x_msgout, at[msg].a_w.w_symbol,
                        emsg-msg-1, at + msg + 1);
      }
    }
  nodice:
    msg = emsg + 1;
  }
  
  binbuf_free(bbuf);
	
}



static void fudi_any (t_websocketserver *x, t_symbol*s, int argc, t_atom*argv) {
  char *buf;
  int length;
  int i;
  t_atom at;
  t_binbuf*bbuf = binbuf_new();
  
  
  memset(&x->fudi, 0, MAXPDSTRING);
  
  
  SETSYMBOL(&at, s);
  binbuf_add(bbuf, 1, &at);
  binbuf_add(bbuf, argc, argv);
  binbuf_gettext(bbuf, &buf, &length);
  binbuf_free(bbuf);
  
  
  memcpy(x->fudi, buf, length);
  
  freebytes(buf, length+1);
    
}

static void fudi_bytes_outlet (t_websocketserver *x, const unsigned char *msg) {
  
  
  int length;
  int i;

  
  length = strlen(msg);
  
  
  if((size_t)length>x->x_numatoms) {
  freebytes(x->x_atoms, sizeof(*x->x_atoms) * x->x_numatoms);
  x->x_numatoms = length;
  x->x_atoms = getbytes(sizeof(*x->x_atoms) * x->x_numatoms);
  }

  for(i=0; i<length; i++) {
    SETFLOAT(x->x_atoms+i, msg[i]);
  }
  
  outlet_list(x->bytes_out, 0, length, x->x_atoms);
}



	
static void websocketserver_send (t_websocketserver *x, t_floatarg fd) {

ws_sendframe_txt((int)fd, x->fudi, (int)x->broad);

}

static void verbosity (t_websocketserver *x, t_floatarg n) {

x->verbosity = (int)n;

}


static void websocketserver_disconnect(t_websocketserver *x) {
	
	int i;
	int fdclose;

	for (i=0; i<MAX_CLIENTS; i++) {	
		if (0 != x->opencloselist[i]) {
			fdclose = x->opencloselist[i]; 				
			ws_close_client(fdclose);
		}
	}

	
	#ifdef _WIN32
			
		
		shutdown(x->intersocket, SD_BOTH);
		closesocket(x->intersocket);
		
	#else
		
		shutdown(x->intersocket, SHUT_RDWR);
		close(x->intersocket);	
	#endif
	
	(void) pthread_join(&x->tid, NULL);
	
	x->started = 0;
	
}

static void websocketserver_free(t_websocketserver *x) {
	
	websocketserver_disconnect(x);
	
#ifdef _WIN32
		Sleep(1000);
#else
		sleep(1);
#endif
	
	freebytes(x->x_atoms, sizeof(*x->x_atoms) * x->x_numatoms);
	x->x_atoms = NULL;
	x->x_numatoms = 0;
	
}

/**
 * @brief Main routine.
 *
 * @note After invoking @ref ws_socket, this routine never returns,
 * unless if invoked from a different thread.
 */



static void websocketserver_main(t_websocketserver *x, t_float portpd) {
	
	if(x->started) {
		logpost(x,2,"server already running.");
		return;
	}
	
	memset(&x->opencloselist, 0, (sizeof(int)) * MAX_CLIENTS);

	x->pd_port = (uint16_t)portpd;
	
	x->started = 1;

    x->pthrdexitmain = pthread_create(&x->tid, NULL, pthreadwrap, x);

}







static void pthreadwrap(t_websocketserver *x) {
	
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	
	
	struct ws_events evs;

	evs.onopen    = &onopen;
	evs.onclose   = &onclose;
	evs.onmessage = &onmessage;
	
	ws_socket(&evs, x);
	
}

static void *websocketserver_new(void)
{

  t_websocketserver *x = (t_websocketserver *)pd_new(websocketserver_class);
  

	floatinlet_new(&x->x_obj, &x->broad);
	inlet_new(&x->x_obj, &x->x_obj.ob_pd, 0, 0);
	
	
	
	x->x_msgout = outlet_new(&x->x_obj, &s_list);
	x->bytes_out = outlet_new(&x->x_obj, &s_list);
	x->sock_out = outlet_new(&x->x_obj, &s_float);
	x->list_out2 = outlet_new(&x->x_obj, &s_list);
	
	x->started = 0;
	x->x_numatoms = 1024;
	x->x_atoms = getbytes(sizeof(*x->x_atoms) * x->x_numatoms);
	x->verbosity = 0;
   
  return (void *)x;
}



void websocketserver_setup(void) {

  websocketserver_class = class_new(gensym("websocketserver"),      
			       (t_newmethod)websocketserver_new,
			       (t_method)websocketserver_free,                          
			       sizeof(t_websocketserver),       
			       CLASS_DEFAULT,				   
			       0);                        

  //class_addbang(websocketserver_class, websocketserver_main);  
  class_addmethod(websocketserver_class, (t_method)websocketserver_main, gensym("connect"), A_FLOAT, 0);
  class_addmethod(websocketserver_class, (t_method)websocketserver_disconnect, gensym("disconnect"), 0);
  class_addmethod(websocketserver_class, (t_method)websocketserver_send, gensym("send"), A_FLOAT, 0);
  class_addmethod(websocketserver_class, (t_method)closeclient, gensym("closeclient"), A_FLOAT, 0);
  class_addmethod(websocketserver_class, (t_method)verbosity, gensym("verbosity"), A_FLOAT, 0);
  class_addanything(websocketserver_class, (t_method)fudi_any);
  
}

////////////////////end external //////////////////////







	
	

