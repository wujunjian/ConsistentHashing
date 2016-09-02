#ifndef __CONSISTENTHASHING_H__
#define __CONSISTENTHASHING_H__
#include <string>
#include <map>
#include <sstream>

class virtualnode 
{
public:
    virtualnode():m_ip("127.0.0.1") {}
    virtualnode(std::string ip):m_ip(ip),Id(0) {}
    ~virtualnode() {}
    std::string m_ip;
    int Id;
    int VId;
};

class Obj2node {
public:
    Obj2node(int cache,int vnum);
    ~Obj2node();
    
    void AddNode(std::string ip);
    bool Active();
    std::string Get(std::string obj);
    void Delete(std::string ip);
    void Debug();

private:
    pthread_rwlock_t m_key2vnode_lock;
    std::map<int,virtualnode*> m_key2vnode;
    bool m_is_active;
    
    int m_CacheNum;
    int m_virtualNum;

	template<typename T>
	static std::string transToString(const T &input)
	{
        std::stringstream ss;
        ss<<input;
        return ss.str();
	}
};

#endif
