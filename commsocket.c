#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "commsocket.h"
typedef struct _SckHandle
{
	int sockArray[100]; //定义socket池数组
	int arrayNum; //数组大小
	int sockfd; //socket句柄
	int contime; //链接超时时间
	int sendtime; //发送超时时间
	int revtime; //接受超时时间
} SckHandle;

/**
 * readn - 读取固定字节数
 * @fd: 文件描述符
 * @buf: 接收缓冲区
 * @count: 要读取的字节数
 * 成功返回count，失败返回-1，读到EOF返回<count
 */
//ssize_t 在x64下为long  在x86下为int
/*
 * 函数名：readn
 * 描述：客服端接受数据
 * 参数：
 *
 * 返回：
 * */
ssize_t readn(int fd, void *buf, size_t count)
{
	//size_t 在x64下为 unsigned long 类型， 在x86下为 unsigned int 类型
	size_t nleft = count; //将count接过来 ，个数
	ssize_t nread;
	char *bufp = (char*) buf; //将空指针类型转换为char类型指针。
	while (nleft > 0)
	{
		/*ssize_t read(int fd, void *buf, size_t count);
		 * 从文件描述符fd中读取count字节存到buf中
		 * 返回读取字节数的个数。
		 * */
		if ((nread = read(fd, bufp, nleft)) < 0)
		{
			/*
			 * 如果是被信号中断的继续读
			 * */
			if (errno == EINTR)
				continue;
			return -1;
		}
		/*
		 * 如果输入的读取个数为0，那么返回的读取个数为0
		 * 不执行任何操作。
		 * nleft为剩余的需要读取的字节个数。
		 * 如果为0，说明读到文件尾，
		 *
		 * */
		else if (nread == 0)
			return count - nleft;
		bufp += nread; //将字符指针向前推进已成功读取字符数的大小单位。
		nleft -= nread; //剩余的个数减去已经成功读取的字节数。
	}
	return count;
}

/**
 * writen - 发送固定字节数
 * @fd: 文件描述符
 * @buf: 发送缓冲区
 * @count: 要读取的字节数
 * 成功返回count，失败返回-1
 */
/*
 * 函数名：writen
 * 描述：客服端接受数据
 * 参数：
 *
 * 返回：
 * */
ssize_t writen(int fd, const void *buf, size_t count)
{
	size_t nleft = count; //剩余的需要写入的字节数。
	ssize_t nwritten; //成功写入的字节数。
	char *bufp = (char*) buf; //将缓冲的指针强制转换为字符类型的指针。
	/*
	 * 如果剩余需要写入的字节数大于0则继续
	 * */
	while (nleft > 0)
	{
		/*
		 * ssize_t write(int fd, const void *buf, size_t count);
		 * fd为需要写入的文件描述符，buf为字符缓存区，count为需要写入的字节数。
		 *
		 * */
		if ((nwritten = write(fd, bufp, nleft)) < 0)
		{
			/*
			 * 如果是被信号中断的继续读
			 * */
			if (errno == EINTR)
				continue;
			return -1;
		} else if (nwritten == 0)
			continue;
		//字符指针推移已经写入成功大小的字节数。
		bufp += nwritten;
		//剩余的字节数。
		nleft -= nwritten;
	}
	return count;
}

/**
 * recv_peek - 仅仅查看套接字缓冲区数据，但不移除数据
 * @sockfd: 套接字
 * @buf: 接收缓冲区
 * @len: 长度
 * 成功返回>=0，失败返回-1
 */
/*
 * 函数名：recv_peek
 * 描述：客服端接受数据
 * 参数：
 *
 * 返回：
 * */
ssize_t recv_peek(int sockfd, void *buf, size_t len)
{
	while (1)
	{
		/*
		 * ssize_t recv(int sockfd, void *buf, size_t len, int flags);
		 * sockfd 套接字
		 * len 需要读取的长度
		 * MSG_PEEK只从队列中查看，但不取出。
		 * 返回接受到的字节的长度，失败返回-1，接受到关闭信号返回0；
		 * */
		int ret = recv(sockfd, buf, len, MSG_PEEK);
		/*
		 * 如果被信号中断了，继续
		 * */
		if (ret == -1 && errno == EINTR)
			continue;
		return ret;
	}
}

