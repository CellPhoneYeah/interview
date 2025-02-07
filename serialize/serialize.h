
#include <sstream>
#include <string>
#include <cstring>

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
    ret.append((const char*)&t, sizeof(Type));\
    return ret;\
    }

#define DEF_DESERIALIZE_BASIC_CPP_TYPE(Type) \
template<> \
int inline deserialize(std::string &str, Type& t){\
    memcpy(&t, str.data(), sizeof(Type));\
    return sizeof(Type);\
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

    // template<typename T>
    // SerializeOS& operator<< (const T &t){
    //     std::string s = serialize(t);
    //     oss.write(s, s.size());
    //     return *this;
    // }

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

    int leftSize(){
        return total - str.size();
    }
};