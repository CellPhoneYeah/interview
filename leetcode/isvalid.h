#include <stack>
#include <string>
bool isValid(std::string s) {
    std::stack<char> stk;
    for(char c: s){
        if(c == '(' || c == '[' || c == '{'){
            stk.push(c);
        }else if(
            (c == ')' && (stk.size() == 0 || stk.top() != '(')) 
        || (c == ']' && (stk.size() == 0 || stk.top() != '[')) 
        || (c == '}' && (stk.size() == 0 || stk.top() != '{'))){
            return false;
        }else{
            stk.pop();
        }
    }
    return stk.size() == 0;
}