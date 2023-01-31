#ifndef USER_H
#define USER_H

// 实现数据库表User的ORM类，业务使用UserModel来操作数据库，而不直接使用数据库
// user表：(id,name,password,state)=(int,string,string,string)
#include <string>

using std::string;

class User {
  public:
    User(
        int id = -1,
        string name = "",
        string password = "",
        string state = "offline") {
        this->id = id;
        this->name = name;
        this->password = password;
        this->state = state;
    }
    void setId(int id) { this->id = id; }
    void setName(string name) { this->name = name; }
    void setPwd(string pwd) { this->password = pwd; }
    void setState(string state) { this->state = state; }

    int getId() { return this->id; }
    string getName() { return this->name; }
    string getPwd() { return this->password; }
    string getState() { return this->state; }

  private:
    int id;
    string name;
    string password;
    string state;
};

#endif