#include "../manger.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 8888
#define IP "0.0.0.0"

int send_recv(int sfd); // 发送接收函数
int login(int sfd);		// 登录函数
void cancel(int sfd);	// 退出函数
int root(int sfd);		// 管理员界面
int user(int sfd);		// 普通用户界面
int sigup(int sfd);		// 注册员工信息
int delusr(int sfd);	// 删除员工信息
int chang(int sfd);		// 修改员工信息
int find(int sfd);		// 查寻员工信息

int flag = 1;
struct msg_st msg;

int main(int argc, char *argv[])
{
	int sfd, res, newsfd;
	char choose;
	struct sockaddr_in sec;
	struct sockaddr_in cli;
	socklen_t sec_len = sizeof(sec);

	sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sfd < 0)
	{
		ERR("socket");
		return -1;
	}

	sec.sin_family = AF_INET;
	sec.sin_port = htons(PORT);
	sec.sin_addr.s_addr = inet_addr(IP);
	res = connect(sfd, (struct sockaddr *)&sec, sec_len);
	if (res)
	{
		ERR("connect");
		return -2;
	}
	while (1)
	{
		system("clear");
		flag = 1;
		printf("**************************************\n");
		printf("***********1 登录*********************\n");
		printf("***********2 退出*********************\n");
		printf("**************************************\n");
		printf("请输入\n");
		choose = getchar();
		getchar();
		switch (choose - 48)
		{
		case LOGIN:
			res = login(sfd);
			if (res < 0)
			{
				printf("登录失败\n");
			}
			else if (res > 0)
			{
				// 管理员界面
				while (flag)
				{
					res = root(sfd);
				}
			}
			else
			{
				// 普通用户界面
				while (flag)
				{
					res = user(sfd);
				}
			}

			break;
		case EXIT:
			exit(0);
			break;
		default:
			printf("输入错误，请重新输入\n");
			break;
		}
	
	}

	close(sfd);
	return 0;
}

int send_recv(int sfd)
{
	int res;
	res = send(sfd, &msg, sizeof(msg), 0);
	if (res < 0)
	{
		ERR("send");
		return -1;
	}
	res = recv(sfd, &msg, sizeof(msg), 0);
	if (res < 0)
	{
		ERR("recv");
		return -2;
	}
	else if (res == 0)
	{
		printf("%d-服务器关闭\n", __LINE__);
		return -3;
	}

	return 0;
}

int login(int sfd)
{

	int res;
	char id[IDSIZE] = "";
	char password[PASSWORDSIZE] = "";

	memset(&msg, 0, sizeof(msg));
	msg.opt = LOGIN;
	printf("请输入ID  : ");
	fgets(id, sizeof(id), stdin);
	id[strlen(id) - 1] = 0;
	printf("请输入密码 : ");
	fgets(password, sizeof(password), stdin);
	password[strlen(password) - 1] = 0;

	strcpy(msg.peoplemsg.id, id);
	strcpy(msg.peoplemsg.password, password);

	res = send_recv(sfd);
	if (res < 0)
	{
		return res;
	}

	if (strncasecmp(msg.msg, "OK", 2))
	{
		printf("%d-%s\n", __LINE__, msg.msg);
		return -4;
	}

	return msg.permissions; // 管理员成功返回1 普通员工返回0
}

void cancel(int sfd)
{
	int res;
	msg.opt = CANCEL;
	send_recv(sfd);
}

int root(int sfd)
{
	char choose;

	printf("***********************************************\n");
	printf("**************3 注册用户************************\n");
	printf("**************4 删除用户************************\n");
	printf("**************5 查找用户************************\n");
	printf("**************6 修改信息************************\n");
	printf("**************7  上一级*************************\n");
	printf("***********************************************\n");
	printf("请输入\n");
	choose = getchar();
	getchar();
	switch (choose - 48)
	{
	case SIGUP:
		sigup(sfd);
		break;
	case DELUSR:
		delusr(sfd);
		break;
	case CHANG:
		chang(sfd);
		break;
	case FIND:
		find(sfd);
		break;
	case CANCEL:
		flag = 0;
		cancel(sfd);
		break;
	default:
		printf("输入错误，请重新输入\n");
	}
	
	return 0;
}