//函数声明
//客户端环境初始化
/*
 * handle 在函数内部分配内存，socket结构体
 * contime 链接超时时间
 * sendtime 发送超时时间
 * revtime 接受超时时间
 * nConNum 链接池的数目
 * */
/*
 * 函数名：sckCliet_init
 * 描述：客服端接受数据
 * 参数：
 *
 * 返回：
 * */
int sckCliet_init(void **handle, int contime, int sendtime, int revtime,
		int nConNum)
{
	int ret = 0;
	//判断传入的参数
	if (handle == NULL || contime < 0 || sendtime < 0 || revtime < 0)
	{
		ret = Sck_ErrParam; //赋值预先定义的错误。
		printf(
				"func sckCliet_init() err: %d, check  (handle == NULL ||contime<0 || sendtime<0 || revtime<0)\n",
				ret);
		return ret;
	}
	//定义结构体
	SckHandle *tmp = (SckHandle *) malloc(sizeof(SckHandle));
	if (tmp == NULL)
	{
		ret = Sck_ErrMalloc;
		printf("func sckCliet_init() err: malloc %d\n", ret);
		return ret;
	}

	tmp->contime = contime;
	tmp->sendtime = sendtime;
	tmp->revtime = revtime;
	tmp->arrayNum = nConNum;
	*handle = tmp;
	return ret;
}

/**
 * activate_noblock - 设置I/O为非阻塞模式
 * @fd: 文件描符符
 */
/*
 * 函数名：activate_nonblock
 * 描述：客服端接受数据
 * 参数：
 *
 * 返回：
 * */
int activate_nonblock(int fd)
{
	int ret = 0;
	/*
	 * int fcntl(int fd, int cmd, ... \* arg \*)
	 获取文件或者修改文件状态
	 F_GETLK 取得文件锁定的状态。
	 返回值 成功则返回0，若有错误则返回-1，错误原因存于errno.
	 */
	int flags = fcntl(fd, F_GETFL);
	if (flags == -1)
	{
		ret = flags;
		printf("func activate_nonblock() err:%d", ret);
		return ret;
	}
	/*
	 *  按位或，加上没有锁的状态
	 * */
	flags |= O_NONBLOCK;
	/*
	 * . F_SETFL ：设置文件状态标志。
	 其中O_RDONLY， O_WRONLY， O_RDWR， O_CREAT， O_EXCL， O_NOCTTY 和 O_TRUNC不受影响，
	 可以更改的标志有 O_APPEND，O_ASYNC， O_DIRECT， O_NOATIME 和 O_NONBLOCK。
	 * */
	ret = fcntl(fd, F_SETFL, flags);
	if (ret == -1)
	{
		printf("func activate_nonblock() err:%d", ret);
		return ret;
	}
	return ret;
}

/**
 * deactivate_nonblock - 设置I/O为阻塞模式
 * @fd: 文件描符符
 */

int deactivate_nonblock(int fd)
{
	int ret = 0;
	/*
	 * int fcntl(int fd, int cmd, ... \* arg \*)
	 获取文件或者修改文件状态
	 F_GETLK 取得文件锁定的状态。
	 返回值 成功则返回0，若有错误则返回-1，错误原因存于errno.
	 */
	int flags = fcntl(fd, F_GETFL);
	if (flags == -1)
	{
		ret = flags;
		printf("func deactivate_nonblock() err:%d", ret);
		return ret;
	}

	/*
	 * 按位与， NONBLOCK的按位反 并上状态
	 * */
	flags &= ~O_NONBLOCK;
	/*
	 * . F_SETFL ：设置文件状态标志。
	 其中O_RDONLY， O_WRONLY， O_RDWR， O_CREAT， O_EXCL， O_NOCTTY 和 O_TRUNC不受影响，
	 可以更改的标志有 O_APPEND，O_ASYNC， O_DIRECT， O_NOATIME 和 O_NONBLOCK。
	 * */
	ret = fcntl(fd, F_SETFL, flags);
	if (ret == -1)
	{
		printf("func deactivate_nonblock() err:%d", ret);
		return ret;
	}
	return ret;
}

