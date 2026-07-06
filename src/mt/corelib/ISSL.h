#pragma once
#include <stdint.h>
#include "ifacesEvent.h"
#include <string>
#include <sys/types.h>
#include "REF.h"
#include "refstring.h"
#include <openssl/ssl.h>
/// interfaces/wrapper to access OpenSSL

// struct SSL;
class I_ssl: public Ifaces::Base
{
public:
    virtual int SSL_do_handshake(SSL *ssl)=0;
    virtual SSL_CTX *SSL_CTX_new(const SSL_METHOD *meth)=0;
    virtual void _SSL_load_error_strings()=0;
    virtual int _SSL_library_init()=0;
    virtual BIO *BIO_new_fd(int fd, int close_flag)=0;
    virtual int SSL_CTX_use_certificate_file(SSL_CTX *ctx, const char *file,
            int type)=0;
    virtual int SSL_CTX_use_PrivateKey_file(SSL_CTX *ctx, const char *file,
                                            int type)=0;
    virtual int SSL_CTX_check_private_key(const SSL_CTX *ctx)=0;
    virtual const SSL_METHOD* _SSLv23_method()=0;
    virtual SSL* SSL_new(SSL_CTX*)=0;
    virtual int SSL_set_fd(SSL*, int)=0;
    virtual void SSL_free(SSL*)=0;
    virtual void SSL_set_connect_state(SSL*)=0;
    virtual void SSL_set_accept_state(SSL*)=0;
    virtual int SSL_get_error(const SSL *s, int ret_code)=0;
    virtual void ERR_print_errors(BIO *bp)=0;
    virtual void ERR_print_errors_fp(FILE* fp)=0;

    virtual int SSL_write(SSL *ssl, const void *buf, int num)=0;
    virtual int SSL_read(SSL *ssl, void *buf, int num)=0;



    virtual ~I_ssl() {}

};

