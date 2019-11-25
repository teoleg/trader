#ifndef _UDF_PLACE_ORDER_H_
#define _UDF_PLACE_ORDER_H_

#include <mysql.h>

#define LIBVERSION "libporder version 0.0.1"

#if defined(_WIN32) || defined(_WIN64)
#define DLLEXP __declspec(dllexport)
#else
#define DLLEXP
#endif

#ifdef __cplusplus
extern "C" {
#endif


DLLEXP long long place_order_fn();
DLLEXP my_bool place_order_fn_init(UDF_INIT *initid, UDF_ARGS *args,char *message);
DLLEXP my_bool place_order_fn_deinit(UDF_INIT *initid);


#ifdef __cplusplus
}
#endif



#endif
