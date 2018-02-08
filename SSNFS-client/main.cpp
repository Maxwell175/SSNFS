#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
/* For pread()/pwrite()/utimensat() */
#define _XOPEN_SOURCE 700
#endif

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <iostream>
#include <ctime>

#include "common.h"

#define STR_EXPAND(tok) #tok
#define STR(tok) STR_EXPAND(tok)
#define HELLO_STR "SSNFS client version " STR(_CLIENT_VERSION)

SSL_CTX* ctx = nullptr;
BIO *serverConn = nullptr;
SSL *ssl = nullptr;

const char testBase[] = "/home/maxwell/fuse-test-base";

static void *xmp_init(struct fuse_conn_info *conn,
					  struct fuse_config *cfg)
{
	(void) conn;
	cfg->use_ino = 1;

	/* Pick up changes from lower filesystem right away. This is
	   also necessary for better hardlink support. When the kernel
	   calls the unlink() handler, it does not know the inode of
	   the to-be-removed entry and can therefore not invalidate
	   the cache of the associated inode - resulting in an
	   incorrect st_nlink value being reported for any remaining
	   hardlinks to this inode. */
	cfg->entry_timeout = 0;
	cfg->attr_timeout = 0;
	cfg->negative_timeout = 0;
	cfg->direct_io = 1;

	return nullptr;
}

static int xmp_getattr(const char *path, struct stat *stbuf,
					   struct fuse_file_info *fi)
{
	(void) fi;
	int res;

	char finalPath[strlen(testBase) + strlen(path)];
	strcpy(finalPath, testBase);
	strcat(finalPath, path);