int sigup(int sfd)
{
	int res;
	bzero(msg.peoplemsg.name, sizeof(msg.peoplemsg.name));
	bzero(msg.peoplemsg.department, sizeof(msg.peoplemsg.department));

	printf("请输入名字 ： ");
	fgets(msg.peoplemsg.name, sizeof(msg.peoplemsg.name), stdin);
	msg.peoplemsg.name[strlen(msg.peoplemsg.name) - 1] = 0;
	printf("请输入部门 ： ");
	fgets(msg.peoplemsg.department, sizeof(msg.peoplemsg.department), stdin);
	msg.peoplemsg.department[strlen(msg.peoplemsg.department) - 1] = 0;

	msg.opt = SIGUP;

	res = send_recv(sfd);
	if (res < 0)
	{
		return res;
	}

	if (strncasecmp(msg.msg, "OK", 2))
	{
		printf("%d-%s\n", __LINE__, msg.msg);
		return -4;
	}

	printf("注册成功\n");
	printf("id:%s\n密码:%s\n姓名:%s\n部门:%s\n", msg.peoplemsg.id, msg.peoplemsg.password,
		   msg.peoplemsg.name, msg.peoplemsg.department);
	return 0;
}

int delusr(int sfd)
{
	int res;
	bzero(msg.peoplemsg.id, sizeof(msg.peoplemsg.id));
	bzero(msg.msg, sizeof(msg.msg));
	printf("请输入要删除的id：");
	fgets(msg.peoplemsg.id, sizeof(msg.peoplemsg.id), stdin);
	msg.peoplemsg.id[strlen(msg.peoplemsg.id) - 1] = 0;
	msg.opt = DELUSR;
	res = send_recv(sfd);
	if (res < 0)
	{
		return res;
	}

	if (strncasecmp(msg.msg, "OK", 2))
	{
		printf("%d-%s\n", __LINE__, msg.msg);
		return -4;
	}
	printf("%s 删除成功\n", msg.peoplemsg.id);
	return 0;
}

int chang(int sfd)
{
	int res;
	char choose;
	
	memset(&(msg.peoplemsg), 0, sizeof(msg.peoplemsg));
	memset(&(msg.chang), 0, sizeof(msg.chang));
	if (msg.permissions == ROOT)
	{
		printf("***********************************************\n");
		printf("**************1 请输入要修改的ID******************\n");
		printf("**************2  空格返回上一级*************************\n");
		printf("***********************************************\n");
		printf("请输入\n");
		fgets(msg.chang.changid, sizeof(msg.chang.changid), stdin);
		if (*msg.chang.changid == ' ')
			return 0;
		msg.chang.changid[strlen(msg.chang.changid) - 1] = 0;
	}
	//判断id
	if (msg.permissions == USER && strcmp(msg.chang.changid, msg.idself))
	{
		printf("%d-你无权限修改此id信息\n", __LINE__);
		return -1;
	}
	printf("请选择修改内容\n");
	printf("***********************************************\n");
	printf("***************  空格返回上一级******************\n");
	printf("**************1 名字****************************\n");
	printf("**************2 年龄****************************\n");
	printf("**************3 性别****************************\n");
	printf("**************4 住址****************************\n");
	printf("**************5 电话****************************\n");
	printf("**************6 密码****************************\n");
	if (msg.permissions == ROOT)
	{
		printf("**************7 部门*************************\n");
		printf("**************8 薪资*************************\n");
	}
	printf("***********************************************\n");
	printf("请输入\n");
	choose = getchar();
	getchar();
	switch (choose - 48)
	{
	case 1:
		strcpy(msg.chang.changkey, "id");
		printf("请输入新名字 ： ");
		break;
	case 2:
		strcpy(msg.chang.changkey, "age");
		printf("请输入年龄 ： ");
		break;
	case 3:
		strcpy(msg.chang.changkey, "sex");
		printf("请输入性别 ： ");
		break;
	case 4:
		strcpy(msg.chang.changkey, "address");
		printf("请输入新住址 ： ");
		break;
	case 5:
		strcpy(msg.chang.changkey, "phone");
		printf("请输入新电话 ： ");
		break;
	case 6:
		strcpy(msg.chang.changkey, "password");
		printf("请输入新密码 ： ");
		break;
	case 7:
		strcpy(msg.chang.changkey, "department");
		printf("请输入新部门 ： ");
		break;
	case 8:
		strcpy(msg.chang.changkey, "money");
		printf("请输入新薪资 ： ");
		break;
	default:
		printf("输入错误请重新输入");
		return -1;
	}
	fgets(msg.chang.changvalues, sizeof(msg.chang.changvalues), stdin);
	if (*msg.chang.changvalues == ' ')
			return 0;
	msg.chang.changvalues[strlen(msg.chang.changvalues) - 1] = 0;

	msg.opt = CHANG;
	res = send_recv(sfd);
	if (res < 0)
	{
		return res;
	}

	if (strncasecmp(msg.msg, "OK", 2))
	{
		printf("%d-%s\n", __LINE__, msg.msg);
		return -4;
	}
	
	return 0;
}