/**
 * connect_timeout - connect
 * @fd: 套接字
 * @addr: 要连接的对方地址
 * @wait_seconds: 等待超时秒数，如果为0表示正常模式
 * 成功（未超时）返回0，失败返回-1，超时返回-1并且errno = ETIMEDOUT
 */
/*
 *
 *struct sockaddr_in {
 *	 sa_family_t    sin_family; // address family: AF_INET
 *	 in_port_t      sin_port;   // port in network byte order
 *	 struct in_addr sin_addr;   // internet address
 *};
 * */
// static 修饰只能在这个文件内部使用。对外隐藏。

/*
 * 函数名：connect_timeout
 * 描述：客服端接受数据
 * 参数：
 *
 * 返回：
 * */
static int connect_timeout(int fd, struct sockaddr_in *addr,
		unsigned int wait_seconds)
{
	int ret;
	//获取socket结构体的大小。
	socklen_t addrlen = sizeof(struct sockaddr_in);
	//如果传入的等待时间大于0就取消socket的阻塞状态，0则不执行。
	if (wait_seconds > 0)
		activate_nonblock(fd);
	//链接
	/*
	 * int connect(int sockfd, const struct sockaddr *addr,socklen_t addrlen);
	 *
	 * */
	ret = connect(fd, (struct sockaddr*) addr, addrlen);
	//EINPROGRESS 正在处理
	if (ret < 0 && errno == EINPROGRESS)
	{
		/*
		 * void FD_CLR(int fd, fd_set *set);
		 * int  FD_ISSET(int fd, fd_set *set);
		 * void FD_SET(int fd, fd_set *set);
		 * void FD_ZERO(fd_set *set);
		 * */
		//设置监听集合
		fd_set connect_fdset;
		struct timeval timeout;
		//初始化集合
		FD_ZERO(&connect_fdset);
		//把fd 文件描述符的socket加入监听集合
		FD_SET(fd, &connect_fdset);
		/*
		 * struct timeval {
		 *     long    tv_sec;         // seconds       秒
		 *     long    tv_usec;        // microseconds  微妙
		 *     };
		 * */
		timeout.tv_sec = wait_seconds;
		timeout.tv_usec = 0;
		do
		{
			// 一但连接建立，则套接字就可写  所以connect_fdset放在了写集合中
			ret = select(fd + 1, NULL, &connect_fdset, NULL, &timeout);
		} while (ret < 0 && errno == EINTR);
		if (ret == 0)
		{
			ret = -1;
			/*
			 * #define ETIMEDOUT       110     // Connection timed out
             *  Tcp是面向连接的。在程序中表现为，当tcp检测到对端socket不再可
             *  用时(不能发出探测包，或探测包没有收到ACK的响应包)，select会
             *  返回socket可读，并且在recv时返回-1，同时置上errno为ETIMEDOUT。
			 * */
			errno = ETIMEDOUT;
		} else if (ret < 0)
			return -1;
		else if (ret == 1)
		{
			//printf("22222222222222222\n");
			/* ret返回为1（表示套接字可写），可能有两种情况，一种是连接建立成功，一种是套接字产生错误，*/
			/* 此时错误信息不会保存至errno变量中，因此，需要调用getsockopt来获取。 */
			int err;
			socklen_t socklen = sizeof(err);
			//获取socket的状态
			int sockoptret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &err,
					&socklen);
			if (sockoptret == -1)
			{
				return -1;
			}
			if (err == 0)
			{
				ret = 0;
			} else
			{
				errno = err;
				ret = -1;
			}
		}
	}
	if (wait_seconds > 0)
	{
		deactivate_nonblock(fd);
	}
	return ret;
}

