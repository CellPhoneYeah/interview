// 175 LCR 计算二叉树深度，结构使用
// struct TreeNode
// {
//     int val;
//     TreeNode *left;
//     TreeNode *right;
//     TreeNode() : val(0), left(nullptr), right(nullptr) {}
//     TreeNode(int x) : val(x), left(nullptr), right(nullptr) {}
//     TreeNode(int x, TreeNode *left, TreeNode *right) : val(x), left(left), right(right) {}
// };
#include "leetcodestructs.h"

class calculatedepth{
public:
    int calculateDepth(TreeNode* root) {
        return findMaxDepth(root, 0);
    }
    int findMaxDepth(TreeNode* node, int currentDepth){
        if(node == nullptr){
            return currentDepth;
        }
        currentDepth ++;
        int leftMaxDepth = findMaxDepth(node->left, currentDepth);
        int rightMaxDepth = findMaxDepth(node->right, currentDepth);
        return leftMaxDepth > rightMaxDepth ? leftMaxDepth : rightMaxDepth;
    }
};