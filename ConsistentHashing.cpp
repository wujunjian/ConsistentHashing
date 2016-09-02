#include <stdint.h>
#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include "ConsistentHashing.h"


uint32_t getcrc(const char* buffer, size_t size);
Obj2node::Obj2node(int cache, int vnum)
{
    pthread_rwlock_init(&m_key2vnode_lock, NULL);
    m_CacheNum = cache;
    m_virtualNum = vnum;
    m_is_active = false;
}

Obj2node::~Obj2node()
{
    
}

void Obj2node::AddNode(std::string ip) 
{
    pthread_rwlock_wrlock(&m_key2vnode_lock);
    m_is_active = false;
    for(int i=0;i<m_virtualNum;i++)
    {
        virtualnode* tmpvnode=new virtualnode(ip);
        tmpvnode->VId = i;

        std::string ipNum = ip+"#"+transToString(i);
        int key = getcrc(ipNum.c_str(), ipNum.size()) % m_CacheNum;
        std::map<int,virtualnode*>::iterator iter = m_key2vnode.find(key);
        if(iter!= m_key2vnode.end())
        {
            if(iter->second->Id == 0)
            {
                if(iter->second->VId < tmpvnode->VId) 
				{
					delete tmpvnode;
                    continue;
                } 
				else if(iter->second->VId == tmpvnode->VId)
 				{
                    if((iter->second->m_ip).compare(ip) <= 0) {
						delete tmpvnode;
                        continue;
                    }
                }
            }
			delete iter->second;
        }
        m_key2vnode[key] = tmpvnode;
    }
    pthread_rwlock_unlock(&m_key2vnode_lock);
}

bool Obj2node::Active() 
{
    pthread_rwlock_wrlock(&m_key2vnode_lock);
    
    if(m_key2vnode.size()==0)
    {
        return false;
    }
    
    int pre_key = m_CacheNum+1;
    int now_key;
    int first_key = m_key2vnode.begin()->first;
    for(std::map<int,virtualnode*>::iterator iter = m_key2vnode.begin();iter!=m_key2vnode.end();iter++)
    {
        if(iter->second == NULL || iter->second->Id != 0)
        {
			continue;
		}
        now_key = iter->first;
        if(pre_key == m_CacheNum+1) 
		{
        	pre_key = now_key;
        	continue;
        }
            
		int tmpVid = m_key2vnode[pre_key]->VId;
		std::string tmpIp = m_key2vnode[pre_key]->m_ip;
        for(int i=pre_key+1,j=1; i<now_key; i++,j++) 
		{
        	virtualnode* tmpvnode = new virtualnode();
            tmpvnode->VId = tmpVid;
            tmpvnode->Id = j;
            tmpvnode->m_ip = tmpIp;
			
        	std::map<int,virtualnode*>::iterator previter = m_key2vnode.find(i);
			if(previter != m_key2vnode.end())
			{
				if(previter->second!=NULL)
					delete previter->second;
			}
            m_key2vnode[i] = tmpvnode;
        }
        pre_key = now_key;
    }
    
	int tmpVid = m_key2vnode[pre_key]->VId;
	std::string tmpIp = m_key2vnode[pre_key]->m_ip;
	int j=1;
    for(int i=pre_key+1;i<m_CacheNum;i++,j++)
	{
    	virtualnode* tmpvnode = new virtualnode();
    	tmpvnode->VId = tmpVid;
    	tmpvnode->Id = j;
    	tmpvnode->m_ip = tmpIp;
    	std::map<int,virtualnode*>::iterator previter = m_key2vnode.find(i);
		if(previter != m_key2vnode.end() && previter->second != NULL)
		{
			delete previter->second;
    	}
		m_key2vnode[i] = tmpvnode;
	}

    for(int i=0;i<first_key;i++,j++)
	{
    	virtualnode* tmpvnode = new virtualnode();
    	tmpvnode->VId = tmpVid;
    	tmpvnode->Id = j;
    	tmpvnode->m_ip = tmpIp;
    	std::map<int,virtualnode*>::iterator previter = m_key2vnode.find(i);
		if(previter != m_key2vnode.end() && previter->second != NULL)
		{
			delete previter->second;
    	}
		m_key2vnode[i] = tmpvnode;
    }
    
    m_is_active = true;
    pthread_rwlock_unlock(&m_key2vnode_lock);
    
    return true;
}


