#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "../manger.h"

#define PORT 8888
#define IP "0.0.0.0"

int sqlite3_init(sqlite3 **sq); // 初始化数据库
void chang(struct msg_st *msg, sqlite3 *sq);
void find(struct msg_st *msg, sqlite3 *sq, int sfd);
void delusr(struct msg_st *msg, sqlite3 *sq);
void cancel(struct msg_st *msg, sqlite3 *sq);
void sigup(struct msg_st *msg, sqlite3 *sq);
void login(struct msg_st *msg, sqlite3 *sq);
void change_status(sqlite3 *sq, char *id, int status); // 改变用户状态

struct choose_st
{
  int sfd;
  FIND_RET find_ret;
  int flag;
  struct msg_st msgcallback;
};

int main(int argc, char *argv[])
{
  int sfd, res, newsfd, epfd, epret, i;
  int reuse = 1, numfd = 1;
  struct sockaddr_in sec;
  struct sockaddr_in cli;
  socklen_t cli_len = sizeof(cli);
  struct epoll_event event, revcevent[10];
  char buf[32] = "";
  sqlite3 *sq = NULL;
  struct msg_st msg;

  printf("初始化系统。。。\n");
  res = sqlite3_init(&sq);
  if (res)
  {
    printf("%d-sqlite3_init error\n", __LINE__);
    return -1;
  }
  printf("数据库初始化完成\n");
  sfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sfd < 0)
  {
    ERR("socket");
    return -1;
  }

  // 允许端口快速重用
  res = setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
  if (res < 0)
  {
    ERR("setsockopt");
    return -1;
  }

  sec.sin_family = AF_INET;
  sec.sin_port = htons(PORT);
  sec.sin_addr.s_addr = inet_addr(IP);
  res = bind(sfd, (struct sockaddr *)&sec, sizeof(sec));
  if (res)
  {
    ERR("bind");
    return -2;
  }

  res = listen(sfd, 5);
  if (res)
  {
    ERR("listen");
    return -3;
  }

  printf("网络初始化完成\n");

  epfd = epoll_create(1);
  if (epfd < 0)
  {
    ERR("epoll_create");
    return -5;
  }

  event.events = EPOLLIN;
  event.data.fd = sfd;
  res = epoll_ctl(epfd, EPOLL_CTL_ADD, sfd, &event);
  if (res)
  {
    ERR("epoll_ctl");
    return -6;
  }
  while (1)
  {
    epret = epoll_wait(epfd, revcevent, numfd, -1);
    if (epret <= 0)
    {
      ERR("epoll_wait");
      printf("errcode-%d\n", epret);
      return -7;
    }
    for (i = 0; i < epret; i++)
    {
      if (revcevent[i].data.fd == sfd)
      {
        newsfd = accept(sfd, (struct sockaddr *)&cli, &cli_len);
        if (newsfd < 0)
        {
          ERR("accept");
          return -4;
        }
        event.data.fd = newsfd;
        res = epoll_ctl(epfd, EPOLL_CTL_ADD, newsfd, &event);
        if (res)
        {
          ERR("epoll_ctl");
          return -6;
        }
        numfd++;
        printf("%s:%d 连接成功\n", inet_ntoa(cli.sin_addr), newsfd);
      }
      else
      {
        printf("%d %d\n", __LINE__, revcevent[i].data.fd);
        memset(&(msg.peoplemsg), 0, sizeof(msg.peoplemsg));
        memset(&(msg.chang), 0, sizeof(msg.chang));
        memset(msg.msg, 0, sizeof(msg.msg));
        res = recv(revcevent[i].data.fd, &msg, sizeof(msg), 0);
        printf("%d-%s\n", __LINE__, msg.idself);
        if (res == 0)
        {
          // event.data.fd = revcevent[i].data.fd;
          epoll_ctl(epfd, EPOLL_CTL_DEL, revcevent[i].data.fd,
                    &revcevent[i]);
          close(revcevent[i].data.fd);
          // 改变用户状态为离线
          change_status(sq, msg.idself, OFFLINE);
          printf("%d 下线\n", revcevent[i].data.fd);
          continue;
        }
        else if (res < 0)
        {
          ERR("recv");
          return -7;
        }
        else
        {
          printf("%d-opt-%d\n", __LINE__, msg.opt);
          switch (msg.opt)
          {
          case LOGIN:
            // printf("%d\n", __LINE__);
            login(&msg, sq);
            break;
          case EXIT:
            break;
          case SIGUP:
            sigup(&msg, sq);
            break;
          case DELUSR:
            delusr(&msg, sq);
            break;
          case CHANG:
            chang(&msg, sq);
            break;
          case FIND:
            find(&msg, sq, revcevent[i].data.fd);
            break;
          case CANCEL:
            cancel(&msg, sq);
            break;
          default:
            break;
          }
          res = send(revcevent[i].data.fd, &msg, sizeof(msg), 0);
          printf("%d %d\n", __LINE__, res);
          if (res < 0)
          {
            ERR("send");
            continue;
          }
        }
      }
    }
  }

  close(epfd);
  close(sfd);

  return 0;
}