/*
 * 函数名：sckCliet_getconn
 * 描述：客服端接受数据
 * 参数：
 *
 * 返回：
 * */
int sckCliet_getconn(void *handle, char *ip, int port, int *connfd)
{

	int ret = 0;
	SckHandle *tmp = NULL;
	if (handle == NULL || ip == NULL || connfd == NULL || port < 0
			|| port > 65537)
	{
		ret = Sck_ErrParam;
		printf(
				"func sckCliet_getconn() err: %d, check  (handle == NULL || ip==NULL || connfd==NULL || port<0 || port>65537) \n",
				ret);
		return ret;
	}

	//
	int sockfd;
	sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd < 0)
	{
		ret = errno;
		printf("func socket() err:  %d\n", ret);
		return ret;
	}

	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	servaddr.sin_addr.s_addr = inet_addr(ip);

	tmp = (SckHandle*) handle;

	/*
	 ret = connect(sockfd, (struct sockaddr*) (&servaddr), sizeof(servaddr));
	 if (ret < 0)
	 {
	 ret = errno;
	 printf("func connect() err:  %d\n", ret);
	 return ret;
	 }
	 */

	ret = connect_timeout(sockfd, (struct sockaddr_in*) (&servaddr),
			(unsigned int) tmp->contime);
	if (ret < 0)
	{
		if (ret == -1 && errno == ETIMEDOUT)
		{
			ret = Sck_ErrTimeOut;
			return ret;
		} else
		{
			printf("func connect_timeout() err:  %d\n", ret);
		}
	}

	*connfd = sockfd;

	return ret;

}

/**
 * write_timeout - 写超时检测函数，不含写操作
 * @fd: 文件描述符
 * @wait_seconds: 等待超时秒数，如果为0表示不检测超时
 * 成功（未超时）返回0，失败返回-1，超时返回-1并且errno = ETIMEDOUT
 */
/*
 * 函数名：write_timeout
 * 描述：客服端接受数据
 * 参数：
 *
 * 返回：
 * */
int write_timeout(int fd, unsigned int wait_seconds)
{
	int ret = 0;
	if (wait_seconds > 0)
	{
		fd_set write_fdset;
		struct timeval timeout;

		FD_ZERO(&write_fdset);
		FD_SET(fd, &write_fdset);

		timeout.tv_sec = wait_seconds;
		timeout.tv_usec = 0;
		do
		{
			ret = select(fd + 1, NULL, &write_fdset, NULL, &timeout);
		} while (ret < 0 && errno == EINTR);

		if (ret == 0)
		{
			ret = -1;
			errno = ETIMEDOUT;
		} else if (ret == 1)
			ret = 0;
	}

	return ret;
}

//客户端发送报文
/*
 * 函数名：sckClient_send
 * 描述：客服端接受数据
 * 参数：
 *
 * 返回：
 * */
int sckClient_send(void *handle, int connfd, unsigned char *data, int datalen)
{
	int ret = 0;

	SckHandle *tmp = NULL;
	tmp = (SckHandle *) handle;
	ret = write_timeout(connfd, tmp->sendtime);
	if (ret == 0)
	{
		int writed = 0;
		unsigned char *netdata = (unsigned char *) malloc(datalen + 4);
		if (netdata == NULL)
		{
			ret = Sck_ErrMalloc;
			printf("func sckClient_send() mlloc Err:%d\n ", ret);
			return ret;
		}
		int netlen = htonl(datalen);
		memcpy(netdata, &netlen, 4);
		memcpy(netdata + 4, data, datalen);

		writed = writen(connfd, netdata, datalen + 4);
		if (writed < (datalen + 4))
		{
			if (netdata != NULL)
			{
				free(netdata);
				netdata = NULL;
			}
			return writed;
		}

	}

	if (ret < 0)
	{
		//失败返回-1，超时返回-1并且errno = ETIMEDOUT
		if (ret == -1 && errno == ETIMEDOUT)
		{
			ret = Sck_ErrTimeOut;
			printf("func sckClient_send() mlloc Err:%d\n ", ret);
			return ret;
		}
		return ret;
	}

	return ret;
}

