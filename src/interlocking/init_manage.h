/*********************************************************************
Copyright (C), 2011,  Co.Hengjun, Ltd.

作者        : 张彦升
版本        : 1.0
创建日期    : 2013年12月9日 13:51:50
用途        : 联锁表管理
历史修改记录: v1.0    创建
**********************************************************************/
#ifndef _ilt_manage_h__
#define _ilt_manage_h__

#include "util/ci_header.h"

#include "base_type_definition.h"

/****************************************************
功能描述:  判断是否为对向道岔
返回值:    
参数:      route_direction 进路方向
           node_index      节点索引号
作者  :    CY
日期  ：   2011/12/2
****************************************************/
CI_BOOL is_face_switch(uint8_t route_direction,int16_t node_index);
/*
功能描述    : 初始化联锁表相关数据，主要从ilt文件当中读取
返回值      : 成功为0，失败为-1
参数        : 
作者        : 张彦升
日期        : 2013年12月10日 18:51:02
*/
int32_t ilt_manage_init(void);

node_t get_next_node(index_t ILT_index, node_t nodes[],index_t sp);
int16_t graphic_search_route(int16_t ILT_index,int16_t nodes[]);
void modify_resolve_switch( int16_t i ) ;
void modify_switch(int16_t ILT_index);
void check_straight_route(index_t ILT_index);

void init_system_var();
CI_BOOL read_config_data();
CI_BOOL is_dead_node( int16_t node_index,EN_node_direction route_direction,CI_BOOL track_stop );
uint16_t get_nodes_switch_state(int16_t nodes[],int32_t node_count,int16_t i);
CI_BOOL is_target_nodes(int16_t ILT_index,int16_t button[],int16_t nodes[],int32_t node_count);
CI_BOOL is_passing_ILT(int16_t ILT_index);
node_t get_next_node(index_t ILT_index, node_t nodes[],index_t sp);
CI_BOOL is_target_ILT(int16_t ILT_index, int16_t next_node);
int16_t graphic_search_route(int16_t ILT_index,int16_t nodes[]);
void modify_switch(int16_t ILT_index);
void modify_resolve_switch( int16_t i ) ;

void init_ILT();
void check_straight_route(index_t ILT_index);
void reset_signal_nodes(void);
void reset_routes(void);

#endif /*_ilt_manage_h__*/