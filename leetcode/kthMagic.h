#include <iostream>
#include <vector>
#include <algorithm>

int getKthMagicNumber(int k) {
    std::vector<int> dp(k+1, 1);
    std::vector<int> factor = {3, 5, 7};
    int p3 = 1, p5 = 1, p7 = 1;
    for(int i = 2; i <= k; i++){
        int num3 = dp[p3] * 3, num5 = dp[p5] * 5, num7 = dp[p7] * 7;
        dp[i] = std::min(std::min(num3, num5), num7);
        if(dp[i] == num3){
            p3++;
        }
        if(dp[i] == num5){
            p5++;
        }
        if(dp[i] == num7){
            p7++;
        }
    }
    return dp[k];
}