/**
 * read_timeout - 读超时检测函数，不含读操作
 * @fd: 文件描述符
 * @wait_seconds: 等待超时秒数，如果为0表示不检测超时
 * 成功（未超时）返回0，失败返回-1，超时返回-1并且errno = ETIMEDOUT
 */
//客户端端接受报文
/*
 * 函数名：read_timeout
 * 描述：客服端接受数据
 * 参数：
 *
 * 返回：
 * */
int read_timeout(int fd, unsigned int wait_seconds)
{
	int ret = 0;
	if (wait_seconds > 0)
	{
		fd_set read_fdset;
		struct timeval timeout;

		FD_ZERO(&read_fdset);
		FD_SET(fd, &read_fdset);

		timeout.tv_sec = wait_seconds;
		timeout.tv_usec = 0;

		//select返回值三态
		//1 若timeout时间到（超时），没有检测到读事件 ret返回=0
		//2 若ret返回<0 &&  errno == EINTR 说明select的过程中被别的信号中断（可中断睡眠原理）
		//2-1 若返回-1，select出错
		//3 若ret返回值>0 表示有read事件发生，返回事件发生的个数

		do
		{
			ret = select(fd + 1, &read_fdset, NULL, NULL, &timeout);
		} while (ret < 0 && errno == EINTR);

		if (ret == 0)
		{
			ret = -1;
			errno = ETIMEDOUT;
		} else if (ret == 1)
			ret = 0;
	}

	return ret;
}

//客户端端接受报文
/*
 * 函数名：sckClient_rev
 * 描述：客服端接受数据
 * 参数：
 *
 * 返回：
 * */
int sckClient_rev(void *handle, int connfd, unsigned char *out, int *outlen)
{

	int ret = 0;
	SckHandle *tmpHandle = (SckHandle *) handle;

	if (handle == NULL || out == NULL)
	{
		ret = Sck_ErrParam;
		printf("func sckClient_rev() timeout , err:%d \n", Sck_ErrTimeOut);
		return ret;
	}

	ret = read_timeout(connfd, tmpHandle->revtime); //bugs modify bombing
	if (ret != 0)
	{
		if (ret == -1 || errno == ETIMEDOUT)
		{
			ret = Sck_ErrTimeOut;
			printf("func sckClient_rev() timeout , err:%d \n", Sck_ErrTimeOut);
			return ret;
		} else
		{
			printf("func sckClient_rev() timeout , err:%d \n", Sck_ErrTimeOut);
			return ret;
		}
	}

	int netdatalen = 0;
	ret = readn(connfd, &netdatalen, 4); //读包头 4个字节
	if (ret == -1)
	{
		printf("func readn() err:%d \n", ret);
		return ret;
	} else if (ret < 4)
	{
		ret = Sck_ErrPeerClosed;
		printf("func readn() err peer closed:%d \n", ret);
		return ret;
	}

	int n;
	n = ntohl(netdatalen);
	ret = readn(connfd, out, n); //根据长度读数据
	if (ret == -1)
	{
		printf("func readn() err:%d \n", ret);
		return ret;
	} else if (ret < n)
	{
		ret = Sck_ErrPeerClosed;
		printf("func readn() err peer closed:%d \n", ret);
		return ret;
	}

	*outlen = n;

	return 0;
}

// 客户端环境释放 
int sckClient_destroy(void *handle)
{
	if (handle != NULL)
	{
		free(handle);
	}
	return 0;
}