	res = lstat(finalPath, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_access(const char *path, int mask)
{
	int res;

	char finalPath[strlen(testBase) + strlen(path)];
	strcpy(finalPath, testBase);
	strcat(finalPath, path);

	res = access(finalPath, mask);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_readlink(const char *path, char *buf, size_t size)
{
	int res;

	char finalPath[strlen(testBase) + strlen(path)];
	strcpy(finalPath, testBase);
	strcat(finalPath, path);

	res = readlink(finalPath, buf, size - 1);
	if (res == -1)
		return -errno;

	buf[res] = '\0';
	return 0;
}


static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
					   off_t offset, struct fuse_file_info *fi,
					   enum fuse_readdir_flags flags)
{
	DIR *dp;
	struct dirent *de;

	(void) offset;
	(void) fi;
	(void) flags;

	char finalPath[strlen(testBase) + strlen(path)];
	strcpy(finalPath, testBase);
	strcat(finalPath, path);

	dp = opendir(finalPath);
	if (dp == nullptr)
		return -errno;

	while ((de = readdir(dp)) != nullptr) {
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;
		if (filler(buf, de->d_name, &st, 0, static_cast<fuse_fill_dir_flags>(0)))
			break;
	}

	closedir(dp);
	return 0;
}

static int xmp_mknod(const char *path, mode_t mode, dev_t rdev)
{
	int res;

	char finalPath[strlen(testBase) + strlen(path)];
	strcpy(finalPath, testBase);
	strcat(finalPath, path);

	/* On Linux this could just be 'mknod(path, mode, rdev)' but this
	   is more portable */
	if (S_ISREG(mode)) {
		res = open(finalPath, O_CREAT | O_EXCL | O_WRONLY, mode);
		if (res >= 0)
			res = close(res);
	} else if (S_ISFIFO(mode))
		res = mkfifo(finalPath, mode);
	else
		res = mknod(finalPath, mode, rdev);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_mkdir(const char *path, mode_t mode)
{
	int res;

	char finalPath[strlen(testBase) + strlen(path)];
	strcpy(finalPath, testBase);
	strcat(finalPath, path);

	res = mkdir(finalPath, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_unlink(const char *path)
{
	int res;

	char finalPath[strlen(testBase) + strlen(path)];
	strcpy(finalPath, testBase);
	strcat(finalPath, path);

	res = unlink(finalPath);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_rmdir(const char *path)
{
	int res;

	char finalPath[strlen(testBase) + strlen(path)];
	strcpy(finalPath, testBase);
	strcat(finalPath, path);

	res = rmdir(finalPath);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_symlink(const char *from, const char *to)
{
	int res;

	char finalPath[strlen(testBase) + strlen(from)];
	strcpy(finalPath, testBase);
	strcat(finalPath, from);

	res = symlink(finalPath, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_rename(const char *from, const char *to, unsigned int flags)
{
	int res;

	char finalPath[strlen(testBase) + strlen(from)];
	strcpy(finalPath, testBase);
	strcat(finalPath, from);

	char finalToPath[strlen(testBase) + strlen(to)];
	strcpy(finalToPath, testBase);
	strcat(finalToPath, to);

	if (flags)
		return -EINVAL;

	res = rename(finalPath, finalToPath);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_link(const char *from, const char *to)
{
	int res;

	char finalPath[strlen(testBase) + strlen(from)];
	strcpy(finalPath, testBase);
	strcat(finalPath, from);

	res = link(finalPath, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_chmod(const char *path, mode_t mode,
					 struct fuse_file_info *fi)
{
	(void) fi;
	int res;

	char finalPath[strlen(testBase) + strlen(path)];
	strcpy(finalPath, testBase);
	strcat(finalPath, path);

	res = chmod(finalPath, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_chown(const char *path, uid_t uid, gid_t gid,
					 struct fuse_file_info *fi)
{
	(void) fi;
	int res;

	char finalPath[strlen(testBase) + strlen(path)];
	strcpy(finalPath, testBase);
	strcat(finalPath, path);

	res = lchown(finalPath, uid, gid);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_truncate(const char *path, off_t size,
						struct fuse_file_info *fi)
{
	int res;

	char finalPath[strlen(testBase) + strlen(path)];
	strcpy(finalPath, testBase);
	strcat(finalPath, path);

	if (fi != NULL)
		res = ftruncate(fi->fh, size);
	else
		res = truncate(finalPath, size);
	if (res == -1)
		return -errno;

	return 0;
}

#ifdef HAVE_UTIMENSAT
static int xmp_utimens(const char *path, const struct timespec ts[2],
		       struct fuse_file_info *fi)
{
	(void) fi;
	int res;

	char finalPath[strlen(testBase) + strlen(path)];
	strcpy(finalPath, testBase);
	strcat(finalPath, path);

	/* don't use utime/utimes since they follow symlinks */
	res = utimensat(0, finalPath, ts, AT_SYMLINK_NOFOLLOW);
	if (res == -1)
		return -errno;

	return 0;
}
#endif

static int xmp_create(const char *path, mode_t mode,
					  struct fuse_file_info *fi)
{
	int res;

	char finalPath[strlen(testBase) + strlen(path)];
	strcpy(finalPath, testBase);
	strcat(finalPath, path);

	res = open(finalPath, fi->flags, mode);
	if (res == -1)
		return -errno;

	fi->fh = res;
	return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi)
{
	int res;

	std::vector<char> operationBytes = Common::getBytes(Common::open);
	BIO_write(serverConn, operationBytes.data(), operationBytes.size());
	std::vector<char> opResultBytes;
	opResultBytes.resize(sizeof(Common::ResultCode));
	BIO_read(serverConn, opResultBytes.data(), sizeof(Common::ResultCode));
	Common::ResultCode opResult = Common::getResultFromBytes(opResultBytes.data());
	if (opResult != Common::OK) {
		std::cout << "Invalid OPEN operation response." << std::endl;
		return -ECOMM;
	}

	std::string strPath = path;
	std::vector<char> data = Common::getBytes((uint16_t)strPath.length());
	data.insert(data.end(), strPath.begin(), strPath.end());
	std::vector<char> flagsBytes = Common::getBytes(fi->flags);
	data.insert(data.end(), flagsBytes.begin(), flagsBytes.end());
	BIO_write(serverConn, data.data(), data.size());

	std::vector<char> resultBytes;
	resultBytes.resize(sizeof(int32_t));
	BIO_read(serverConn, resultBytes.data(), resultBytes.size());
	res = Common::getInt32FromBytes(resultBytes.data());

	if (res > 0)
		fi->fh = res;

	return res > 0 ? 0 : res;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
					struct fuse_file_info *fi)
{
	int fd;
	int res;

	char finalPath[strlen(testBase) + strlen(path)];
	strcpy(finalPath, testBase);
	strcat(finalPath, path);

	if(fi == NULL)
		fd = open(finalPath, O_RDONLY);
	else
		fd = fi->fh;

	if (fd == -1)
		return -errno;

	res = pread(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	if(fi == NULL)
		close(fd);
	return res;
}

static int xmp_write(const char *path, const char *buf, size_t size,
					 off_t offset, struct fuse_file_info *fi)
{
	int fd;
	int res;

	std::clock_t start = std::clock();

	(void) fi;
	if(fi == NULL)
		fd = open(path, O_WRONLY);
	else
		fd = fi->fh;

	if (fd == -1)
		return -errno;

	std::vector<char> operationBytes = Common::getBytes(Common::write);
	BIO_write(serverConn, operationBytes.data(), operationBytes.size());
	std::vector<char> opResultBytes;
	opResultBytes.resize(sizeof(Common::ResultCode));
	BIO_read(serverConn, opResultBytes.data(), sizeof(Common::ResultCode));
	Common::ResultCode opResult = Common::getResultFromBytes(opResultBytes.data());
	if (opResult != Common::OK) {
		std::cout << "Invalid WRITE operation response." << std::endl;
		return -ECOMM;
	}

	std::cout << "Operation done." << (std::clock() - start) / (double) (CLOCKS_PER_SEC / 1000) << std::endl;


	std::string strPath = path;
	std::vector<char> data = Common::getBytes((uint16_t)strPath.length());
	data.insert(data.end(), strPath.begin(), strPath.end());
	std::vector<char> fdBytes = Common::getBytes(fd);
	data.insert(data.end(), fdBytes.begin(), fdBytes.end());
	std::vector<char> sizeBytes = Common::getBytes(size);
	data.insert(data.end(), sizeBytes.begin(), sizeBytes.end());
	std::vector<char> offsetBytes = Common::getBytes(offset);
	data.insert(data.end(), offsetBytes.begin(), offsetBytes.end());
	data.insert(data.end(), buf, buf+size);
	BIO_write(serverConn, data.data(), data.size());

	std::cout << "Sent data." << (std::clock() - start) / (double) (CLOCKS_PER_SEC / 1000) << std::endl;

	std::vector<char> resultBytes;
	resultBytes.resize(sizeof(int32_t));
	BIO_read(serverConn, resultBytes.data(), resultBytes.size());
	res = Common::getInt32FromBytes(resultBytes.data());

	std::cout << "All done." << (std::clock() - start) / (double) (CLOCKS_PER_SEC / 1000) << std::endl;

	if(fi == NULL)
		close(fd);
	return res;
}

#ifdef HAVE_SETXATTR
/* xattr operations are optional and can safely be left unimplemented */
static int xmp_setxattr(const char *path, const char *name, const char *value,
			size_t size, int flags)
{
	int res = lsetxattr(path, name, value, size, flags);
	if (res == -1)
		return -errno;
	return 0;
}

static int xmp_getxattr(const char *path, const char *name, char *value,
			size_t size)
{
	int res = lgetxattr(path, name, value, size);
	if (res == -1)
		return -errno;
	return res;
}

static int xmp_listxattr(const char *path, char *list, size_t size)
{
	int res = llistxattr(path, list, size);
	if (res == -1)
		return -errno;
	return res;
}

static int xmp_removexattr(const char *path, const char *name)
{
	int res = lremovexattr(path, name);
	if (res == -1)
		return -errno;
	return 0;
}
#endif /* HAVE_SETXATTR */

void handleFailure() {
	std::cout << "A SSL error occured." << std::endl;
	printf("%s\n", ERR_error_string(ERR_get_error(), nullptr));
	exit(1);
}

static fuse_operations xmp_oper;

int main(int argc, char *argv[])
{
	umask(0);

	xmp_oper.init       = xmp_init;
	xmp_oper.getattr	= xmp_getattr;
	xmp_oper.access		= xmp_access;
	xmp_oper.readlink	= xmp_readlink;
	xmp_oper.readdir	= xmp_readdir;
	xmp_oper.mknod		= xmp_mknod;
	xmp_oper.mkdir		= xmp_mkdir;
	xmp_oper.symlink	= xmp_symlink;
	xmp_oper.unlink		= xmp_unlink;
	xmp_oper.rmdir		= xmp_rmdir;
	xmp_oper.rename		= xmp_rename;
	xmp_oper.link		= xmp_link;
	xmp_oper.chmod		= xmp_chmod;
	xmp_oper.chown		= xmp_chown;
	xmp_oper.truncate	= xmp_truncate;
#ifdef HAVE_UTIMENSAT
	xmp_oper.utimens	= xmp_utimens;
#endif
	xmp_oper.open		= xmp_open;
	//xmp_oper.create 	= xmp_create;
	xmp_oper.read		= xmp_read;
	xmp_oper.write		= xmp_write;
#ifdef HAVE_SETXATTR
	xmp_oper.setxattr	= xmp_setxattr;
	xmp_oper.getxattr	= xmp_getxattr;
	xmp_oper.listxattr	= xmp_listxattr;
	xmp_oper.removexattr	= xmp_removexattr;
#endif
	
	long res = 1;

	(void)SSL_library_init();

	SSL_load_error_strings();

	const SSL_METHOD* method = SSLv23_method();
	if(!(NULL != method)) handleFailure();

	ctx = SSL_CTX_new(method);
	if(!(ctx != NULL)) handleFailure();

/* Cannot fail ??? */
	//SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, verify_callback);

/* Cannot fail ??? */
	//SSL_CTX_set_verify_depth(ctx, 4);

/* Cannot fail ??? */
	const long flags = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION;
	SSL_CTX_set_options(ctx, flags);

	//res = SSL_CTX_load_verify_locations(ctx, "random-org-chain.pem", NULL);
	//if(!(1 == res)) handleFailure();

	serverConn = BIO_new_ssl_connect(ctx);
	if(!(serverConn != NULL)) handleFailure();

	res = BIO_set_conn_hostname(serverConn, "localhost:2050");
	if(!(1 == res)) handleFailure();

	BIO_get_ssl(serverConn, &ssl);
	if(!(ssl != NULL)) handleFailure();

	const char* const PREFERRED_CIPHERS = "HIGH:!aNULL:!kRSA:!PSK:!SRP:!MD5:!RC4";
	res = SSL_set_cipher_list(ssl, PREFERRED_CIPHERS);
	if(!(1 == res)) handleFailure();

	//res = SSL_set_tlsext_host_name(ssl, HOST_NAME);
	if(!(1 == res)) handleFailure();

	res = BIO_do_connect(serverConn);
	if(!(1 == res)) handleFailure();

	res = BIO_do_handshake(serverConn);
	if(!(1 == res)) handleFailure();

/* Step 1: verify a server certificate was presented during the negotiation */
	X509* cert = SSL_get_peer_certificate(ssl);
	if(cert) { X509_free(cert); } /* Free immediately */
	if(NULL == cert) handleFailure();

/* Step 2: verify the result of chain verification */
/* Verification performed according to RFC 4158    */
	res = SSL_get_verify_result(ssl);
	//if(!(X509_V_OK == res)) handleFailure();
	std::cout << "Verify Result: " << res << std::endl;


	int len;
	std::vector<char> data = Common::getBytes(Common::Hello);
	std::string clientHelloStr = HELLO_STR;
	int32_t clientHelloLen = clientHelloStr.length();
	std::vector<char> clientHelloLenBytes = Common::getBytes(clientHelloLen);
	data.insert(data.end(), clientHelloLenBytes.begin(), clientHelloLenBytes.end());
	data.insert(data.end(), clientHelloStr.begin(), clientHelloStr.end());
	std::cout << std::endl;
	BIO_write(serverConn, data.data(), data.size());

	std::vector<char> ServerResultBytes;
	ServerResultBytes.resize(sizeof(Common::ResultCode));
	BIO_read(serverConn, ServerResultBytes.data(), sizeof(Common::ResultCode));
	Common::ResultCode ServerResult = Common::getResultFromBytes(ServerResultBytes.data());
	if (ServerResult != Common::Hello) {
		std::cout << "Invalid result code from server!" << std::endl;
		exit(1);
	}

	std::vector<char> ServerHelloLenBytes;
	ServerHelloLenBytes.resize(sizeof(int32_t));
	BIO_read(serverConn, ServerHelloLenBytes.data(), sizeof(int32_t));
	int32_t serverHelloLen = Common::getInt32FromBytes(ServerHelloLenBytes.data());

	std::vector<char> ServerHelloBytes;
	ServerHelloBytes.resize(serverHelloLen);
	BIO_read(serverConn, ServerHelloBytes.data(), serverHelloLen);
	std::cout << "Server Hello: " << ServerHelloBytes.data() << std::endl;


	int FUSEargc = 4;
	char *FUSEargv[4];
	FUSEargv[0] = argv[0];
	FUSEargv[1] = "-f";
	FUSEargv[2] = "-s";
	FUSEargv[3] = "/home/maxwell/fuse-test-mount";

	return fuse_main(FUSEargc, FUSEargv, &xmp_oper, NULL);
}