#ifndef __MANGER_H__
#define __MANGER_H__

#define ERR(msg) do{\
    perror(msg);\
    printf("%s-%s-%d\n", __FILE__, __func__, __LINE__);\
    }while(0)

#define ROOT            1                      // 管理员权限
#define USER            0                      // 普通用户权限
#define ONLINE          1                      // 在线状态
#define OFFLINE         0                      // 离线状态
#define MAN             'M'
#define FEMALE          'F'
#define IDSIZE          32
#define NAMESIZE        32
#define ADDSIZE         64
#define PHONESIZE       32
#define PASSWORDSIZE    32
#define DEPSIZE         32
#define MSGSIZE         32
#define SQLSIZE         256
#define FILENAME        "./sq.db"
#define ROOTID          "1221"                  // 管理员id
#define ROOTPASSWORD    "2112"                  // 管理员密码
// 操作码
typedef enum
{
    LOGIN = 1,
    EXIT,
    SIGUP,
    DELUSR,
    FIND,
    CHANG,
    CANCEL,
}OPT;

// 查找条件
typedef enum
{
    FIND_ID = 1,
    FIND_NAME,
    FIND_DEP,
    FIND_ALL,
}FIND_OP;

// 返回查找信息
typedef enum
{
    FIND_RET_ALL = 1,
    FIND_RET_SELF,
    FIND_RET_HALF,
}FIND_RET;
// 员工信息
struct people_st
{
    char id[IDSIZE];
    char name[NAMESIZE];
    int age;
    char sex;
    char address[ADDSIZE];
    char department[DEPSIZE];
    char phone[PHONESIZE];
    int money;
    char password[PASSWORDSIZE];
    char status;
};

// 修改信息结构体
struct change_st
{
    char changid[IDSIZE];
    char changkey[MSGSIZE];
    char changvalues[ADDSIZE];
};


// 协议
struct msg_st
{
    char permissions;
    struct people_st peoplemsg;
    OPT opt;
    char msg[MSGSIZE];
    char idself[IDSIZE];
    struct change_st chang;
};

#endif