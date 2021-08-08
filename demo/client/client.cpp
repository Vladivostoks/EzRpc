/**
 * @file client.cpp
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2021-07-18
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "stdio.h"

#include "../server/dev/api_cluster.h"

int main(int argc,char* argv[])
{
    int a = 12;
    float b = 64.0f;
    bool c = true;
    int ret = api_3(a,b,c);
    printf("get ret=>%d\n",ret);
    // ret = api_3(a,b,c);
    ret = api_2(b,c);
    printf("get ret=>%d\n",ret);
    // ret = api_3(a,b,c);
    ret = api_1(c);
    printf("get ret=>%d\n",ret);
}
