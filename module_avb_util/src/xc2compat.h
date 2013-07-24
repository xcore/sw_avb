#ifndef _xc2compat_h_
#define _xc2compat_h_

#ifdef __XC__
#define unsafe unsafe
#else
#define unsafe
#endif

#ifdef __XC__
#define CLIENT_INTERFACE(type, name) client interface type name
#else
#define CLIENT_INTERFACE(type, name) unsigned int name
#endif

#endif