std::string Obj2node::Get(std::string obj) 
{
    std::string res;
    pthread_rwlock_rdlock(&m_key2vnode_lock);
    if(m_is_active) 
	{
        res = m_key2vnode[getcrc(obj.c_str(),obj.size()) % m_CacheNum]->m_ip;
    } 
	else 
	{
        std::cout << "Obj2node is not actived" << std::endl;
        exit(0);
    }
    pthread_rwlock_unlock(&m_key2vnode_lock);
    return res;
}

void Obj2node::Delete(std::string ip)
{
    pthread_rwlock_wrlock(&m_key2vnode_lock);
    m_is_active = false;
    for(int i=0;i<m_virtualNum;i++)
	{
        std::string ipNum = ip+"#"+transToString(i);
        int key = getcrc(ipNum.c_str(), ipNum.size()) % m_CacheNum;
    	std::map<int,virtualnode*>::iterator previter = m_key2vnode.find(key);
        if(previter==m_key2vnode.end() || previter->second == NULL)  
		{
            continue;
        }
        if(previter->second->m_ip != ip) 
		{
            continue;
        }
		delete previter->second;
		m_key2vnode[key] = NULL;
    }
    pthread_rwlock_unlock(&m_key2vnode_lock);
}

void Obj2node::Debug() 
{
    pthread_rwlock_rdlock(&m_key2vnode_lock);
    std::cout<<"CacheNum:"<<m_CacheNum<<"virtualNum:"<<m_virtualNum<<std::endl;
    std::cout<<"real cache num:"<<m_key2vnode.size()<<std::endl;
    std::cout<<"is actived ?"<<m_is_active<<std::endl;
	if(!m_is_active)
		return;
    
	std::map<std::string, int> count;
    for(int i=0;i<m_CacheNum;i++)
	{
        std::map<std::string, int>::iterator iter = count.find(m_key2vnode[i]->m_ip);
		if(iter==count.end())
			count[m_key2vnode[i]->m_ip]=1;
		else
		{
			int tmpnum = iter->second;
			count[m_key2vnode[i]->m_ip]=tmpnum+1;
		}
        std::cout<< "" << i << ":\t\t" <<m_key2vnode[i]->m_ip<< "\t" << m_key2vnode[i]->Id<< "\t" << m_key2vnode[i]->VId<<std::endl;
    }
	
	std::cout << "==================count==================" << std::endl;
	std::map<std::string,int>::iterator iter_count = count.begin();
	for(;iter_count!=count.end();iter_count++)
	{
		std::cout << iter_count->first << "\t" << iter_count->second << std::endl;
	}

	//std::cout<<count<<std::endl;
    pthread_rwlock_unlock(&m_key2vnode_lock);
}



uint32_t getcrc(const char* buffer, size_t size)
{
	if(size==0)
		return 0;
		
	static const uint32_t crc32tab[256] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba,
    0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
    0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
    0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
    0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,
    0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
    0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
    0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940,
    0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116,
    0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
    0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
    0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a,
    0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818,
    0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
    0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
    0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c,
    0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
    0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
    0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
    0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086,
    0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4,
    0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
    0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
    0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
    0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe,
    0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
    0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
    0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252,
    0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60,
    0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
    0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
    0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04,
    0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a,
    0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
    0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
    0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e,
    0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
    0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
    0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
    0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0,
    0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6,
    0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
    0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d,
    };
	uint64_t x;
	uint32_t crc= 4294967295U;
	for (x= 0; x < size; x++)
		crc= (crc >> 8) ^ crc32tab[(crc ^ (uint64_t)buffer[x]) & 0xff];

	return ~crc;
}