int sckCliet_closeconn(int connfd)
{
	if (connfd >= 0)
	{
		close(connfd);
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////
//函数声明
//服务器端初始化
/*
 * 函数名：sckServer_init
 * 描述：服务器端的socket初始化
 * 参数： port 绑定的端口
 *       listenfd 监听的socket文件
 * 返回：如果成功返回0 ，失败返回<0 或者 成功发送的数据的字节大小。
 * */
int sckServer_init(int port, int *listenfd)
{
	int ret = 0;
	int mylistenfd;
	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    //返回一个新的socket描述符
	mylistenfd = socket(PF_INET, SOCK_STREAM, 0);
	if (mylistenfd < 0)
	{
		ret = errno;
		printf("func socket() err:%d \n", ret);
		return ret;
	}

	int on = 1;
	ret = setsockopt(mylistenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	if (ret < 0)
	{
		ret = errno;
		printf("func setsockopt() err:%d \n", ret);
		return ret;
	}

	ret = bind(mylistenfd, (struct sockaddr*) &servaddr, sizeof(servaddr));
	if (ret < 0)
	{
		ret = errno;
		printf("func bind() err:%d \n", ret);
		return ret;
	}

	ret = listen(mylistenfd, SOMAXCONN);
	if (ret < 0)
	{
		ret = errno;
		printf("func listen() err:%d \n", ret);
		return ret;
	}

	*listenfd = mylistenfd;

	return 0;
}

/**
 * accept_timeout - 带超时的accept
 * @fd: 套接字
 * @addr: 输出参数，返回对方地址
 * @wait_seconds: 等待超时秒数，如果为0表示正常模式
 * 成功（未超时）返回已连接套接字，超时返回-1并且errno = ETIMEDOUT
 */
int accept_timeout(int fd, struct sockaddr_in *addr, unsigned int wait_seconds)
{
	int ret;
	socklen_t addrlen = sizeof(struct sockaddr_in);

	if (wait_seconds > 0)
	{
		fd_set accept_fdset;
		struct timeval timeout;
		FD_ZERO(&accept_fdset);
		FD_SET(fd, &accept_fdset);
		timeout.tv_sec = wait_seconds;
		timeout.tv_usec = 0;
		do
		{
			ret = select(fd + 1, &accept_fdset, NULL, NULL, &timeout);
		} while (ret < 0 && errno == EINTR);
		if (ret == -1)
			return -1;
		else if (ret == 0)
		{
			errno = ETIMEDOUT;
			return -1;
		}
	}

	//一但检测出 有select事件发生，表示对等方完成了三次握手，客户端有新连接建立
	//此时再调用accept将不会堵塞
	if (addr != NULL)
		ret = accept(fd, (struct sockaddr*) addr, &addrlen); //返回已连接套接字
	else
		ret = accept(fd, NULL, NULL);
	if (ret == -1)
	{
		ret = errno;
		printf("func accept() err:%d \n", ret);
		return ret;
	}

	return ret;
}
/*
 * 函数名：sckServer_accept
 * 描述：服务器端等待数据
 * 参数： listenfd 监听的sock
 * 	     timeout 定义的超时时间
 * 返回：如果成功返回0 ，失败返回<0 或者 成功发送的数据的字节大小。
 * */

int sckServer_accept(int listenfd, int *connfd, int timeout)
{
	int ret = 0;
    //
	ret = accept_timeout(listenfd, NULL, (unsigned int) timeout);
	if (ret < 0)
	{
		if (ret == -1 && errno == ETIMEDOUT)
		{
			ret = Sck_ErrTimeOut;
			printf("func accept_timeout() timeout err:%d \n", ret);
			return ret;
		} else
		{
			ret = errno;
			printf("func accept_timeout() err:%d \n", ret);
			return ret;
		}
	}

	*connfd = ret;
	return 0;
}
//服务器端发送报文
/*
 * 函数名：sckServer_send
 * 描述：发送报文，并进行了粘包处理。
 * 参数： connfd 链接的socket描述符
 * 	     data 发送的数据  ，传入数据，在内部重新打包封装。
 * 	     datalen 要发送的数据的长度
 * 	     timeout 定义的超时时间
 * 返回：如果成功返回0 ，失败返回<0 或者 成功发送的数据的字节大小。
 * */
int sckServer_send(int connfd, unsigned char *data, int datalen, int timeout)
{
	int ret = 0;
    //写时超时检测
	ret = write_timeout(connfd, timeout);
	if (ret == 0)
	{
		int writed = 0;
		//分配内存空间
		unsigned char *netdata = (unsigned char *) malloc(datalen + 4);
		if (netdata == NULL)
		{
			ret = Sck_ErrMalloc;
			printf("func sckServer_send() mlloc Err:%d\n ", ret);
			return ret;
		}
		//将本地数据转换为网络数据  ;小端===》大端
		int netlen = htonl(datalen);
		//将数据的长度加到数据包的头4字节处
		memcpy(netdata, &netlen, 4);
		//将数据打包到新的数据包中。
		memcpy(netdata + 4, data, datalen);
        //发送数据
		//writed为成功发送的数据的字节长度。
		writed = writen(connfd, netdata, datalen + 4);
		//直到数据分包 封装 发送完成之后，返回
		if (writed < (datalen + 4))
		{
			//释放内存
			if (netdata != NULL)
			{
				free(netdata);
				netdata = NULL;
			}
			return writed;
		}

	}
    //检测超时
	if (ret < 0)
	{
		//失败返回-1，超时返回-1并且errno = ETIMEDOUT
		//链接超时
		if (ret == -1 && errno == ETIMEDOUT)
		{
			ret = Sck_ErrTimeOut;
			printf("func sckServer_send() mlloc Err:%d\n ", ret);
			return ret;
		}
		return ret;
	}

	return ret;
}
//服务器端端接受报文
/*
 * 函数名：sckServer_rev
 * 描述：接受报文，并进行了粘包处理。
 * 参数： connfd 链接的socket描述符
 * 	     out 读取的内容，在外部分配内存
 * 	     outlen 读取到内容的长度。
 * 	     timeout 定义的超时时间
 * 返回：如果成功返回0 ，失败返回<0 或者错误码。
 * */
int sckServer_rev(int connfd, unsigned char *out, int *outlen, int timeout)
{

	int ret = 0;
    //检测传入的参数是否是有效的参数。
	if (out == NULL || outlen == NULL)
	{
		ret = Sck_ErrParam;
		printf("func sckClient_rev() timeout , err:%d \n", Sck_ErrTimeOut);
		return ret;
	}
    //检测是否可读，防止阻塞假死，一个链接的等待时间是1.5倍的RTT 一个RTT 75秒
	ret = read_timeout(connfd, timeout); //bugs modify bombing
	if (ret != 0)
	{
		if (ret == -1 || errno == ETIMEDOUT)
		{
			ret = Sck_ErrTimeOut;
			printf("func sckClient_rev() timeout , err:%d \n", Sck_ErrTimeOut);
			return ret;
		} else
		{
			printf("func sckClient_rev() timeout , err:%d \n", Sck_ErrTimeOut);
			return ret;
		}
	}
	/*
	 * 防止粘包
	 * */

    //定义收取的数据的长度，以用来获取收取数据的长度，初始化为0；
	//通过调用readn返回数据的长度
	int netdatalen = 0;
	ret = readn(connfd, &netdatalen, 4); //读包头 4个字节
	if (ret == -1)
	{
		printf("func readn() err:%d \n", ret);
		return ret;
	} else if (ret < 4)
	{
		ret = Sck_ErrPeerClosed;
		printf("func readn() err peer closed:%d \n", ret);
		return ret;
	}
	int n;
	//将网络数据转换为本地数据，大端===>小端
	n = ntohl(netdatalen);
	ret = readn(connfd, out, n); //根据长度读数据
	if (ret == -1)
	{
		printf("func readn() err:%d \n", ret);
		return ret;
	} else if (ret < n)
	{
		ret = Sck_ErrPeerClosed;
		printf("func readn() err peer closed:%d \n", ret);
		return ret;
	}
    //抛出需要读取的字节长度。
	*outlen = n;
	return 0;
}

//服务器端环境释放 
int sckServer_destroy(void *handle)
{
     if(handle!=NULL)
     {
    	 free(handle);
    	 handle=NULL;//没有起作用。
     }
	return 0;
}
