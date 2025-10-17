#include "st_rsa_impl.h"
#ifdef _WIN32
#include <malloc.h>
#endif
#include <openssl/ssl.h>

#ifdef _MSC_VER
#pragma comment(lib, "libeay32.lib")
#pragma comment(lib, "ssleay32.lib")
#endif



C_ssl::C_ssl()
{

}

int C_ssl::SSL_do_handshake(SSL *ssl)
{
    return ::SSL_do_handshake(ssl);
}

SSL_CTX *C_ssl::SSL_CTX_new(const SSL_METHOD *meth)
{
    return ::SSL_CTX_new(meth);
}
void C_ssl::_SSL_load_error_strings()
{
    SSL_load_error_strings();
}
int C_ssl::_SSL_library_init()
{
    return SSL_library_init();
}
BIO *C_ssl::BIO_new_fd(int fd, int close_flag)
{
    return ::BIO_new_fd(fd,close_flag);
}
int C_ssl::SSL_CTX_use_certificate_file(SSL_CTX *ctx, const char *file, int type)
{
    return ::SSL_CTX_use_certificate_file(ctx,file,type);
}
int C_ssl::SSL_CTX_use_PrivateKey_file(SSL_CTX *ctx, const char *file, int type)
{
    return ::SSL_CTX_use_PrivateKey_file(ctx,file,type);
}
int C_ssl::SSL_CTX_check_private_key(const SSL_CTX *ctx)
{
    return ::SSL_CTX_check_private_key(ctx);
}

const SSL_METHOD* C_ssl::_SSLv23_method()
{
    return SSLv23_method();
}
SSL* C_ssl::SSL_new(SSL_CTX* ctx)
{
    return ::SSL_new(ctx);
}
int C_ssl::SSL_set_fd(SSL* ssl, int fd)
{
    return ::SSL_set_fd(ssl,fd);
}
void C_ssl::SSL_free(SSL* ssl)
{
    ::SSL_free(ssl);
}