int sqlite3_init(sqlite3 **sq)
{
  int res;
  char sql[SQLSIZE] =
      "create table if not exists Zhangscorporation(id char primary "
      "key, name char, age int, sex char, address char, department "
      "char, phone char, money int, password char, status char, maxid char)";
  char *errmsg = NULL;
  res = sqlite3_open(FILENAME, sq);
  if (res)
  {
    printf("%d-%s\n", __LINE__, sqlite3_errmsg(*sq));
    return res;
  }

  res = sqlite3_exec(*sq, sql, NULL, NULL, &errmsg);
  if (res)
  {
    printf("%d-%s\n", __LINE__, errmsg);
    return res;
  }

  // 设置管理员账号
  memset(sql, 0, sizeof(sql));
  sprintf(
      sql,
      "insert into Zhangscorporation(id,password,maxid, name, department, "
      "status, age, sex, address, phone, money) values ('%s', "
      "'%s', '0', '-', '-', 0, 0, '-', '-', '-', 0)",
      ROOTID, ROOTPASSWORD);
  res = sqlite3_exec(*sq, sql, NULL, NULL, &errmsg);
  if (res)
  {
    if (res != SQLITE_CONSTRAINT)
    {
      printf("%d-%s\n", __LINE__, errmsg);
      return res;
    }
  }

  // 把所有人状态设为离线
  res = sqlite3_exec(*sq, "update Zhangscorporation set status=0", NULL, NULL,
                     &errmsg);
  if (res)
  {
    printf("%d-%s\n", __LINE__, errmsg);
    return res;
  }

  return 0;
}

void login(struct msg_st *msg, sqlite3 *sq)
{
  // printf("%d %s\n", __LINE__, msg->peoplemsg.id);
  int res, i, j, k = 0;
  char sql[SQLSIZE] = "";
  sprintf(sql, "select password, status from Zhangscorporation where id='%s'",
          msg->peoplemsg.id);
  // printf("%d\n", __LINE__);
  // puts(sql);
  char **pres = NULL;
  int row, col;
  char *permsg = NULL;
  char *errmsg = NULL;
  char status;

  // printf("%d\n", __LINE__);
  res = sqlite3_get_table(sq, sql, &pres, &row, &col, &permsg);
  if (res)
  {
    printf("%d-sqlite3_get_table error : %s\n", __LINE__, permsg);
    strcpy(msg->msg, "系统获取数据错误");
    return;
  }
  /*
            for(i = 0; i < (row+1); i++)
            {
                    for(j = 0; j < col; j++)
                            printf("%s\n", pres[k++]);
                    putchar(10);

            }
    */

  status = atoi(pres[3]);
  printf("%d-%d\n", __LINE__, status);
  if(row==0)
  {
    strcpy(msg->msg, "id或密码错误");
    return;
  }
  if (strcmp(pres[2], msg->peoplemsg.password))
  {
    strcpy(msg->msg, "id或密码错误");
  }
  else if (status == ONLINE)
  {
    strcpy(msg->msg, "账号已登录");
  }
  else if (status == OFFLINE &&
           (!strcmp(pres[2], msg->peoplemsg.password)))
  {
    if ((!strcmp(msg->peoplemsg.id, ROOTID)))
      msg->permissions = ROOT;
    else
      msg->permissions = USER;
    memset(sql, 0, sizeof(sql));
    // 登录成功 状态设为在线
    sprintf(sql, "update Zhangscorporation set status=%d where id='%s' ",
            ONLINE, msg->peoplemsg.id);
    res = sqlite3_exec(sq, sql, NULL, NULL, &errmsg);
    if (res)
    {
      printf("%d-%s\n", __LINE__, errmsg);
      strcpy(msg->msg, "未知错误，请重试");
      return;
    }
    strcpy(msg->msg, "OK");
    strcpy(msg->idself, msg->peoplemsg.id);
  }
  else
  {
    printf("%d-%d-%s\n", __LINE__, status, msg->peoplemsg.id);
    strcpy(msg->msg, "未知错误，请重试");
  }
  sqlite3_free_table(pres);
}

