/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

文件名      : config.h
作者        : 张彦升
版本        : 1.0
创建日期    : 2013年10月17日 8:45:41
用途        : 配置信息
历史修改记录:
    v1.0    创建
            最初设计使用platform.conf做配置信息存储，最后舍弃，我们可以使用向main
            传入参数实现配置，另外也可以根据环境变量进行传入参数，这两种方案都会减少
            工作量，使用配置文件方式毕竟要对文件做很多处理，如果这个文件的可读权限
            被意外去掉怎么办？这也会增加维护难度，而相对使用环境变量方式系统将会更加
            灵活。
    v2.0    张彦升   使用xml作为配置文件
            使用xml作为配置文件更加清晰，并且将4个主机上的配置整合到同一个配置文件
            当中，这样在程序当中便能够验证4个主机上的配置正确性
    v2.1    将ConfigurationId改为DeviceId，很多地方用到了DeviceId，所以有必要
            抽取出来
**********************************************************************/
#include <fcntl.h>
#include <ctype.h>
#include <expat.h>

#include "ci_header.h"
#include "log.h"
#include "config.h"
#include "util/app_config.h"

static int32_t device_id = 0;
static XML_Parser parser = NULL;

/*一个configuration当中存放的最大属性的个数，目前使用10个已经够用，若需要更多请更改程序*/
#define CONFIG_FILE_MAXCHARS 1000000
static char xmltext[CONFIG_FILE_MAXCHARS] = {0};

/*跟踪xml文档的深度 */
static int32_t depth = 0;

#define Node_TYPE(ob)             (((XmlNode*)(ob))->ob_type)
#define Node_PARENT(ob)           (((XmlNode*)(ob))->parent)
#define Node_HEAD                 XmlNode ob_base;

/* Macros trading binary compatibility for speed. See also pymem.h.
   Note that these macros expect non-NULL object pointers.*/
#define Node_INIT(op, typeobj) \
    ( Node_TYPE(op) = (typeobj),Node_PARENT(op) = NULL,(op) )

typedef struct _XmlNode
{
    struct _TypeNode *ob_type;
    struct _XmlNode* parent;
}XmlNode;

typedef struct _TypeNode {
    const char *tp_name;        /* For printing*/
} TypeNode;

#define NAME_MAX_LEN 40
#define VALUE_MAX_LEN 128
/*
 * 属性
 */
typedef struct _PropertyNode
{
    Node_HEAD
    char name[NAME_MAX_LEN];           /*属性名称*/
    char value[VALUE_MAX_LEN];         /*属性值*/
#if 0   /*description is useless*/
    char description[VALUE_MAX_LEN];   /*属性说明*/
#endif

}PropertyNode;

static TypeNode Property_Type = {
    "Property",                 /*tp_name*/
};

#define CONFIGURATION_MAX_CHILDREN 20
/*
 * Configuration用来保存配置信息
 */
typedef struct _ConfigurationNode
{
    Node_HEAD
    XmlNode* children[CONFIGURATION_MAX_CHILDREN];
    int16_t children_size;
    int32_t id;
}ConfigurationNode;

static TypeNode Configuration_Type = {
    "Configuration",            /*tp_name*/
};

#define NODE_STACK_SIZE  30

static XmlNode* node_stack[NODE_STACK_SIZE] = {0};
static int32_t node_stack_size = 0;

#define NODE_MAXSIZE                100
static PropertyNode property_nodes[NODE_MAXSIZE];
static int32_t property_node_size = 0;
static ConfigurationNode configuration_nodes[NODE_MAXSIZE];
static int32_t configuration_node_size = 0;

static CI_BOOL b_parse_error = CI_FALSE;
static const ConfigurationNode* using_configuration = NULL;

