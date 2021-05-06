



////// not used anymore



#include "m_pd.h"
#include <pthread.h>

typedef struct _websocket {
  t_object  x_obj;
  int pthrdexitmain;
  pthread_t tid;
  int intersocket;
  uint16_t pd_port;
  t_canvas  *x_canvas;
  t_outlet *symbol_out, *bang_out;
  
} t_websocket;