void sigup(struct msg_st *msg, sqlite3 *sq)
{
  int res;
  char sql[SQLSIZE] = "";
  char **pres = NULL;
  int row, col;
  char *permsg = NULL;
  char *errmsg = NULL;
  char maxid[IDSIZE];

  bzero(msg->peoplemsg.id, sizeof(msg->peoplemsg.id));
  bzero(msg->peoplemsg.password, sizeof(msg->peoplemsg.password));
  bzero(msg->msg, sizeof(msg->msg));

  sprintf(sql, "select maxid from Zhangscorporation where id='%s'", ROOTID);
  res = sqlite3_get_table(sq, sql, &pres, &row, &col, &permsg);
  if (res)
  {
    printf("%d-sqlite3_get_table error : %s\n", __LINE__, permsg);
    strcpy(msg->msg, "系统获取数据错误");
    return;
  }
  printf("%d-%s\n", __LINE__, pres[1]);
  sprintf(maxid, "%d", atoi(pres[1]) + 1);
  puts(maxid);
  sqlite3_free_table(pres);
  bzero(sql, sizeof(sql));
  sprintf(sql,
          "insert into Zhangscorporation(id, name, department, password, "
          "status, age, sex, address, phone, money, maxid) "
          "values('%s','%s','%s','0000', 0, 0, '-', '-', '-', 0, '-')",
          maxid, msg->peoplemsg.name, msg->peoplemsg.department);
  res = sqlite3_get_table(sq, sql, &pres, &row, &col, &permsg);
  if (res)
  {
    printf("%d-sqlite3_get_table error : %s\n", __LINE__, permsg);
    strcpy(msg->msg, "系统注册数据错误");
    return;
  }
  bzero(sql, sizeof(sql));
  sprintf(sql, "update Zhangscorporation set maxid='%s' where id='%s'", maxid,
          ROOTID);
  res = sqlite3_exec(sq, sql, NULL, NULL, &errmsg);
  if (res)
  {
    printf("%d-%s\n", __LINE__, errmsg);
    strcpy(msg->msg, "未知错误，请重试");
    return;
  }
  strcpy(msg->peoplemsg.id, maxid);
  strcpy(msg->peoplemsg.password, "0000");
  strcpy(msg->msg, "OK");
  sqlite3_free_table(pres);
}

void cancel(struct msg_st *msg, sqlite3 *sq)
{
  char sql[SQLSIZE] = "";
  int res;
  char *errmsg = NULL;
  sprintf(sql, "update Zhangscorporation set status=%d where id='%s' ",
          OFFLINE, msg->idself);
  // puts(sql);
  res = sqlite3_exec(sq, sql, NULL, NULL, &errmsg);
  if (res)
  {
    printf("%d-%s\n", __LINE__, errmsg);
    strcpy(msg->msg, "未知错误，请重试");
    return;
  }
  strcpy(msg->msg, "OK");
}

