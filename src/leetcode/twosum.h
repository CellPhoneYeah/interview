#include "runcode.h"
#include <vector>
#include <unordered_map>

using namespace std;

class twosum{
public:
    vector<int> twoSum(vector<int>& nums, int target){
        vector<int> ret;
        unordered_map<int, int> needs;
        for (size_t i = 0; i < nums.size(); i++)
        {
            int need = target - nums[i];
            std::unordered_map<int, int>::const_iterator it = needs.find(need);
            if(it != needs.end()){
                ret.push_back(i);
                ret.push_back((*it).second);
            }
            needs.insert(std::make_pair(nums[i], i));
        }
        
        return ret;
    }
};