/*
 功能描述    : 根据查询条件查询，当条件中既指定了name由指定了value时，两者的值都
              匹配后返回，若只指定了name未指定value，则查询到Name就返回
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年3月15日 16:47:41
*/
static const PropertyNode* config_find_property(const ConfigSelectCondition* con)
{
    int32_t hit_time = 0;
    int32_t i = 0;
    const PropertyNode* result = NULL;

    if (NULL == con || NULL == con->name)
    {
        return NULL;
    }

    for (i = 0;i < property_node_size;i++)
    {
        if (strcmp(property_nodes[i].name,con->name) == 0)
        {
            if (NULL == con->value)
            {
                result = &property_nodes[i];
                hit_time++;
            }
            else
            {
                /*如果value有值，则对value一并查询*/
                if (strcmp(property_nodes[i].value,con->value) == 0)
                {
                    result = &property_nodes[i];
                    hit_time++;
                }
            }
        }
    }
    if (hit_time > 1)
    {
        if (NULL == con->value)
        {
            CILog_Msg("Property Name(%s) multi defined",con->name);
        }
        else
        {
            CILog_Msg("Property Name(%s) Value(%s) multi defined",con->name,con->value);
        }

        return NULL;
    }

    return result;
}
/*
 功能描述    : 
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年3月16日 9:38:54
*/
static const ConfigurationNode* config_find_configuration(int32_t id)
{
    const ConfigurationNode* result = NULL;
    int32_t i = 0;
    int32_t hit_time = 0;

    for (i = 0;i < configuration_node_size;i++)
    {
        if (configuration_nodes[i].id == id)
        {
            result = &configuration_nodes[i];
            hit_time ++;
        }
    }

    if (hit_time > 1)
    {
        CILog_Msg("Configuration id multi defined");

        return NULL;
    }

    return result;
}
/*
 功能描述    : 在节点树上递归查找父节点及兄弟节点当中值为p_item的项，
              如果找到一项立刻返回，不做多项定义检查
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年3月16日 9:16:38
*/
static const char* config_select_value(const char* p_item,const ConfigurationNode* parent_node)
{
    int i = 0;
    PropertyNode* property_node = NULL;

    if (NULL == parent_node || NULL == p_item)
    {
        return NULL;
    }

    /*遍历兄弟节点及追溯根节点的父节点，从而找到p_item的值*/

    for (i = 0;i < parent_node->children_size;i++)
    {
        if (parent_node->children[i]->ob_type == &Property_Type)
        {
            property_node = (PropertyNode*)parent_node->children[i];
            if (strcmp(property_node->name,p_item) == 0)
            {
                return property_node->value;
            }
        }
    }
    /*recursive find parent.children nodes*/
    return config_select_value(p_item,(ConfigurationNode*)Node_PARENT(parent_node));
}
/*
功能描述    : 获取系统配置项值
参数        : @p_item 要得到的配置项的名称
返回值      : 若找不到则返回NULL
             若找到唯一的一项，则以字符串返回
日期        : 2013年11月18日 10:46:55
*/
const char* CIConfig_GetValue(const char* p_item)
{
    return config_select_value(p_item,using_configuration);
}
/*
功能描述    : 该函数设计的最终目的是能选出复杂的元素，但是做这些工作显得有些徒劳。
             使用支持xPath的xml库能很顺利的解决这个问题。
参数        : @p_item 要获取的配置项的名称
             @con 设置的条件
返回值      : 成功返回取得的值的指针，未成功返回NULL
作者        : 何境泰
日期        : 2013年11月18日 10:46:55
*/
const char* CIConfig_SelectValue(const char* p_item,const ConfigSelectCondition* con)
{
    const PropertyNode* node = NULL;

    if (NULL == p_item || NULL == con)
    {
        return NULL;
    }
    node = config_find_property(con);
    if (NULL == node)
    {
        return NULL;
    }
    else
    {
        return config_select_value(p_item,(ConfigurationNode*)Node_PARENT(node));
    }
}
/*
 功能描述    : 验证config文件当中节点的属性是否正确
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年3月15日 11:26:53
*/
static XmlNode* config_create_node(const char *el, const char **attr)
{
    XmlNode* node = NULL;
    int32_t i = 0;

    if (NULL == el || NULL == attr)
    {
        CILog_Msg("config_create_node parameter is null");
        b_parse_error = CI_TRUE;
        return NULL;
    }

    if (strcmp(el,"Configuration") == 0)
    {
        if (NODE_MAXSIZE <= configuration_node_size)
        {
            CILog_Msg("There are too many configuration Element,Line:%d,Column:%d",
                XML_GetCurrentLineNumber(parser),
                XML_GetCurrentColumnNumber(parser)
                );

            b_parse_error = CI_TRUE;
            return NULL;
        }
        node = (XmlNode*)Node_INIT(&configuration_nodes[configuration_node_size],
            &Configuration_Type);

        configuration_nodes[configuration_node_size].children_size = 0;
        /*id,default is 0*/
        configuration_nodes[configuration_node_size].id = 0;
        for (i = 0; attr[i]; i += 2)
        {
            if (strcmp("id",attr[i]) == 0)
            {
                /*TODO how to copy safely?*/
                configuration_nodes[configuration_node_size].id = atoi(attr[i + 1]);
            }
            else
            {
                CILog_Msg("Element configuration attribute not recognised:%s=%s,Line:%d,Column:%d",
                    attr[i],attr[i + 1],
                    XML_GetCurrentLineNumber(parser),
                    XML_GetCurrentColumnNumber(parser)
                    );
                b_parse_error = CI_TRUE;

                return NULL;
            }
        }
        configuration_node_size++;

        return node;
    }
    else if (strcmp(el,"Property") == 0)
    {
        if (NODE_MAXSIZE <= property_node_size)
        {
            CILog_Msg("There are too many property Element,Line:%d,Column:%d",
                XML_GetCurrentLineNumber(parser),
                XML_GetCurrentColumnNumber(parser)
                );

            b_parse_error = CI_TRUE;
            return NULL;
        }

        node = (XmlNode*)Node_INIT(&property_nodes[property_node_size],&Property_Type);

        for (i = 0; attr[i]; i += 2)
        {
            if (strcmp("Name",attr[i]) == 0)
            {
                strcpy(property_nodes[property_node_size].name, attr[i + 1]);
            }
            else if (strcmp("Value",attr[i]) == 0)
            {
                strcpy(property_nodes[property_node_size].value,attr[i + 1]);
            }
            else if (strcmp("Description",attr[i]) == 0)
            {
#if 0   /*description is useless*/
                strcpy(property_nodes[property_node_size].description,attr[i + 1]);
#endif
            }
            else
            {
                CILog_Msg("Element Property attribute not recognised:%s=%s,Line:%d,Column:%d",
                    attr[i],attr[i + 1],
                    XML_GetCurrentLineNumber(parser),
                    XML_GetCurrentColumnNumber(parser)
                    );
                b_parse_error = CI_TRUE;

                return NULL;
            }
        }

        property_node_size++;

        return node;
    }
    else
    {
        /*parser error*/
        CILog_Msg("not recognise Element:%s,Line:%d,Column:%d",
            el,
            XML_GetCurrentLineNumber(parser),
            XML_GetCurrentColumnNumber(parser)
            );
        b_parse_error = CI_TRUE;

        return NULL;
    }
}
/*
 功能描述    : 进入文档树一层时调用
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年3月14日 21:09:17
*/
static void config_parse_start(void* UNUSED(data), const char *el, const char **attr)
{
    XmlNode* node = NULL;

    if (CI_TRUE == b_parse_error || NULL == el || NULL == attr)
    {
        return;
    }

    if(NULL == (node = config_create_node(el,attr)))
    {
        return;
    }
    
    if (0 == depth && node->ob_type != &Configuration_Type)
    {
        CILog_Msg("root Element must be configuration");

        b_parse_error = CI_TRUE;

        return;
    }
    if (NODE_STACK_SIZE <= node_stack_size)
    {
        CILog_Msg("xml node stack overflow");
        b_parse_error = CI_TRUE;

        return;
    }
    /*push this node to stack*/
    node_stack[node_stack_size++] = node;

    depth++;
}
/*
 功能描述    : 退出文档树一层时调用
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年3月14日 21:09:09
*/
static void config_parse_end(void* UNUSED(data), const char *el)
{
    XmlNode* node = NULL;
    ConfigurationNode* parent_node = NULL;
    int32_t this_node_pos = node_stack_size - 1;

    if (CI_TRUE == b_parse_error || NULL == el)
    {
        return;
    }
    
    if (0 > this_node_pos)
    {
        CILog_Msg("stack is empty");
        b_parse_error = CI_TRUE;

        return;
    }
    
    /*pop stack*/
    node_stack_size --;
    /*get this node*/
    node = node_stack[this_node_pos];
    /*stack is not empty beside this node*/
    if (0 <= this_node_pos - 1)
    {
        /*现有逻辑当中只有Configuration可以是父节点*/
        if (node_stack[this_node_pos - 1]->ob_type != &Configuration_Type)
        {
            CILog_Msg("Document structure not recognised,%s shouldn't be parent node,Line:%d,Column:%d",
                node_stack[this_node_pos - 1]->ob_type->tp_name,
                XML_GetCurrentLineNumber(parser),
                XML_GetCurrentColumnNumber(parser)
                );
            b_parse_error = CI_TRUE;

            return;
        }
        /*add this to parent children list*/
        parent_node = (ConfigurationNode*)node_stack[this_node_pos - 1];
        if (CONFIGURATION_MAX_CHILDREN <= parent_node->children_size)
        {
            CILog_Msg("There are too many configuration children,Line:%d,Clomn:%d",
                XML_GetCurrentLineNumber(parser),
                XML_GetCurrentColumnNumber(parser)
                );
            b_parse_error = CI_TRUE;

            return;
        }
        parent_node->children[parent_node->children_size++] = node;
        node->parent = (XmlNode*)parent_node;
    }
    
    depth--;
}

