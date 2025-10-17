#pragma once

#include <openssl/rsa.h>
#include <openssl/md5.h>
#include <openssl/aes.h>
#include <openssl/x509.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include "ISSL.h"
class C_ssl: public I_ssl
{
public:


    C_ssl();
    ~C_ssl() {}


    int SSL_do_handshake(SSL *ssl);
    SSL_CTX *SSL_CTX_new(const SSL_METHOD *meth);
    void _SSL_load_error_strings();
    int _SSL_library_init();
    int SSL_CTX_use_certificate_file(SSL_CTX *ctx, const char *file,
                                     int type);
    int SSL_CTX_use_PrivateKey_file(SSL_CTX *ctx, const char *file,
                                    int type);
    BIO *BIO_new_fd(int fd, int close_flag);
    int SSL_CTX_check_private_key(const SSL_CTX *ctx);
    const SSL_METHOD* _SSLv23_method();
    SSL* SSL_new(SSL_CTX*);
    int SSL_set_fd(SSL*, int);
    void SSL_free(SSL*);

    void SSL_set_connect_state(SSL* ssl)
    {
        ::SSL_set_connect_state(ssl);
    }
    void SSL_set_accept_state(SSL* ssl)
    {
        ::SSL_set_accept_state(ssl);
    }
    int SSL_get_error(const SSL *s, int ret_code)
    {
        return ::SSL_get_error(s,ret_code);
    }
    void ERR_print_errors(BIO *bp)
    {
        return ::ERR_print_errors(bp);
    }
    int SSL_write(SSL *ssl, const void *buf, int num)
    {
        return ::SSL_write(ssl,buf,num);
    }
    int SSL_read(SSL *ssl, void *buf, int num)
    {
        return ::SSL_read(ssl,buf,num);

    }

    void ERR_print_errors_fp(FILE* fp)
    {
        ::ERR_print_errors_fp(fp);
    }











};



