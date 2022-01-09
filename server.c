//http://research.nii.ac.jp/~ichiro/syspro98/wwwserver.html
//fdはファイルディスクリプタ
//ファイルディスクリプタとは、プログラムからファイルを操作する際、操作対象のファイルを識別・同定するために割り当てられる番号。

//fprintf関数は、ファイルへの書き込みを行う。
//fprintf(output stream, formatted string);

//ファイルディスクリプタのライブラリ
#include <sys/fcntl.h>

//ソケット用ライブラリ
#include <sys/socket.h>

//データタイプ用ライブラリ
#include <sys/types.h>

//インターネットアドレスファミリ
#include <netinet/in.h>

//ネットワークデータベース
#include <netdb.h>

//いつもの
#include <stdio.h>

//UNIX標準に関するヘッダファイル
#include <unistd.h>

#include <string.h>


#define HTTP_TCP_PORT 8080

//プロトタイプ宣言のあれ
void http(int sockfd);
int send_msg(int fd, char *msg);




int main(){
	int sockfd, new_sockfd;
	int writer_len;
	int flag;

	//sockaddr_inは、接続先のIPアドレスやポート番号を保持するための構造体。
	struct sockaddr_in reader_addr, writer_addr;

	//bzero関数。メモリ領域をゼロにする。第一引数はメモリ領域の開始アドレス。第二引数はバイト数。
	bzero((char *) &reader_addr, sizeof(reader_addr));
	
	//ソケットが通信できるアドレスのタイプを指定
	reader_addr.sin_family=AF_INET;

	//コンピュータのIPアドレス(INADDR_ANY)をhtonl関数でホストバイトオーダーからネットワークバイトオーダーに変換して指定。
	reader_addr.sin_addr.s_addr=htonl(INADDR_ANY);

	//HTTP_TCP_PORTをhtons関数でネットワークバイトオーダーへ符号なし短整数変換を行って指定。
	reader_addr.sin_port=htons(HTTP_TCP_PORT);

	//ソケット生成。
	//int socket(int domain, int type, int protocol);
	//第一引数、どのプロトコルファミリーを通信に使うか。第二引数、通信方式。第三引数、ソケットが使用するプロトコルを指定。
	//https://qiita.com/Michinosuke/items/0778a5344bdf81488114
	//fprintfはストリームにフォーマットされた文字列を出力する
	if ((sockfd=socket(AF_INET, SOCK_STREAM, 0))<0){
		fprintf(stderr, "error: socket()\n");
		exit(1);
	}

	//ポート番号をソケットに割当る
	//int bind(int sockfd, const struct sockaddr *addr, socklen_t, addrlen);
	//第一引数、ソケットを示すファイルディスクリプタ。第二引数、ソケットに割り当てるアドレス。第三引数、addrの指す構造体のサイズ。
	//https://qiita.com/Michinosuke/items/0778a5344bdf81488114
	if (bind(sockfd, (struct sockaddr *)&reader_addr, sizeof(reader_addr))<0){
		fprintf(stderr, "error: bind()\n");
		close(sockfd);
		exit(1);
	}

	flag=1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&flag, sizeof(flag))<0){
		fprintf(stderr, "error: setsocketopt()\n");
		exit(1);
	}

	//ソケットを待ち状態へ
	//int listen(int sockfd, int backlog);
	//第一引数、省略。第二引数、保留中のキューの最大長。
	if (listen(sockfd,5)<0){
		fprintf(stderr, "error: listen()\n");
		close(sockfd);
		exit(1);
	}

	//acceptは、クライアントから通信接続要求が来るまでプログラムを停止し、接続後にプログラムを再開する関数。
	//accept(int sockfd, struct sockaddr *addr, socklen_t *addrken);
	//第一引数、省略。第二引数、接続した相手のIPアドレスやポート番号などの情報。第三引数、addrの指す構造体のサイズ。
	while(1){
		if ((new_sockfd=accept(sockfd,(struct sockaddr *)&writer_addr,(socklen_t *)&writer_len))<0){
			fprintf(stderr, "error: accepting a socket.\n");
			break;
		}
		else{
			http(new_sockfd);
			close(new_sockfd);
		}
	}
	close(sockfd);
	return 0;
}
	


void http(int sockfd){
	int len;
	int read_fd;
	char buf[1024];

	//method name
	char meth_name[16];

	char uri_addr[256];
	char http_ver[64];
	char *uri_file;


	//データの受信
	//ssize_t read(int fd, void *buf, size_t count);
	//第一引数、読み込み/書き込み先を示すファイルディスクリプタ。第二引数、読み込み/書き込み先を示すバッファ。第三引数、読み込み/書き込みの最大バイト数。
	//ssize_tは確保されたメモリ領域のサイズを表す変数型。
	if (read(sockfd, buf, 1024)<=0){
		fprintf(stderr, "error: reading a request.\n");
	}else{
		//sscanf関数、文字列から書式指定に従い入力
		//"GET /index.html HTTP/1.1"的なやつ
		sscanf(buf, "%s %s %s", meth_name, uri_addr, http_ver);
		if (strcmp(meth_name, "GET")==0){
			//URIの/をなくすための+1?
			uri_file = uri_addr+1;
			//int open(const char *path, int mode, [mode_t creat_mode])
			//第一引数、path。第二引数、許可を与えるモード。第三引数、O_CREATの場合にオープンするファイルのモード。
			if ((read_fd=open(uri_file, O_RDONLY, 0666))==-1){
				send_msg(sockfd, "HTTP/1.0 404 Not Found\r\n");
				send_msg(sockfd, "text/html\r\n");
				send_msg(sockfd, "\r\n");
				send_msg(sockfd, "404 Not Found\r\n");
			}else{
				send_msg(sockfd, "HTTP/1.0 200 OK\r\n");
				send_msg(sockfd, "text/html\r\n");
				send_msg(sockfd, "\r\n");
				while ( (len=read(read_fd, buf, len))>0 ){
					if (write(sockfd, buf, len)!=len){
						fprintf(stderr, "error: writing a response.\n");
						break;
					}
				}
				//ファイルのファイルディスクリプタをcloseする。
				close(read_fd);
		}
		}
		else{
			send_msg(sockfd, "501 Not Implemented");
		}
	}
}


int send_msg(int fd, char *msg){
	int len;
	//strlenは、NULL(\0)の直前までの文字数を求める。
	len = strlen(msg);
	if (write(fd, msg, len)!=len){
		fprintf(stderr, "error: writing.");
	}
	return len;
}