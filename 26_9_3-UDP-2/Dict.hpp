#include "Log.hpp"
#include <iostream>
#include <string>
#include <unordered_map>

using namespace LogModule;

#define DICT_PATH "./Dict.txt"


//字典序类  提供函数 实现 英译汉
class Dict{
public:
    Dict(const std::string& path = DICT_PATH)
    : m_path(path)
    {          
    }

    bool Load()
    {
        std::ifstream in(m_path);
        if(!in.is_open())
        {
            LOG(LogLevel::ERROR)<<"open file "<<m_path<<" failed";
            return false;
        }
        std::string line;
        //读取填入哈希表
        while(std::getline(in,line))
        {
            size_t pos = line.find(": ");
            if(pos == std::string::npos)
            {
                LOG(LogLevel::WARNING)<<"加载失败: "<<line;
                continue;
            }
            else
            {
                LOG(LogLevel::DEBUG)<<"加载成功: "<<line;
            }
            std::string key = line.substr(0,pos);
            std::string value = line.substr(pos+1);
            m_dict[key] = value;
        }
        in.close();
        return true;
    }

    std::string Translate(const std::string& word, InetAddr& cli)
    {
            auto iter = m_dict.find(word);
        if (iter == m_dict.end())
        {
            LOG(LogLevel::DEBUG) << "进入到了翻译模块, [" << cli.IP() << " : " << cli.Port() << "]# " << word << "->None";
            return "None";
        }
        LOG(LogLevel::DEBUG) << "进入到了翻译模块, [" << cli.IP() << " : " << cli.Port() << "]# " << word << "->" << iter->second;
        return iter->second;
    }


    ~Dict()
    {
        
    }
private:
     std::string m_path; //文件路径
    std::unordered_map<std::string,std::string> m_dict; 

};