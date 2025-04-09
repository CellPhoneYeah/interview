
#include <sstream>
#include <string>
#include <cstring>
#include <map>
#include <vector>

#define INT_LEN sizeof(int)

template<typename SerializableType>
std::string serialize(SerializableType& t){
    return t.serialize();
}

template<typename DeserializableType>
int deserialize(std::string &str, DeserializableType& t){
    return t.deserialize(str);
}

#define DEF_SERIALIZE_BASIC_CPP_TYPE(Type) \
template<> \
std::string inline serialize(Type& t){ \
    std::string ret;\
    ret.append((const char*)&t, sizeof(t));\
    return ret;\
    }

#define DEF_DESERIALIZE_BASIC_CPP_TYPE(Type) \
template<> \
int inline deserialize(std::string &str, Type& t){\
    memcpy(&t, str.data(), sizeof(t));\
    return sizeof(t);\
}

#define DEF_SERIALIZE_AND_DESERIALIZE(Type) \
    DEF_SERIALIZE_BASIC_CPP_TYPE(Type) \
    DEF_DESERIALIZE_BASIC_CPP_TYPE(Type);

DEF_SERIALIZE_AND_DESERIALIZE(char);
DEF_SERIALIZE_AND_DESERIALIZE(unsigned char);
DEF_SERIALIZE_AND_DESERIALIZE(short int);
DEF_SERIALIZE_AND_DESERIALIZE(unsigned short int);
DEF_SERIALIZE_AND_DESERIALIZE(unsigned int);
DEF_SERIALIZE_AND_DESERIALIZE(int);
DEF_SERIALIZE_AND_DESERIALIZE(long int);
DEF_SERIALIZE_AND_DESERIALIZE(unsigned long int);
DEF_SERIALIZE_AND_DESERIALIZE(float);
DEF_SERIALIZE_AND_DESERIALIZE(long long int);
DEF_SERIALIZE_AND_DESERIALIZE(unsigned long long int);
DEF_SERIALIZE_AND_DESERIALIZE(double);

template<>
std::string inline serialize(std::string& str){
    std::string ret;
    int len = str.size();
    ret.append(::serialize(len));
    ret.append(str.data(), len);
    return ret;
}

template<>
int inline deserialize(std::string &str, std::string &target){
    int len;
    ::deserialize(str, len);
    target = str.substr(sizeof(len), len);
    return sizeof(int) + len;
}

class SerializeOS{
private:
    std::ostringstream oss;
public:
    SerializeOS(): oss(std::ios::binary){
    }

    template<typename T>
    SerializeOS& operator<< (T &t){
        std::string s = serialize(t);
        oss.write(s.data(), s.size());
        return *this;
    }

    template<typename T>
    SerializeOS& operator<< (std::vector<T> v){
        int len = v.size();
        std::string str = ::serialize(len);
        oss.write(str.data(), str.size());
        for (typename std::vector<T>::const_iterator i = v.begin(); i != v.end(); i++)
        {
            T t = *i;
            std::string s = ::serialize(t);
            oss.write(s.data(), s.size());
        }
        return *this;
    }

    template<typename T1, typename T2>
    SerializeOS& operator<< (std::map<T1, T2> &m){
        std::vector<T1> keys;
        std::vector<T2> values;
        for (typename std::map<T1, T2>::const_iterator i = m.begin(); i != m.end(); i++)
        {
            keys.push_back((*i).first);
            values.push_back((*i).second);
        }
        this->operator<<(keys);
        this->operator<<(values);
        
        return *this;
    }

    std::string getStr(){
        return oss.str();
    }
};

class SerializeIS{
private:
    std::string str;
    int total;
public:
    SerializeIS(std::string s){
        this->str = s;
        this->total = s.size();
    }

    template<typename T>
    SerializeIS& operator>> (T &t){
        int len = deserialize(str, t);
        str = str.substr(len);
        return *this;
    }

    template<typename T>
    SerializeIS& operator>>(std::vector<T> &v){
        int veclen = 0;
        this->operator>>(veclen);
        for (int i = 0; i < veclen; i++)
        {
            T t;
            this->operator>>(t);
            v.push_back(t);
        }
        return *this;
    }

    template<typename T1, typename T2>
    SerializeIS& operator>> (std::map<T1, T2> &m){
        std::vector<T1> v1;
        std::vector<T2> v2;
        this->operator>>(v1);
        this->operator>>(v2);
        typename std::vector<T2>::const_iterator i2 = v2.begin();
        for (typename std::vector<T1>::const_iterator i = v1.begin(); i != v1.end(); i++)
        {
            m.insert(std::make_pair(*i, *i2));
            i2++;
        }
        return *this;
    }

    int leftSize(){
        return total - str.size();
    }
};