void delusr(struct msg_st *msg, sqlite3 *sq)
{
  char sql[SQLSIZE] = "";
  int res;
  char *errmsg = NULL;

  sprintf(sql, "delete from Zhangscorporation where id='%s'",
          msg->peoplemsg.id);
  res = sqlite3_exec(sq, sql, NULL, NULL, &errmsg);
  if (res)
  {
    printf("%d-%s\n", __LINE__, errmsg);
    strcpy(msg->msg, "删除失败，请重试");
    return;
  }
  strcpy(msg->msg, "OK");
}
int findcallback(void *arg, int ncolumn, char **column_text,
                 char **column_name)
{
  struct msg_st msg_ = ((struct choose_st *)arg)->msgcallback;
  int sfd = ((struct choose_st *)arg)->sfd;
  int find_ret = ((struct choose_st *)arg)->find_ret;
  int res;

  /*
            int i;
            for(i = 0; i < ncolumn; i++)
                    printf("%s\n", column_text[i]);
                    putchar(10);
    */
  if (msg_.permissions == USER && (!strcmp(column_text[0], msg_.idself)))
    find_ret = FIND_RET_SELF;
  memset(&(msg_.peoplemsg), 0, sizeof(msg_.peoplemsg));
  switch (find_ret)
  {
  case FIND_RET_ALL:
  case FIND_RET_SELF:
    msg_.peoplemsg.age = atoi(column_text[2]);
    msg_.peoplemsg.sex = *(column_text[3]);
    msg_.peoplemsg.status = atoi(column_text[9]);
    msg_.peoplemsg.money = atoi(column_text[7]);
    strcpy(msg_.peoplemsg.address, column_text[4]);
    strcpy(msg_.peoplemsg.password, column_text[8]);
  case FIND_RET_HALF:
    strcpy(msg_.peoplemsg.id, column_text[0]);
    strcpy(msg_.peoplemsg.name, column_text[1]);
    strcpy(msg_.peoplemsg.department, column_text[5]);
    strcpy(msg_.peoplemsg.phone, column_text[6]);
  }
  if (msg_.permissions == USER && (!strcmp(msg_.peoplemsg.id, ROOTID)))
    return 0;
  res = send(sfd, &msg_, sizeof(msg_), 0);
  if (res < 0)
  {
    ERR("send");
  }
  ((struct choose_st *)arg)->flag = 1;
  return 0;
}
void find(struct msg_st *msg, sqlite3 *sq, int sfd)
{
  char sql[SQLSIZE] = "";
  int res;
  char *errmsg = NULL;
  struct choose_st choose;

  if (!(strlen(msg->msg)))
  {
    // 全部人员打印
    sprintf(sql, "select *from Zhangscorporation");
  }
  else
  {
    sprintf(sql,
            "select *from Zhangscorporation where id='%s' or name='%s' or "
            "department='%s'",
            msg->peoplemsg.id, msg->peoplemsg.name,
            msg->peoplemsg.department);
  }
  puts(sql);
  choose.sfd = sfd;
  choose.msgcallback = *msg;
  choose.flag = 0;
  if (msg->permissions == ROOT)
    choose.find_ret = FIND_RET_ALL;
  else if (strcmp(msg->idself, msg->peoplemsg.id))
    choose.find_ret = FIND_RET_HALF;
  else
    choose.find_ret = FIND_RET_SELF;

  res = sqlite3_exec(sq, sql, findcallback, &choose, &errmsg);
  if (res)
  {
    printf("%d-%s\n", __LINE__, errmsg);
    strcpy(msg->msg, "未知错误，请重试");
    return;
  }
  if (choose.flag)
    strcpy(msg->msg, "OK查询完成");
  else
    strcpy(msg->msg, "OK未查到");
}

void chang(struct msg_st *msg, sqlite3 *sq)
{
  char sql[SQLSIZE] = "";
  int res;
  char *errmsg = NULL;

  if (msg->permissions == USER)
  {
    if ((strcmp(msg->chang.changkey, "department") == 0) ||
        (strcmp(msg->chang.changkey, "money") == 0))
    {
      printf("%d-%s\n", __LINE__, "权限不足");
      strcpy(msg->msg, "权限不足");
      return;
    }
  }
  sprintf(sql, "update Zhangscorporation set '%s'='%s' where id='%s'",
          msg->chang.changkey, msg->chang.changvalues, msg->chang.changid);
  puts(sql);
  res = sqlite3_exec(sq, sql, NULL, NULL, &errmsg);
  if (res)
  {
    printf("%d-%s\n", __LINE__, errmsg);
    strcpy(msg->msg, "修改失败，请重试");
    return;
  }

  strcpy(msg->msg, "OK");
}

// 改变用户状态
void change_status(sqlite3 *sq, char *id, int status)
{
  char sql[SQLSIZE] = "";
  char *errmsg = NULL;
  sprintf(sql, "update Zhangscorporation set status=%d where id='%s' ",
          status, id);
  puts(sql);
  sqlite3_exec(sq, sql, NULL, NULL, &errmsg);
}
