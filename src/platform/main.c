/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年10月12日 12:48:50
用途        : 主程序
历史修改记录: 
**********************************************************************/
#include "app.h"
#include "util/config.h"

int main(int argc,char * argv[])
{
    int ret = 0;
    CIAppOps* app_ops = NULL;

#ifdef WIN32
    extern CIAppOps win_app_ops;
    app_ops = &win_app_ops;
#else
    extern CIAppOps linux_app_ops;
    app_ops = &linux_app_ops;
#endif /*WIN32*/

    CIApp_SaveProgramName(argv[0]);

    ret = app_ops->decode_arguments(argc,argv);
    if (-1 == ret)
    {
        return 1;
    }
    ret = CIConfig_Init();
    assert(-1 != ret);
    /*
     * 务必在decode_arguments之后再做init，因为部分初始化模块会根据参数来判断是否需要
     * 打开相应功能
     */
    ret = app_ops->init();
    if (0 > ret)
    {
        return 1;
    }

    app_ops->run();

    app_ops->terminate();

    return 1;
}