/*
 功能描述    : 从配置文件当中得到设备的id
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年3月27日 14:11:15
*/
int32_t CIConfig_GetDeviceId(void)
{
    ConfigSelectCondition con = {"DeviceId",NULL};
    const PropertyNode* node = NULL;

    if (device_id)
    {
        return device_id;
    }

    node = config_find_property(&con);
    if (NULL != node)
    {
        device_id = atoi(node->value);
    }
    else
    {
        CILog_Msg("配置文件当中DeviceId项未配置");

        return -1;
    }

    if (0 >= device_id || 5 < device_id)
    {
        CILog_Msg("配置文件当中DeviceId(%d)项配置不正确，范围1-5",device_id);

        return -1;
    }

    return device_id;
}
/*
 功能描述    : 设置setting文件存放的目录
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年6月17日 13:47:17
*/
int32_t CIConfig_SetSettingPath(const char* path)
{
    if (NULL == path)
    {
        return -1;
    }
    return 0;
}
/*
 功能描述    : 初始化系统配置，在该函数当中会检查配置文件的格式及内容
              是否正确
 返回值      : 成功为0，失败为-1
 参数        : @file_path 配置文件的路径
 日期        : 2015年3月14日 17:27:52
*/
int32_t CIConfig_Init(void)
{
    FILE* fp = NULL;
    const char* file_path = NULL;

    if( (fp = fopen(APP_CONFIG_PATH,"r")) == NULL )
    {
        CILog_Msg("无法找到%s文件!", APP_CONFIG_PATH);

        return -1;
    }

    /*当在测试程序当中测试该项目时，多次初始化会使这些变量包含脏数据*/
    device_id = 0;
    depth = 0;
    memset(node_stack,0,sizeof(XmlNode*) * NODE_STACK_SIZE);
    node_stack_size = 0;

    memset(&property_nodes,0,sizeof(PropertyNode) * NODE_MAXSIZE);
    property_node_size = 0;

    memset(configuration_nodes,0,sizeof(ConfigurationNode) * NODE_MAXSIZE);
    configuration_node_size = 0;

    b_parse_error = CI_FALSE;
    using_configuration = NULL;

    memset(xmltext,0,CONFIG_FILE_MAXCHARS * sizeof(char));

    parser = XML_ParserCreate(NULL);
    if (parser == NULL)
    {
        CILog_Msg("expat XML_ParserCreate Fail");
        return -1;
    }

    XML_SetElementHandler(parser, config_parse_start, config_parse_end);

    fread(xmltext, sizeof(char), CONFIG_FILE_MAXCHARS, fp);
    if (XML_Parse(parser, xmltext, strlen(xmltext), XML_TRUE) == XML_STATUS_ERROR)
    {
        CILog_Msg("解析%s文档失败:%s,行%d:列:%d",
            file_path,
            XML_ErrorString(XML_GetErrorCode(parser)),
            XML_GetCurrentLineNumber(parser),
            XML_GetCurrentColumnNumber(parser)
            );

        goto fail;
    }
    if (CI_TRUE == b_parse_error)
    {
        goto fail;
    }

    /*请牢记释放内存*/
    fclose(fp);

    XML_ParserFree(parser);
    parser = NULL;

#if 0
    CILog_Msg("配置文件%s检查成功",file_path);
#endif
    device_id = CIConfig_GetDeviceId();

    using_configuration = config_find_configuration(device_id);
    if (NULL == using_configuration)
    {
        return -1;
    }

    return 0;

fail:
    /*请牢记释放内存*/
    fclose(fp);

    XML_ParserFree(parser);
    parser = NULL;

    return -1;
}