int find(int sfd)
{

	char choose;
	int res;
	bzero(msg.peoplemsg.name, sizeof(msg.peoplemsg.name));
	bzero(msg.peoplemsg.department, sizeof(msg.peoplemsg.department));
	bzero(msg.peoplemsg.id, sizeof(msg.peoplemsg.id));
	bzero(msg.msg, sizeof(msg.msg));

	printf("***********************************************\n");
	printf("**************1 ID查找**************************\n");
	printf("**************2 名字查找************************\n");
	printf("**************3 部门查找************************\n");
	printf("**************4 全部查找************************\n");
	printf("***********************************************\n");
	printf("请输入\n");
	choose = getchar();
	getchar();
	switch (choose - 48)
	{
	case FIND_ID:
		printf("请输入要查询的id :");
		fgets(msg.peoplemsg.id, sizeof(msg.peoplemsg.id), stdin);
		msg.peoplemsg.id[strlen(msg.peoplemsg.id) - 1] = 0;
		strcpy(msg.msg, "id");
		break;
	case FIND_NAME:
		printf("请输入要查询的名字 :");
		fgets(msg.peoplemsg.name, sizeof(msg.peoplemsg.name), stdin);
		msg.peoplemsg.name[strlen(msg.peoplemsg.name) - 1] = 0;
		strcpy(msg.msg, "name");
		break;
	case FIND_DEP:
		printf("请输入要查询的部门 :");
		fgets(msg.peoplemsg.department, sizeof(msg.peoplemsg.department), stdin);
		msg.peoplemsg.department[strlen(msg.peoplemsg.department) - 1] = 0;
		strcpy(msg.msg, "department");
		break;
	case FIND_ALL:
		break;
	default:
		printf("无效输入\n");
		return -1;
	}
	msg.opt = FIND;
	res = send(sfd, &msg, sizeof(msg), 0);
	if (res < 0)
	{
		ERR("send");
		return -1;
	}
	printf("%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s\n", "ID", "名字", "年龄", "性别", "部门", "住址", "电话", "薪资", "密码", "状态");
	printf("--------------------------------------------------------------------------\n");
	while (1)
	{
		res = recv(sfd, &msg, sizeof(msg), 0);
		if (res < 0)
		{
			ERR("recv");
			return -2;
		}
		else if (res == 0)
		{
			printf("%d-服务器关闭\n", __LINE__);
			return -3;
		}
		if (!(strncasecmp(msg.msg, "OK", 2)))
		{
			printf("%s\n", msg.msg + 2);
			break;
		}
		else
		{

			printf("%-10s%-10s%-10d%-10c%-10s%-10s%-10s%-10d%-10s%-10d\n", msg.peoplemsg.id,
				   msg.peoplemsg.name, msg.peoplemsg.age, msg.peoplemsg.sex, msg.peoplemsg.department,
				   msg.peoplemsg.address, msg.peoplemsg.phone, msg.peoplemsg.money, msg.peoplemsg.password,
				   msg.peoplemsg.status);
			printf("--------------------------------------------------------------------------\n");
		}
	}

	return 0;
}

int user(int sfd)
{
	char choose;
	
	printf("***********************************************\n");
	printf("**************5 查寻信息************************\n");
	printf("**************6 修改信息************************\n");
	printf("**************7  上一级*************************\n");
	printf("***********************************************\n");
	printf("请输入\n");
	choose = getchar();
	getchar();
	switch (choose - 48)
	{
	case CHANG:
		chang(sfd);
		break;
	case FIND:
		find(sfd);
		break;
	case CANCEL:
		flag = 0;
		cancel(sfd);
		break;
	default:
		printf("输入错误，请重新输入\n");
	}
	
	return 0;
}
